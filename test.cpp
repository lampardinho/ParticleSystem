
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
	g_CurrentSettings.emitOnDestroyProbability = 0.1;
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

	if (!isVisible)
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
	ParticleEffect* particles;
	int lastParticleId;
	int nextDeadParticleId;

	int activeParticlesCount;
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

	ParticleBuffer* mp_particles[3];
	int renderBufferId = 0;
	int lastUpdatedBufferId = 0;
	int currentUpdatingBufferId = 1;

	int lastParticleId;
	int nextDeadParticleId;

	int activeParticlesCount;
	bool rendered = false;
	std::condition_variable cond_var;
	std::mutex mutex_;
};

CParticleManager	g_ParticleManager;


CParticleManager::CParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		mp_particles[i] = nullptr;
	}
	
	m_num_particles = 0;

	lastParticleId = 0;
	nextDeadParticleId = 0;
	activeParticlesCount = 0;
}

CParticleManager::~CParticleManager()
{
	for (int i = 0; i < 3; ++i)
	{
		if (mp_particles[i])
			delete[] mp_particles[i];
	}
}

void CParticleManager::Init(int n)
{
	m_num_particles = n;

	for (int i = 0; i < 3; ++i)
	{
		mp_particles[i] = new ParticleEffect[n];
		
		for (int j = 0; j < n; j++)
		{
			mp_particles[i][j].Init(g_CurrentSettings.particlesPerEffectCount);
		}
	}
}


void CParticleManager::Update(float time)
{
	std::lock_guard<std::mutex> lock(mutex_);

	nvtxRangePush(__FUNCTION__);
	
	int count = activeParticlesCount;
	int last = lastParticleId;
	for (int index = 0; index < count; index++)
	{
		int i = (last + index) % m_num_particles;

		float t = globalTime.load();
		if (mp_particles[currentUpdatingBufferId][i].birthTime > 0 && mp_particles[currentUpdatingBufferId][i].birthTime + g_CurrentSettings.particleLifetime < t)
		{
			mp_particles[currentUpdatingBufferId][i].Destroy();
			lastParticleId = (lastParticleId + 1) % m_num_particles;
			activeParticlesCount--;	
			continue;
		}

		mp_particles[currentUpdatingBufferId][i].Update(mp_particles[lastUpdatedBufferId][i], time);
	}

	nvtxRangePop();
}

void CParticleManager::Render()
{
//	std::lock_guard<std::mutex> lock(mutex_);

	for (int index = 0; index < activeParticlesCount; index++)
	{
		int i = (lastParticleId + index) % m_num_particles;
		mp_particles[renderBufferId][i].Render();
	}

	renderCount++;	
}

void CParticleManager::Emit(int x, int y)
{
//	std::cout << activeParticlesCount << "\n";
//
//	if (activeParticlesCount > m_num_particles)
//		std::cout << "alert\n";

//	std::lock_guard<std::mutex> lock(mutex_);

	mp_particles[lastUpdatedBufferId][nextDeadParticleId].Emit(x, y);

	nextDeadParticleId = (nextDeadParticleId + 1) % m_num_particles;	

	if (nextDeadParticleId == lastParticleId)
		lastParticleId = (lastParticleId + 1) % m_num_particles;
	else
		activeParticlesCount++;
}

void CParticleManager::SwapRenderBuffer()
{
//	std::lock_guard<std::mutex> lock(mutex_);
//
//	std::swap(renderBufferId, lastUpdatedBufferId);
}

void CParticleManager::SwapUpdateBuffer()
{
	std::cout << currentUpdatingBufferId << " " << renderBufferId << " " << lastUpdatedBufferId << "\n";
	std::swap(currentUpdatingBufferId, lastUpdatedBufferId);
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

	std::thread workerThread(WorkerThread);
	workerThread.detach(); // Glut + MSVC = join hangs in atexit()

	// some code
	g_ParticleManager.Init(g_CurrentSettings.effectsCount);
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
//	std::lock_guard<std::mutex> lock(g_ParticleManager.mutex_);
	
	g_ParticleManager.Emit(x, SCREEN_HEIGHT - y);
}