#include "ParticleManager.h"
#include "./nvToolsExt.h"
#include <iostream>

ParticleManager::ParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		buffers[i] = nullptr;
	}

	m_num_particles = 0;
}

ParticleManager::~ParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		if (buffers[i])
		{
			delete[] buffers[i];
			buffers[i] = nullptr;
		}
	}
}

void ParticleManager::Init(int n, int particlesPerEffectCount, int particleLifetime, float gravity, float particleMaxInitialVelocity, float emitOnDestroyProbability)
{
	m_num_particles = n;
	this->particleLifetime = particleLifetime;

	for (int i = 0; i < 3; ++i)
	{
		buffers[i] = new ParticleBuffer();
		buffers[i]->effects = new ParticleEffect[n];

		for (int j = 0; j < n; j++)
		{
			buffers[i]->effects[j].Init(particlesPerEffectCount, gravity, particleMaxInitialVelocity, emitOnDestroyProbability);
		}
	}
}


void ParticleManager::Update(int first, int end, float dt, float time)
{
	//	std::lock_guard<std::mutex> lock(mutex_);

	nvtxRangePush(__FUNCTION__);

	ParticleBuffer* buffer = buffers[currentUpdatingBufferId];
	int last = buffer->lastParticleId;

	for (int index = first; index < end; index++)
	{
		int i = (last + index) % m_num_particles;

		if (buffer->effects[i].birthTime > 0 && buffer->effects[i].birthTime + particleLifetime < time)
		{
			buffer->effects[i].Destroy(std::bind(&ParticleManager::Emit, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), time);
			buffer->lastParticleId = (buffer->lastParticleId + 1) % m_num_particles;
			buffer->activeParticlesCount--;
			continue;
		}

		buffer->effects[i].Update(buffers[lastUpdatedBufferId]->effects[i], dt);
	}

	nvtxRangePop();
}

void ParticleManager::Render()
{
	ParticleBuffer* buffer = buffers[renderBufferId];
	for (int index = 0; index < buffer->activeParticlesCount; index++)
	{
		int i = (buffer->lastParticleId + index) % m_num_particles;
		buffer->effects[i].Render();
	}
}

void ParticleManager::Emit(int x, int y, float time)
{
	std::lock_guard<std::mutex> lock(mutex_);	

	ParticleBuffer* buffer = buffers[currentUpdatingBufferId];
	buffer->effects[buffer->nextDeadParticleId].Emit(x, y, time);

	buffer->nextDeadParticleId = (buffer->nextDeadParticleId + 1) % m_num_particles;

	if (buffer->nextDeadParticleId == buffer->lastParticleId)
		buffer->lastParticleId = (buffer->lastParticleId + 1) % m_num_particles;
	else
		buffer->activeParticlesCount++;

//	std::cout << buffer->activeParticlesCount << "\n";
}

void ParticleManager::SwapRenderBuffer()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (renderBufferId == lastUpdatedBufferId)
		return;

	nextToUpdateBufferId = renderBufferId;
		renderBufferId = lastUpdatedBufferId;

//	std::cout << "render: " << renderBufferId << "\tlast updated: " << lastUpdatedBufferId << "\tcurrent: " << currentUpdatingBufferId << "\tnext: " << nextToUpdateBufferId << " render\n";
}

void ParticleManager::SwapUpdateBuffer()
{
	std::lock_guard<std::mutex> lock(mutex_);
	
	lastUpdatedBufferId = currentUpdatingBufferId;
	currentUpdatingBufferId = nextToUpdateBufferId;
	nextToUpdateBufferId = lastUpdatedBufferId;

	std::memcpy(buffers[currentUpdatingBufferId], buffers[lastUpdatedBufferId], sizeof *buffers[lastUpdatedBufferId]);
}
