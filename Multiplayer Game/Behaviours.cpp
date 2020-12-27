#include "Networks.h"
#include "Behaviours.h"



void Laser::start()
{
	gameObject->networkInterpolationEnabled = false;

	App->modSound->playAudioClip(App->modResources->audioClipLaser);
}

void Laser::update()
{
	secondsSinceCreation += Time.deltaTime;

	const float pixelsPerSecond = 1000.0f;
	gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;

	if (isServer)
	{
		const float neutralTimeSeconds = 0.1f;
		if (secondsSinceCreation > neutralTimeSeconds && gameObject->collider == nullptr) {
			gameObject->collider = App->modCollision->addCollider(ColliderType::Laser, gameObject);
		}

		const float lifetimeSeconds = 2.0f;
		if (secondsSinceCreation >= lifetimeSeconds) {
			NetworkDestroy(gameObject);
		}
	}
}





void Spaceship::start()
{
	gameObject->tag = (uint32)(Random.next() * UINT_MAX);

	lifebar = Instantiate();
	lifebar->sprite = App->modRender->addSprite(lifebar);
	lifebar->sprite->pivot = vec2{ 0.0f, 0.5f };
	lifebar->sprite->order = 5;
}

void Spaceship::onInput(const InputController &input)
{
	if (input.horizontalAxis != 0.0f)
	{
		const float rotateSpeed = 180.0f;
		gameObject->angle += input.horizontalAxis * rotateSpeed * Time.deltaTime;

		if (isServer)
		{
			NetworkUpdate(gameObject);
		}
	}

	if (input.actionDown == ButtonState::Pressed)
	{
		
		gameObject->position += vec2FromDegrees(gameObject->angle) * advanceSpeed * Time.deltaTime;

		if (isServer)
		{
			NetworkUpdate(gameObject);
		}
	}

	if (input.actionLeft == ButtonState::Press)
	{
		if (isServer)
		{
			GameObject *laser = NetworkInstantiate();

			laser->position = gameObject->position;
			laser->angle = gameObject->angle;
			laser->size = { 20, 60 };

			laser->sprite = App->modRender->addSprite(laser);
			laser->sprite->order = 3;
			

			Laser *laserBehaviour = App->modBehaviour->addLaser(laser);
			laserBehaviour->isServer = isServer;
			laserBehaviour->l_type = currentLaser;
			switch (laserBehaviour->l_type)
			{
			case Laser::LaserType::BIG:
				laser->sprite->texture = App->modResources->laserBig;
				break;
			case Laser::LaserType::NORMAL:
				laser->sprite->texture = App->modResources->laser;
				break;
			}
			laser->tag = gameObject->tag;
		}
	}
}

void Spaceship::update()
{
	static const vec4 colorAlive = vec4{ 0.2f, 1.0f, 0.1f, 0.5f };
	static const vec4 colorDead = vec4{ 1.0f, 0.2f, 0.1f, 0.5f };
	const float lifeRatio = max(0.01f, (float)(hitPoints) / (MAX_HIT_POINTS));
	lifebar->position = gameObject->position + vec2{ -50.0f, -50.0f };
	lifebar->size = vec2{ lifeRatio * 80.0f, 5.0f };
	lifebar->sprite->color = lerp(colorDead, colorAlive, lifeRatio);

	if (bombing)
		manageBombs();
}

void Spaceship::destroy()
{
	Destroy(lifebar);
}

void Spaceship::onCollisionTriggered(Collider &c1, Collider &c2)
{

	if (c2.type == ColliderType::Laser && c2.gameObject->tag != gameObject->tag)
	{
		if (isServer)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser
		
			if (hitPoints > 0)
			{
				hitPoints--;
				NetworkUpdate(gameObject);
			}

			float size = 30 + 50.0f * Random.next();
			vec2 position = gameObject->position + 50.0f * vec2{Random.next() - 0.5f, Random.next() - 0.5f};

			if (hitPoints <= 0)
			{
				// Centered big explosion
				size = 250.0f + 100.0f * Random.next();
				position = gameObject->position;

				NetworkDestroy(gameObject);
			}

			GameObject *explosion = NetworkInstantiate();
			explosion->position = position;
			explosion->size = vec2{ size, size };
			explosion->angle = 365.0f * Random.next();

			explosion->sprite = App->modRender->addSprite(explosion);
			explosion->sprite->texture = App->modResources->explosion1;
			explosion->sprite->order = 100;

			explosion->animation = App->modRender->addAnimation(explosion);
			explosion->animation->clip = App->modResources->explosionClip;

			NetworkDestroy(explosion, 2.0f);

			// NOTE(jesus): Only played in the server right now...
			// You need to somehow make this happen in clients
			App->modSound->playAudioClip(App->modResources->audioClipExplosion);
		}
	}
	if (c2.type == ColliderType::PowerUp && c2.gameObject->tag != gameObject->tag)
	{
		if (isServer)
		{
			NetworkDestroy(c2.gameObject);
			dynamic_cast<PowerUp*>(c2.gameObject->behaviour)->managePowerUp(gameObject);
		}
	}

	if (c2.type == ColliderType::Bomb && c2.gameObject->tag != gameObject->tag)
	{
		if (isServer)
		{
			NetworkDestroy(c2.gameObject);

			if (hitPoints > 0)
			{
				hitPoints--;
				NetworkUpdate(gameObject);
			}
			if (hitPoints <= 0)
			{
				// Centered big explosion
				//size = 250.0f + 100.0f * Random.next();
				//position = gameObject->position;

				NetworkDestroy(gameObject);
			}
		}
	}

	if (c2.type == ColliderType::Asteroid)
	{
		if (isServer)
		{
			NetworkDestroy(c2.gameObject);
			if (hitPoints > 0)
			{
				hitPoints--;
				NetworkUpdate(gameObject);
			}

			//float size = 30 + 50.0f * Random.next();
			//vec2 position = gameObject->position + 50.0f * vec2{ Random.next() - 0.5f, Random.next() - 0.5f };

			//if (hitPoints <= 0)
			//{
			//	// Centered big explosion
			//	size = 250.0f + 100.0f * Random.next();
			//	position = gameObject->position;

			//	NetworkDestroy(gameObject);
			//}

			//GameObject* explosion = NetworkInstantiate();
			//explosion->position = position;
			//explosion->size = vec2{ size, size };
			//explosion->angle = 365.0f * Random.next();

			//explosion->sprite = App->modRender->addSprite(explosion);
			//explosion->sprite->texture = App->modResources->explosion1;
			//explosion->sprite->order = 100;

			//explosion->animation = App->modRender->addAnimation(explosion);
			//explosion->animation->clip = App->modResources->explosionClip;

			//NetworkDestroy(explosion, 2.0f);

			// NOTE(jesus): Only played in the server right now...
			// You need to somehow make this happen in clients
			App->modSound->playAudioClip(App->modResources->audioClipExplosion);
		}
	}
}

void Spaceship::onNotCollisionTriggered(Collider& c1, Collider& c2)
{
	if (c2.type == ColliderType::SquareOfDeath)
	{
		hitPoints -= 0.1;
		if (hitPoints <= 0)
		{
			NetworkDestroy(gameObject);
		}
	}
}

void Spaceship::manageBombs()
{
	if (Time.time - timer > 2)
	{
		timer = Time.time;
		if (isServer)
		{
			GameObject* bomb = NetworkInstantiate();

			bomb->position = gameObject->position;
			bomb->angle = gameObject->angle;
			//bomb->size = { 20, 60 };

			bomb->sprite = App->modRender->addSprite(bomb);
			bomb->sprite->order = 3;
			bomb->collider = App->modCollision->addCollider(ColliderType::Bomb, bomb);

			Bomb* bombBehaviour = App->modBehaviour->addBomb(bomb);
			bombBehaviour->isServer = isServer;
			bomb->sprite->texture = App->modResources->bomb;
			bomb->tag = gameObject->tag;
		}
	}
}


void Spaceship::write(OutputMemoryStream & packet)
{
	packet << hitPoints;
}

void Spaceship::read(const InputMemoryStream & packet)
{
	packet >> hitPoints;
}

void PowerUp::start()
{
	gameObject->tag = (uint32)(Random.next() * UINT_MAX);
}

void PowerUp::update()
{
}

void PowerUp::destroy()
{
}

void PowerUp::managePowerUp(GameObject *gO)
{
	switch (p_type)
	{
	case PowerUpType::WEAPON:
		dynamic_cast<Spaceship*>(gO->behaviour)->currentLaser = Laser::LaserType::BIG;
		break;
	case PowerUpType::BOMB:
		dynamic_cast<Spaceship*>(gO->behaviour)->bombing = true;
		dynamic_cast<Spaceship*>(gO->behaviour)->timer = Time.time;
		break;
	case PowerUpType::SPEED:
		dynamic_cast<Spaceship*>(gO->behaviour)->advanceSpeed = 400;
		break;
	}
}

void Asteroid::start()
{
}

void Asteroid::update()
{
	gameObject->angle += 0.1;
}

void Asteroid::destroy()
{
}

void Asteroid::onCollisionTriggered(Collider& c1, Collider& c2)
{
	if (c2.type == ColliderType::Laser && c2.gameObject->tag != gameObject->tag)
	{
		if (isServer)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser
			NetworkDestroy(gameObject); // Destroy the laser
		}
	}
}

void SquareOfDeath::start()
{
}

void SquareOfDeath::update()
{
	gameObject->size -= vec2{ 0.1,0.1 };
}

void Bomb::start()
{
}

void Bomb::update()
{
	secondsSinceCreation += Time.deltaTime;
	if (isServer)
	{
		if (secondsSinceCreation > 5.0f)
		{
			NetworkDestroy(gameObject);
		}
	}
}
