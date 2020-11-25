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
			if (repAction == ReplicationAction::Create)
			{
				//object size
				packet >> gameObject->size.x;
				packet >> gameObject->size.y;

				//sprite
				gameObject->sprite = App->modRender->addSprite(gameObject);

				if (gameObject->sprite == nullptr)
					continue;

				int id;
				packet >> id;
				if (id == -1)
					gameObject->sprite->texture = nullptr;
				else
					gameObject->sprite->texture = App->modTextures->GetTextureById(id);
			}
		}
		else
		{
			gameObject = App->modLinkingContext->getNetworkGameObject(netId);
			
		}
	}
}
