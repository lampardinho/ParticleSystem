#pragma once

#include <mutex>

class ParticleEffect;

class ParticleBuffer
{
public:
	ParticleEffect* effects;
	int lastParticleId = 0;
	int nextDeadParticleId = 0;
	int activeParticlesCount = 0;
};

class	ParticleManager
{
public:
	ParticleManager();
	~ParticleManager();
	void Init(int n, int particlesPerEffectCount, int particleLifetime, float gravity, float particleMaxInitialVelocity, float emitOnDestroyProbability);
	void Update(int first, int end, float dt, float time);
	void Render();
	void Emit(int x, int y, float time);
	void SwapRenderBuffer();
	void SwapUpdateBuffer();

	ParticleBuffer* buffers[3];
	std::mutex swapMutex;
	std::mutex emitMutex;
	int currentUpdatingBufferId = 2;

private:
	int	effectsCount = 0;
	int renderBufferId = 0;
	int lastUpdatedBufferId = 1;
	int nextToUpdateBufferId = 1;
	int particleLifetime = 0;
};

