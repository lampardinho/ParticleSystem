
#include <math.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "./nvToolsExt.h"

#include "test.h"
#include "vector2.h"
#include <iostream>
#include <algorithm>
#include <list>
#include "CircularBuffer.h"
#include <condition_variable>


// Just some hints on implementation
// You could remove all of them

static std::atomic<float> globalTime;
static std::atomic<int> renderCount;
static volatile bool workerMustExit = false;



struct GlobalSettings 
{
	int effectsCount;
	int particlesPerEffectCount;
	int particleMaxInitialVelocity;
	float particleLifetime;
	float gravity;
	float emitOnDestroyProbability;
};

GlobalSettings g_CurrentSettings;

void InitializeSettings()
{
	g_CurrentSettings.effectsCount = 2048;
	g_CurrentSettings.particlesPerEffectCount = 64;
	g_CurrentSettings.particleMaxInitialVelocity = 100;
	g_CurrentSettings.particleLifetime = 2000;
	g_CurrentSettings.gravity = 1;
	g_CurrentSettings.emitOnDestroyProbability = 0.5;
}


// some code
class Particle
{

public:
	Vector2	m_pos;
	Vector2	m_vel;
	bool isVisible = true;
	void	Update(const Particle& lastState, float time);
};

void Particle::Update(const Particle& lastState, float time)
{
	if (lastState.m_pos.x < 0 || lastState.m_pos.x > test::SCREEN_WIDTH)
		isVisible = false;

	if (lastState.m_pos.y < 0 || lastState.m_pos.y > test::SCREEN_HEIGHT)
		isVisible = false;

	if (!lastState.isVisible)
		return;

	Vector2 pos2 = lastState.m_pos + lastState.m_vel * time;
	m_vel = lastState.m_vel - Vector2(0.0f, g_CurrentSettings.gravity);
	m_pos = pos2;
}








class ParticleEffect
{

public:
	Particle* particles;
	int count;
	float birthTime;

	~ParticleEffect();
	void Update(const ParticleEffect& lastState, float time);
	void Init(int n);
	void Render();
	void Emit(int x, int y);
	void Destroy();
};





class ParticleBuffer
{
public:
	ParticleEffect* effects;
	int lastParticleId = 0;
	int nextDeadParticleId = 0;

	int activeParticlesCount = 0;
};





class	CParticleManager
{
public:
	CParticleManager();
	~CParticleManager();
	void Init(int n);
	void updateOffscreen(int i);
	bool removeOld(int& i);
	void Update(float time);
	void Render();
	void Emit(int x, int y);
	void SwapRenderBuffer();
	void SwapUpdateBuffer();
//	private:
	int			m_num_particles;

	ParticleBuffer* buffers[3];
	int renderBufferId = 0;
	int lastUpdatedBufferId = 0;
	int currentUpdatingBufferId = 1;

//	int lastParticleId;
//	int nextDeadParticleId;
//
//	int activeParticlesCount;
	bool rendered = false;
	std::condition_variable cond_var;
	std::mutex mutex_;
};

CParticleManager	g_ParticleManager;


CParticleManager::CParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		buffers[i] = nullptr;
	}
	
	m_num_particles = 0;

	
}

CParticleManager::~CParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		if (buffers[i])
			delete[] buffers[i];
	}
}

void CParticleManager::Init(int n)
{
	m_num_particles = n;

	for (int i = 0; i < 3; ++i)
	{
		buffers[i] = new ParticleBuffer();
		buffers[i]->effects = new ParticleEffect[n];
		
		for (int j = 0; j < n; j++)
		{
			buffers[i]->effects[j].Init(g_CurrentSettings.particlesPerEffectCount);
		}
	}
}


void CParticleManager::Update(float time)
{
//	std::lock_guard<std::mutex> lock(mutex_);

	nvtxRangePush(__FUNCTION__);

	ParticleBuffer* buffer = buffers[currentUpdatingBufferId];
	int count = buffer->activeParticlesCount;
	int last = buffer->lastParticleId;
	for (int index = 0; index < count; index++)
	{
		int i = (last + index) % m_num_particles;

		float t = globalTime.load();
		if (buffer->effects[i].birthTime > 0 && buffer->effects[i].birthTime + g_CurrentSettings.particleLifetime < t)
		{
			buffer->effects[i].Destroy();
			buffer->lastParticleId = (buffer->lastParticleId + 1) % m_num_particles;
			buffer->activeParticlesCount--;
			continue;
		}

		buffer->effects[i].Update(buffers[lastUpdatedBufferId]->effects[i], time);
	}

	nvtxRangePop();
}

void CParticleManager::Render()
{
//	std::lock_guard<std::mutex> lock(mutex_);

	ParticleBuffer* buffer = buffers[renderBufferId];
	for (int index = 0; index < buffer->activeParticlesCount; index++)
	{
		int i = (buffer->lastParticleId + index) % m_num_particles;
		buffer->effects[i].Render();
	}

	renderCount++;	
}

void CParticleManager::Emit(int x, int y)
{
	std::cout << currentUpdatingBufferId << "\n";
//
//	if (activeParticlesCount > m_num_particles)
//		std::cout << "alert\n";

//	std::lock_guard<std::mutex> lock(mutex_);

	ParticleBuffer* buffer = buffers[currentUpdatingBufferId];
	buffer->effects[buffer->nextDeadParticleId].Emit(x, y);

	buffer->nextDeadParticleId = (buffer->nextDeadParticleId + 1) % m_num_particles;

	if (buffer->nextDeadParticleId == buffer->lastParticleId)
		buffer->lastParticleId = (buffer->lastParticleId + 1) % m_num_particles;
	else
		buffer->activeParticlesCount++;



	buffer = buffers[lastUpdatedBufferId];
	buffer->effects[buffer->nextDeadParticleId].Emit(x, y);

	buffer->nextDeadParticleId = (buffer->nextDeadParticleId + 1) % m_num_particles;

	if (buffer->nextDeadParticleId == buffer->lastParticleId)
		buffer->lastParticleId = (buffer->lastParticleId + 1) % m_num_particles;
	else
		buffer->activeParticlesCount++;
}

void CParticleManager::SwapRenderBuffer()
{
//	std::lock_guard<std::mutex> lock(mutex_);
//
//	std::swap(renderBufferId, lastUpdatedBufferId);
}

void CParticleManager::SwapUpdateBuffer()
{
	std::swap(currentUpdatingBufferId, lastUpdatedBufferId);
	std::cout << currentUpdatingBufferId << " " << renderBufferId << " " << lastUpdatedBufferId << "\n";
}







ParticleEffect::~ParticleEffect()
{
	if (particles)
		delete[] particles; //exception here
}

void ParticleEffect::Init(int n)
{
	particles = new Particle[n];
	count = n;
	birthTime = -1;

	for (int i = 0; i < count; i++)
	{
		particles[i].isVisible = true;
		particles[i].m_pos = Vector2(0, 0);
		particles[i].m_vel = Vector2(0, 0);
	}
}

void ParticleEffect::Update(const ParticleEffect& lastState, float time)
{
	for (int i = 0; i < count; ++i)
	{
		particles[i].Update(lastState.particles[i], time);
	}
}

void ParticleEffect::Render()
{
	for (int i = 0; i < count; i++)
	{
		if (particles[i].isVisible)
			platform::drawPoint(particles[i].m_pos.x, particles[i].m_pos.y, 1, 1, 1, 1);
	}
}

void ParticleEffect::Emit(int x, int y)
{
	birthTime = globalTime.load();

	for (int i = 0; i < count; i++)
	{
		particles[i].isVisible = true;
		particles[i].m_pos = Vector2(x, y);
		particles[i].m_vel = Vector2(rand() % 101 - 50, rand() % 101 - 50);
		particles[i].m_vel = particles[i].m_vel.Normal();
		particles[i].m_vel = particles[i].m_vel * (rand() % g_CurrentSettings.particleMaxInitialVelocity);
	}
}

void ParticleEffect::Destroy()
{
	birthTime = -1;

	for (int i = 0; i < count; i++)
	{
		if (!particles[i].isVisible)
			continue;

		if (rand() % 101 < g_CurrentSettings.emitOnDestroyProbability * 100)
			g_ParticleManager.Emit(particles[i].m_pos.x, particles[i].m_pos.y);
	}
}








void WorkerThread(void)
{
	while (!workerMustExit)
	{
		nvtxRangePush(__FUNCTION__);

		static float lastTime = 0.f;
		float time = globalTime.load();
		float delta = time - lastTime;
		lastTime = time;

		// some code
//		std::cout << delta << "\n";
		g_ParticleManager.Update(delta / 1000.f);

		std::unique_lock<std::mutex> lock(g_ParticleManager.mutex_);
		while (!g_ParticleManager.rendered) {  // loop to avoid spurious wakeups
			g_ParticleManager.cond_var.wait(lock);
		}
		g_ParticleManager.SwapUpdateBuffer();
		g_ParticleManager.rendered = false;

		if (delta < 10)
		{
			int duration = 10 - static_cast<int>(delta/**1000.f*/);
			std::this_thread::sleep_for(std::chrono::milliseconds(duration));
		}

		nvtxRangePop();
	}
}


void test::init(void)
{
	// some code
	InitializeSettings();

	g_ParticleManager.Init(g_CurrentSettings.effectsCount);

	std::thread workerThread(WorkerThread);
	workerThread.detach(); // Glut + MSVC = join hangs in atexit()

	// some code
	
}

void test::term(void)
{
	// some code

	workerMustExit = true;

	// some code
}

void test::render(void)
{
	static float lastTime = 0.f;
	float time = globalTime.load();
	float delta = time - lastTime;
	lastTime = time;

	

	// some code
	std::unique_lock<std::mutex> lock(g_ParticleManager.mutex_);
	g_ParticleManager.Render();
//	g_ParticleManager.SwapRenderBuffer();
	g_ParticleManager.rendered = true;
	g_ParticleManager.cond_var.notify_one();

	// some code
}

void test::update(int dt)
{
	// some code
//	std::cout << g_ParticleManager.activeParticlesCount << "\n";
	
//	g_ParticleManager.Update(dt / 1000.f);

	float time = globalTime.load();
	globalTime.store(time + dt);

	// some code
}

void test::on_click(int x, int y)
{
	std::lock_guard<std::mutex> lock(g_ParticleManager.mutex_);
	
	g_ParticleManager.Emit(x, SCREEN_HEIGHT - y);
}