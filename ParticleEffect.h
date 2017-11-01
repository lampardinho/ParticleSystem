#pragma once

#include "vector2.h"
#include <functional>

//#include "ParticleManager.h"

class Particle
{

public:
	Vector2	m_pos;
	Vector2	m_vel;
	bool isVisible = true;
	void	Update(float time, float gravity);
};

class ParticleEffect
{
	int count;
	
	float gravity;
	int particleMaxInitialVelocity; 
	float emitOnDestroyProbability;

public:
	Particle* particles;
	float birthTime;

	~ParticleEffect();
	void Update(const ParticleEffect& lastState, float time);
	void Init(int n, float gravity, float particleMaxInitialVelocity, float emitOnDestroyProbability);
	void Render();
	void Emit(int x, int y, float time);
	void Destroy(std::function<void(int, int, float)> emitFunc, float time);
};