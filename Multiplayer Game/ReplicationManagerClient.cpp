#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream& packet)
{
	GameObject* gameObject = nullptr;
	size_t obj_size;
	packet >> obj_size;
	for (int i = 0; i < obj_size; ++i)
	{
		uint32 netId;
		packet >> netId;
		
		ReplicationAction repAction;
		packet >> repAction;
		if (repAction != ReplicationAction::Destroy)
		{
			if (repAction == ReplicationAction::Create)
			{
				gameObject = App->modGameObject->Instantiate();
				App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, netId);
			}
			else if (repAction == ReplicationAction::Update)
			{
				gameObject = App->modLinkingContext->getNetworkGameObject(netId);
			}
			packet >> gameObject->position.x;
			packet >> gameObject->position.y;
			packet >> gameObject->angle;
		}
		else
		{
			gameObject = App->modLinkingContext->getNetworkGameObject(netId);
			
		}
	}
}
