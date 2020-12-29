#include "Networks.h"
#include "ReplicationManagerServer.h"
#include "ModuleNetworkingServer.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	commands.emplace(networkId, ReplicationAction::Create);
}

void ReplicationManagerServer::update(uint32 networkId)
{
	commands[networkId] = ReplicationAction::Update;
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	commands[networkId] = ReplicationAction::Destroy;
}

void ReplicationManagerServer::write(OutputMemoryStream& packet)
{	
	packet << commands.size();
	for (auto iter = commands.begin(); iter != commands.end();)
	{
		packet << (*iter).first;
		packet << (*iter).second;
		GameObject* gameObject = App->modLinkingContext->getNetworkGameObject((*iter).first);
		
		if ((*iter).second == ReplicationAction::None)
		{
			++iter;
			continue;
		}
		if ((*iter).second == ReplicationAction::Destroy)
		{
			iter = commands.erase(iter);
			continue;
		}

		packet << gameObject->position.x;
		packet << gameObject->position.y;
		packet << gameObject->angle;

		if ((*iter).second == ReplicationAction::Create)
		{
			if (gameObject->behaviour != nullptr)
				packet << (int)gameObject->behaviour->type();
			else
				packet << 0;
			packet << gameObject->size.x;
			packet << gameObject->size.y;
			if (gameObject->sprite != nullptr && gameObject->sprite->texture != nullptr)
			{
				std::string name = gameObject->sprite->texture->filename;
				packet << name;
			}
			else
			{
				std::string name = " ";
				packet << name;
			}

			if (gameObject->animation != nullptr && gameObject->animation->clip != nullptr)
				packet << (int)gameObject->animation->clip->id;
			else
				packet << -1;
		}	

		if ((*iter).second == ReplicationAction::Update)
		{
			if (gameObject->behaviour != nullptr)
				if (gameObject->behaviour->type() == BehaviourType::Spaceship)
					gameObject->behaviour->write(packet);

		}

		(*iter).second = ReplicationAction::None;
		++iter;

	}
	
}
