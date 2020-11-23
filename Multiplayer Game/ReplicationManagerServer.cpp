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
	packet << PROTOCOL_ID;
	packet << ClientMessage::Input;
	packet << commands.size();
	for (auto iter = commands.begin(); iter != commands.end(); ++iter)
	{
		packet << (*iter).first;
		packet << (*iter).second;
		GameObject* gameObject = App->modLinkingContext->getNetworkGameObject((*iter).first);
		if ((*iter).second == ReplicationAction::Create)
		{
			packet << gameObject->position.x;
			packet << gameObject->position.y;
			packet << gameObject->angle;
		}
		else if ((*iter).second == ReplicationAction::Update)
		{
			packet << gameObject->position.x;
			packet << gameObject->position.y;
			packet << gameObject->angle;
		}
		else if ((*iter).second == ReplicationAction::Destroy)
		{
			commands.erase(iter);
			continue;
		}
		(*iter).second = ReplicationAction::None;
	}
	
}
