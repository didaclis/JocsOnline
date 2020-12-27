#include "Networks.h"
#include "ModuleBehaviour.h"

bool ModuleBehaviour::update()
{
	for (Spaceship &behaviour : spaceships)
	{
		handleBehaviourLifeCycle(&behaviour);
	}
	
	for (Laser &behaviour : lasers)
	{
		handleBehaviourLifeCycle(&behaviour);
	}

	for (PowerUp& behaviour : powerups)
	{
		handleBehaviourLifeCycle(&behaviour);
	}

	for (Asteroid& behaviour : asteroids)
	{
		handleBehaviourLifeCycle(&behaviour);
	}
	for (SquareOfDeath& behaviour : squareofdeath)
	{
		handleBehaviourLifeCycle(&behaviour);
	}

	for (Bomb& behaviour : bombs)
	{
		handleBehaviourLifeCycle(&behaviour);
	}
	return true;
}

Behaviour *ModuleBehaviour::addBehaviour(BehaviourType behaviourType, GameObject *parentGameObject)
{
	switch (behaviourType)
	{
	case BehaviourType::Spaceship:
		return addSpaceship(parentGameObject);
	case BehaviourType::Laser:
		return addLaser(parentGameObject);
	case BehaviourType::PowerUp:
		return addPowerUp(parentGameObject);
	case BehaviourType::Asteroid:
		return addAsteroid(parentGameObject);
	case BehaviourType::SquareOfDeath:
		return addSquareOfDeath(parentGameObject);
	default:
		return nullptr;
	}
}

Spaceship *ModuleBehaviour::addSpaceship(GameObject *parentGameObject)
{
	for (Spaceship &behaviour : spaceships)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGameObject;
			parentGameObject->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}

Laser *ModuleBehaviour::addLaser(GameObject *parentGameObject)
{
	for (Laser &behaviour : lasers)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGameObject;
			parentGameObject->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}

PowerUp* ModuleBehaviour::addPowerUp(GameObject* parentGameObject)
{
	for (PowerUp& behaviour : powerups)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGameObject;
			parentGameObject->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}

Asteroid* ModuleBehaviour::addAsteroid(GameObject* parentGameObject)
{
	for (Asteroid& behaviour : asteroids)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGameObject;
			parentGameObject->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}

SquareOfDeath* ModuleBehaviour::addSquareOfDeath(GameObject* parentGameObject)
{
	for (SquareOfDeath& behaviour : squareofdeath)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGameObject;
			parentGameObject->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}

Bomb* ModuleBehaviour::addBomb(GameObject* parentGameObject)
{
	for (Bomb& behaviour : bombs)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGameObject;
			parentGameObject->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}


void ModuleBehaviour::handleBehaviourLifeCycle(Behaviour *behaviour)
{
	GameObject *gameObject = behaviour->gameObject;

	if (gameObject != nullptr)
	{
		switch (gameObject->state)
		{
		case GameObject::STARTING:
			behaviour->start();
			break;
		case GameObject::UPDATING:
			behaviour->update();
			break;
		case GameObject::DESTROYING:
			behaviour->destroy();
			gameObject->behaviour = nullptr;
			behaviour->gameObject = nullptr;
			break;
		default:;
		}
	}
}
