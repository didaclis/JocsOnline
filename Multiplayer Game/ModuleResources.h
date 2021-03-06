#pragma once

#define USE_TASK_MANAGER

struct Texture;

class ModuleResources : public Module
{
public:

	Texture *background = nullptr;
	Texture *space = nullptr;
	Texture *asteroid1 = nullptr;
	Texture *asteroid2 = nullptr;
	Texture *spacecraft1 = nullptr;
	Texture *spacecraft2 = nullptr;
	Texture *spacecraft3 = nullptr;
	Texture *laser = nullptr;
	Texture *explosion1 = nullptr;
	Texture* powerup01 = nullptr;
	Texture* laserBig = nullptr;
	Texture* powerupWeapon = nullptr;
	Texture* bomb = nullptr;
	Texture* squareOfDeath = nullptr;
	Texture* powerupBomb = nullptr;
	Texture* beginText = nullptr;
	Texture* winText = nullptr;
	AnimationClip *explosionClip = nullptr;

	AudioClip *audioClipLaser = nullptr;
	AudioClip *audioClipExplosion = nullptr;

	bool finishedLoading = false;

private:

	bool init() override;

#if defined(USE_TASK_MANAGER)
	
	class TaskLoadTexture : public Task
	{
	public:

		const char *filename = nullptr;
		Texture **texture = nullptr;

		void execute() override;
	};

	static const int MAX_RESOURCES = 18;
	TaskLoadTexture tasks[MAX_RESOURCES] = {};
	uint32 taskCount = 0;
	uint32 finishedTaskCount = 0;

	void onTaskFinished(Task *task) override;

	void loadTextureAsync(const char *filename, Texture **texturePtrAddress);

#endif

};

