#pragma once

#include<mutex>

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
	void updateOffscreen(int i);
	bool removeOld(int& i);
	void Update(int first, int end, float dt, float time);
	void Render();
	void Emit(int x, int y, float time);
	void SwapRenderBuffer();
	void SwapUpdateBuffer();
	//	private:
	int			m_num_particles;

	ParticleBuffer* buffers[3];
	int renderBufferId = 0;
	int lastUpdatedBufferId = 1;
	int currentUpdatingBufferId = 2;
	int nextToUpdateBufferId = 1;

	int particleLifetime;

	std::mutex mutex_;
};

