#include "ParticleManager.h"
#include "nvToolsExt.h"
#include "ParticleEffect.h"

ParticleManager::ParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		buffers[i] = nullptr;
	}
}

ParticleManager::~ParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		if (buffers[i] != nullptr)
		{
			delete[] buffers[i];
			buffers[i] = nullptr;
		}
	}
}

void ParticleManager::Init(int n, int particlesPerEffectCount, int particleLifetime, float gravity, float particleMaxInitialVelocity, float emitOnDestroyProbability)
{
	effectsCount = n;
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
	nvtxRangePush(__FUNCTION__);

	ParticleBuffer* buffer = buffers[currentUpdatingBufferId];
	int last = buffer->lastParticleId;

	for (int index = first; index < end; index++)
	{
		int i = (last + index) % effectsCount;

		if (buffer->effects[i].birthTime > 0 && buffer->effects[i].birthTime + particleLifetime < time)
		{
			{ //lock_guard scope
				std::lock_guard<std::mutex> lock(emitMutex);
				buffer->effects[i].Destroy(std::bind(&ParticleManager::Emit, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), time);
				buffer->lastParticleId = (buffer->lastParticleId + 1) % effectsCount;
				buffer->activeParticlesCount--;
			}
			continue;
		}

		buffer->effects[i].Update(dt);
	}

	nvtxRangePop();
}

void ParticleManager::Render()
{
	ParticleBuffer* buffer = buffers[renderBufferId];
	for (int index = 0; index < buffer->activeParticlesCount; index++)
	{
		int i = (buffer->lastParticleId + index) % effectsCount;
		buffer->effects[i].Render();
	}
}

void ParticleManager::Emit(int x, int y, float time)
{
	ParticleBuffer* buffer = buffers[currentUpdatingBufferId];
	buffer->effects[buffer->nextDeadParticleId].Emit(x, y, time);

	buffer->nextDeadParticleId = (buffer->nextDeadParticleId + 1) % effectsCount;

	if (buffer->nextDeadParticleId == buffer->lastParticleId)
		buffer->lastParticleId = (buffer->lastParticleId + 1) % effectsCount;
	else
		buffer->activeParticlesCount++;
}

void ParticleManager::SwapRenderBuffer()
{
	std::lock_guard<std::mutex> lock(swapMutex);

	if (renderBufferId == lastUpdatedBufferId)
		return;

	nextToUpdateBufferId = renderBufferId;
		renderBufferId = lastUpdatedBufferId;
}

void ParticleManager::SwapUpdateBuffer()
{
	std::lock_guard<std::mutex> lock(swapMutex);
	
	lastUpdatedBufferId = currentUpdatingBufferId;
	currentUpdatingBufferId = nextToUpdateBufferId;
	nextToUpdateBufferId = lastUpdatedBufferId;

	std::memcpy(buffers[currentUpdatingBufferId], buffers[lastUpdatedBufferId], sizeof *buffers[lastUpdatedBufferId]);
}
