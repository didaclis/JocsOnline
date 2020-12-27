#include "ModuleNetworkingServer.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::setListenPort(int port)
{
	listenPort = port;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::onStart()
{
	if (!createSocket()) return;

	// Reuse address
	int enable = 1;
	int res = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingServer::start() - setsockopt");
		disconnect();
		return;
	}

	// Create and bind to local address
	if (!bindSocketToPort(listenPort)) {
		return;
	}

	state = ServerState::Listening;
}

void ModuleNetworkingServer::onGui()
{
	if (ImGui::CollapsingHeader("ModuleNetworkingServer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Connection checking info:");
		ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
		ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

		ImGui::Separator();

		if (state == ServerState::Listening)
		{
			int count = 0;

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				if (clientProxies[i].connected)
				{
					ImGui::Text("CLIENT %d", count++);
					ImGui::Text(" - address: %d.%d.%d.%d",
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b1,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b2,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b3,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b4);
					ImGui::Text(" - port: %d", ntohs(clientProxies[i].address.sin_port));
					ImGui::Text(" - name: %s", clientProxies[i].name.c_str());
					ImGui::Text(" - id: %d", clientProxies[i].clientId);
					if (clientProxies[i].gameObject != nullptr)
					{
						ImGui::Text(" - gameObject net id: %d", clientProxies[i].gameObject->networkId);
					}
					else
					{
						ImGui::Text(" - gameObject net id: (null)");
					}
					
					ImGui::Separator();
				}
			}

			ImGui::Checkbox("Render colliders", &App->modRender->mustRenderColliders);
		}
	}
}

void ModuleNetworkingServer::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	if (state == ServerState::Listening)
	{
		uint32 protoId;
		packet >> protoId;
		if (protoId != PROTOCOL_ID) return;

		ClientMessage message;
		packet >> message;

		ClientProxy *proxy = getClientProxy(fromAddress);

		if (message == ClientMessage::Hello)
		{
			if (proxy == nullptr)
			{
				proxy = createClientProxy();

				if (proxy != nullptr)
				{
					std::string playerName;
					uint8 spaceshipType;
					packet >> playerName;
					packet >> spaceshipType;

					proxy->address.sin_family = fromAddress.sin_family;
					proxy->address.sin_addr.S_un.S_addr = fromAddress.sin_addr.S_un.S_addr;
					proxy->address.sin_port = fromAddress.sin_port;
					proxy->connected = true;
					proxy->name = playerName;
					proxy->clientId = nextClientId++;

					// Create new network object
					vec2 initialPosition = 500.0f * vec2{ Random.next() - 0.5f, Random.next() - 0.5f};
					float initialAngle = 360.0f * Random.next();
					proxy->gameObject = spawnPlayer(spaceshipType, initialPosition, initialAngle);
				}
				else
				{
					// NOTE(jesus): Server is full...
				}
			}

			if (proxy != nullptr)
			{
				// Send welcome to the new player
				OutputMemoryStream welcomePacket;
				welcomePacket << PROTOCOL_ID;
				welcomePacket << ServerMessage::Welcome;
				welcomePacket << proxy->clientId;
				welcomePacket << proxy->gameObject->networkId;
				sendPacket(welcomePacket, fromAddress);

				// Send all network objects to the new player
				uint16 networkGameObjectsCount;
				GameObject *networkGameObjects[MAX_NETWORK_OBJECTS];
				App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
				for (uint16 i = 0; i < networkGameObjectsCount; ++i)
				{
					GameObject *gameObject = networkGameObjects[i];
					
					// TODO(you): World state replication lab session
					proxy->repManagerServer.create(gameObject->networkId);
				}
				OutputMemoryStream packet;
				proxy->repManagerServer.write(packet);
				sendPacket(packet, fromAddress);

				LOG("Message received: hello - from player %s", proxy->name.c_str());
			}
			else
			{
				OutputMemoryStream unwelcomePacket;
				unwelcomePacket << PROTOCOL_ID;
				unwelcomePacket << ServerMessage::Unwelcome;
				sendPacket(unwelcomePacket, fromAddress);

				WLOG("Message received: UNWELCOMED hello - server is full");
			}
		}
		else if (message == ClientMessage::Input)
		{
			// Process the input packet and update the corresponding game object
			if (proxy != nullptr && IsValid(proxy->gameObject))
			{
				// TODO(you): Reliability on top of UDP lab session

				// Read input data
				while (packet.RemainingByteCount() > 0)
				{
					InputPacketData inputData;
					packet >> inputData.sequenceNumber;
					packet >> inputData.horizontalAxis;
					packet >> inputData.verticalAxis;
					packet >> inputData.buttonBits;

					if (inputData.sequenceNumber >= proxy->nextExpectedInputSequenceNumber)
					{
						proxy->gamepad.horizontalAxis = inputData.horizontalAxis;
						proxy->gamepad.verticalAxis = inputData.verticalAxis;
						unpackInputControllerButtons(inputData.buttonBits, proxy->gamepad);
						proxy->gameObject->behaviour->onInput(proxy->gamepad);
						proxy->nextExpectedInputSequenceNumber = inputData.sequenceNumber + 1;
					}
				}
			}

			//send last sequence number recived
			OutputMemoryStream inputNumberPacket;
			inputNumberPacket << PROTOCOL_ID;
			inputNumberPacket << ClientMessage::InputNumber;
			inputNumberPacket << proxy->nextExpectedInputSequenceNumber;
			sendPacket(inputNumberPacket, fromAddress);
		}

		// TODO(you): UDP virtual connection lab session
	}
}

void ModuleNetworkingServer::onUpdate()
{
	if (state == ServerState::Listening)
	{
		// Handle networked game object destructions
		for (DelayedDestroyEntry &destroyEntry : netGameObjectsToDestroyWithDelay)
		{
			if (destroyEntry.object != nullptr)
			{
				destroyEntry.delaySeconds -= Time.deltaTime;
				if (destroyEntry.delaySeconds <= 0.0f)
				{
					destroyNetworkObject(destroyEntry.object);
					destroyEntry.object = nullptr;
				}
			}
		}
		if (areMoreThanOne())//Prevent the game to start with only 1 player
			manageGame();

		for (ClientProxy &clientProxy : clientProxies)
		{
			if (clientProxy.connected)
			{
				// TODO(you): UDP virtual connection lab session
				if (secondsSincelastPingPacket > PING_INTERVAL_SECONDS)
					disconnect();
				// Don't let the client proxy point to a destroyed game object
				if (!IsValid(clientProxy.gameObject))
				{
					clientProxy.gameObject = nullptr;
				}

				// TODO(you): World state replication lab session
				OutputMemoryStream packet;
				clientProxy.repManagerServer.write(packet);
				sendPacket(packet, clientProxy.address);
				// TODO(you): Reliability on top of UDP lab session
			}
		}
	}
}

void ModuleNetworkingServer::onConnectionReset(const sockaddr_in & fromAddress)
{
	// Find the client proxy
	ClientProxy *proxy = getClientProxy(fromAddress);

	if (proxy)
	{
		// Clear the client proxy
		destroyClientProxy(proxy);
	}
}

void ModuleNetworkingServer::onDisconnect()
{
	uint16 netGameObjectsCount;
	GameObject *netGameObjects[MAX_NETWORK_OBJECTS];
	App->modLinkingContext->getNetworkGameObjects(netGameObjects, &netGameObjectsCount);
	for (uint32 i = 0; i < netGameObjectsCount; ++i)
	{
		NetworkDestroy(netGameObjects[i]);
	}

	for (ClientProxy &clientProxy : clientProxies)
	{
		destroyClientProxy(&clientProxy);
	}
	
	nextClientId = 0;

	state = ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Client proxies
//////////////////////////////////////////////////////////////////////

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::createClientProxy()
{
	// If it does not exist, pick an empty entry
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clientProxies[i].connected)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxy(const sockaddr_in &clientAddress)
{
	// Try to find the client
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].address.sin_addr.S_un.S_addr == clientAddress.sin_addr.S_un.S_addr &&
			clientProxies[i].address.sin_port == clientAddress.sin_port)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

void ModuleNetworkingServer::destroyClientProxy(ClientProxy *clientProxy)
{
	// Destroy the object from all clients
	if (IsValid(clientProxy->gameObject))
	{
		destroyNetworkObject(clientProxy->gameObject);
	}

    *clientProxy = {};
}


//////////////////////////////////////////////////////////////////////
// Spawning
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::areMoreThanOne()
{
	int cont = 0;
	for (ClientProxy& clientProxy : clientProxies)
	{
		if (clientProxy.connected)
		{
			cont++;
			if (cont > 1)
				return true;
		}
	}
	return false;
}

void ModuleNetworkingServer::manageGame()
{
	if (beginSquare == false)
	{
		square = spawnSquareOfDeath();
		beginSquare = true;
	}
	
	if (Time.time - lastPowerUpTime > nextPowerUpTime) //PowerUps Management
	{
		lastPowerUpTime = Time.time;
		nextPowerUpTime = 5 + 15 * Random.next();
		int t = (int)3 * Random.next();
		PowerUp::PowerUpType type = (PowerUp::PowerUpType)t;
		vec2 initialPosition = withinSquarePosition();
		spawnPowerUp(initialPosition, 90, type);
	}
	if (Time.time - lastAsteroidTime > nextAsteroidTime)//Asteroids Management
	{
		lastAsteroidTime = Time.time;
		nextAsteroidTime = 5+ 10 * Random.next();
		int t = (int)4 * Random.next();
		Asteroid::AsteroidType type = (Asteroid::AsteroidType)t;
		spawnAsteroid(90, type);
	}
}
vec2 ModuleNetworkingServer::withinSquarePosition()
{
	vec2 upLeftPos = { square->size.x * 0.5, square->size.y * 0.5 }; // Upper left side of the square
	vec2 vec = square->position - upLeftPos;

	return { vec.x + square->size.x * Random.next(), vec.y + square->size.y * Random.next() };
}
GameObject * ModuleNetworkingServer::spawnPlayer(uint8 spaceshipType, vec2 initialPosition, float initialAngle)
{
	// Create a new game object with the player properties
	GameObject *gameObject = NetworkInstantiate();
	gameObject->position = initialPosition;
	gameObject->size = { 100, 100 };
	gameObject->angle = initialAngle;

	// Create sprite
	gameObject->sprite = App->modRender->addSprite(gameObject);
	gameObject->sprite->order = 5;
	if (spaceshipType == 0) {
		gameObject->sprite->texture = App->modResources->spacecraft1;
	}
	else if (spaceshipType == 1) {
		gameObject->sprite->texture = App->modResources->spacecraft2;
	}
	else {
		gameObject->sprite->texture = App->modResources->spacecraft3;
	}

	// Create collider
	gameObject->collider = App->modCollision->addCollider(ColliderType::Player, gameObject);
	gameObject->collider->isTrigger = true; // NOTE(jesus): This object will receive onCollisionTriggered events

	// Create behaviour
	Spaceship * spaceshipBehaviour = App->modBehaviour->addSpaceship(gameObject);
	gameObject->behaviour = spaceshipBehaviour;
	gameObject->behaviour->isServer = true;

	return gameObject;
}
GameObject* ModuleNetworkingServer::spawnPowerUp(vec2 initialPosition, float initialAngle, PowerUp::PowerUpType type)
{
	GameObject* powerup = NetworkInstantiate();
	powerup->position = initialPosition;
	powerup->angle = initialAngle;
	powerup->sprite = App->modRender->addSprite(powerup);
	powerup->sprite->order = 3;
	
	powerup->collider = App->modCollision->addCollider(ColliderType::PowerUp, powerup);

	PowerUp* powerupBehaviour = App->modBehaviour->addPowerUp(powerup);
	powerupBehaviour->isServer = true;
	powerupBehaviour->p_type = type;
	powerup->behaviour = powerupBehaviour;

	switch (powerupBehaviour->p_type)
	{
	case PowerUp::PowerUpType::SPEED:
		powerup->sprite->texture = App->modResources->powerup01;
		break;
	case PowerUp::PowerUpType::BOMB:
		powerup->sprite->texture = App->modResources->powerupBomb;
		break;
	case PowerUp::PowerUpType::WEAPON:
		powerup->sprite->texture = App->modResources->powerupWeapon;
		break;
	}
	return powerup;
}

GameObject* ModuleNetworkingServer::spawnAsteroid(float initialAngle, Asteroid::AsteroidType type)
{
	GameObject* gameObject = NetworkInstantiate();
	switch (type)
	{
	case Asteroid::AsteroidType::NORTH:
	{
		vec2 loc = { square->size.x * 0.5, square->size.y * 0.5 };
		vec2 vec = square->position - loc;
		gameObject->position = { vec.x + square->size.x * Random.next(), vec.y };
		break;
	}
	case Asteroid::AsteroidType::SOUTH:
	{
		vec2 loc = { square->size.x * 0.5, square->size.y * 0.5 };
		vec2 vec = square->position + loc;
		gameObject->position = { vec.x + square->size.x * Random.next(), vec.y };
		break;
	}
	case Asteroid::AsteroidType::EAST:
	{
		vec2 loc = { square->size.x * 0.5, square->size.y * 0.5 };
		vec2 vec = square->position + loc;
		gameObject->position = { vec.x, vec.y + square->size.y * Random.next() };
		break;
	}
	case Asteroid::AsteroidType::WEST:
	{
		vec2 loc = { square->size.x * 0.5, square->size.y * 0.5 };
		vec2 vec = square->position - loc;
		gameObject->position = { vec.x, vec.y + square->size.y * Random.next() };
		break;
	}
	}

	gameObject->angle = initialAngle;
	gameObject->size = { 100,100 };
	// Create sprite
	gameObject->sprite = App->modRender->addSprite(gameObject);
	gameObject->sprite->order = 6;
	gameObject->sprite->texture = App->modResources->asteroid1;

	// Create collider
	gameObject->collider = App->modCollision->addCollider(ColliderType::Asteroid, gameObject);
	gameObject->collider->isTrigger = true;
	// Create behaviour
	Asteroid* asteroidBehaviour = App->modBehaviour->addAsteroid(gameObject);
	asteroidBehaviour->p_type = type;
	gameObject->behaviour = asteroidBehaviour;
	gameObject->behaviour->isServer = true;

	return gameObject;
}

GameObject* ModuleNetworkingServer::spawnSquareOfDeath()
{
	GameObject* gameObject = NetworkInstantiate();
	gameObject->angle = 0;
	gameObject->position = vec2{ 0,0 };
	// Create sprite
	gameObject->sprite = App->modRender->addSprite(gameObject);
	gameObject->sprite->order = 10;
	gameObject->sprite->texture = App->modResources->squareOfDeath;
	gameObject->size = vec2{ 2048,2048 };
	// Create collider
	gameObject->collider = App->modCollision->addCollider(ColliderType::SquareOfDeath, gameObject);
	//gameObject->collider->isTrigger = true;
	// Create behaviour
	SquareOfDeath* squareBehaviour = App->modBehaviour->addSquareOfDeath(gameObject);
	gameObject->behaviour = squareBehaviour;
	gameObject->behaviour->isServer = true;

	return gameObject;
}

//////////////////////////////////////////////////////////////////////
// Update / destruction
//////////////////////////////////////////////////////////////////////

GameObject * ModuleNetworkingServer::instantiateNetworkObject()
{
	// Create an object into the server
	GameObject * gameObject = Instantiate();

	// Register the object into the linking context
	App->modLinkingContext->registerNetworkGameObject(gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			clientProxies[i].repManagerServer.create(gameObject->networkId);
			// TODO(you): World state replication lab session
		}
	}

	return gameObject;
}

void ModuleNetworkingServer::updateNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			clientProxies[i].repManagerServer.update(gameObject->networkId);
		}
	}
}

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			clientProxies[i].repManagerServer.destroy(gameObject->networkId);
		}
	}

	// Assuming the message was received, unregister the network identity
	App->modLinkingContext->unregisterNetworkGameObject(gameObject);

	// Finally, destroy the object from the server
	Destroy(gameObject);
}

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject, float delaySeconds)
{
	uint32 emptyIndex = MAX_GAME_OBJECTS;
	for (uint32 i = 0; i < MAX_GAME_OBJECTS; ++i)
	{
		if (netGameObjectsToDestroyWithDelay[i].object == gameObject)
		{
			float currentDelaySeconds = netGameObjectsToDestroyWithDelay[i].delaySeconds;
			netGameObjectsToDestroyWithDelay[i].delaySeconds = min(currentDelaySeconds, delaySeconds);
			return;
		}
		else if (netGameObjectsToDestroyWithDelay[i].object == nullptr)
		{
			if (emptyIndex == MAX_GAME_OBJECTS)
			{
				emptyIndex = i;
			}
		}
	}

	ASSERT(emptyIndex < MAX_GAME_OBJECTS);

	netGameObjectsToDestroyWithDelay[emptyIndex].object = gameObject;
	netGameObjectsToDestroyWithDelay[emptyIndex].delaySeconds = delaySeconds;
}


//////////////////////////////////////////////////////////////////////
// Global create / update / destruction of network game objects
//////////////////////////////////////////////////////////////////////

GameObject * NetworkInstantiate()
{
	ASSERT(App->modNetServer->isConnected());

	return App->modNetServer->instantiateNetworkObject();
}

void NetworkUpdate(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());
	ASSERT(gameObject->networkId != 0);

	App->modNetServer->updateNetworkObject(gameObject);
}

void NetworkDestroy(GameObject * gameObject)
{
	NetworkDestroy(gameObject, 0.0f);
}

void NetworkDestroy(GameObject * gameObject, float delaySeconds)
{
	ASSERT(App->modNetServer->isConnected());
	ASSERT(gameObject->networkId != 0);

	App->modNetServer->destroyNetworkObject(gameObject, delaySeconds);
}
