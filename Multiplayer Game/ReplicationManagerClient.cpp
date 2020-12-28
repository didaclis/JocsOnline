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

		if (repAction == ReplicationAction::None)
			continue;

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
				int bType;
				packet >> bType;

				if ((BehaviourType)bType == BehaviourType::Laser)
				{
					App->modSound->playAudioClip(App->modResources->audioClipLaser);
				}

				gameObject->behaviour = App->modBehaviour->addBehaviour((BehaviourType)bType, gameObject);

				//object size
				packet >> gameObject->size.x;
				packet >> gameObject->size.y;

				//sprite
				gameObject->sprite = App->modRender->addSprite(gameObject);

				if (gameObject->sprite == nullptr)
					continue;

				std::string name;
				packet >> name;
				if (strcmp(name.c_str(), " ") == 0)
					gameObject->sprite->texture = nullptr;
				else
					gameObject->sprite->texture = App->modTextures->GetTextureByName(name);

				int clipID;
				packet >> clipID;
				if (clipID == 0)
				{
					gameObject->animation = App->modRender->addAnimation(gameObject);
					if (gameObject->animation != nullptr)
					{
						gameObject->animation->clip = App->modResources->explosionClip;
						App->modSound->playAudioClip(App->modResources->audioClipExplosion);
					}
				}
			}

			if (repAction == ReplicationAction::Update)
			{
				if (gameObject->behaviour != nullptr)
				{
					if (gameObject->behaviour->type() == BehaviourType::Spaceship)
						gameObject->behaviour->read(packet);
				}
			}

		}
		else
		{
			gameObject = App->modLinkingContext->getNetworkGameObject(netId);
			App->modLinkingContext->unregisterNetworkGameObject(gameObject);
			Destroy(gameObject);
		}
	}
}
