#pragma once


enum class BehaviourType : uint8;

struct Behaviour
{
	GameObject *gameObject = nullptr;
	bool isServer = false;
	bool isLocalPlayer = false;

	virtual BehaviourType type() const = 0;

	virtual void start() { }

	virtual void onInput(const InputController &input) { }

	virtual void update() { }

	virtual void destroy() { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }

	virtual void write(OutputMemoryStream &packet) { }

	virtual void read(const InputMemoryStream &packet) { }
};


enum class BehaviourType : uint8
{
	None,
	Spaceship,
	PowerUp,
	Laser,
	Asteroid,
};

struct Asteroid : public Behaviour
{
	BehaviourType type() const override { return BehaviourType::Asteroid; }

	void start() override;

	void update() override;

	void destroy() override;

	void onCollisionTriggered(Collider& c1, Collider& c2) override;
};

struct PowerUp : public Behaviour
{
	enum class PowerUpType
	{
		SPEED,
		WEAPON,
		BOMB,
	};
	PowerUpType p_type;
	float secondsSinceCreation = 0.0f;

	BehaviourType type() const override { return BehaviourType::PowerUp; }

	void start() override;

	void update() override;

	void destroy() override;

	void managePowerUp(GameObject *gO);
};

struct Laser : public Behaviour
{
	enum class LaserType
	{
		NORMAL,
		BIG,
		BURST,
	};
	LaserType l_type = LaserType::NORMAL;
	float secondsSinceCreation = 0.0f;

	BehaviourType type() const override { return BehaviourType::Laser; }

	void start() override;

	void update() override;
};


struct Spaceship : public Behaviour
{
	Laser::LaserType currentLaser = Laser::LaserType::NORMAL;
	static const uint8 MAX_HIT_POINTS = 5;
	uint8 hitPoints = MAX_HIT_POINTS;
	float advanceSpeed = 200.0f;

	GameObject *lifebar = nullptr;

	BehaviourType type() const override { return BehaviourType::Spaceship; }

	void start() override;

	void onInput(const InputController &input) override;

	void update() override;

	void destroy() override;

	void onCollisionTriggered(Collider &c1, Collider &c2) override;

	

	void write(OutputMemoryStream &packet) override;

	void read(const InputMemoryStream &packet) override;
};
