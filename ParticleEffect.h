#pragma once

#include "vector2.h"
#include <functional>

class Particle
{
public:
	void Update(float time, float gravity);
	
	Vector2	position;
	Vector2	velocity;
	bool isVisible = true;
};

class ParticleEffect
{
public:
	~ParticleEffect();
	void Update(float time);
	void Init(int n, float gravity, float particleMaxInitialVelocity, float emitOnDestroyProbability);
	void Render();
	void Emit(int x, int y, float time);
	void Destroy(std::function<void(int, int, float)> emitFunc, float time);

	Particle* particles;
	float birthTime;

private:
	int count;
	float gravity;
	int particleMaxInitialVelocity;
	float emitOnDestroyProbability;
};