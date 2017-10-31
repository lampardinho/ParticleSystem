
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


// Just some hints on implementation
// You could remove all of them

static std::atomic<float> globalTime;
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
	g_CurrentSettings.emitOnDestroyProbability = 0.7;
}


// some code
class Particle
{

public:
	Vector2	m_pos;
	Vector2	m_vel;
	bool isVisible = true;
	void	Update(float time);
};

void Particle::Update(float time)
{
	if (m_pos.x < 0 || m_pos.x > test::SCREEN_WIDTH)
		isVisible = false;

	if (m_pos.y < 0 || m_pos.y > test::SCREEN_HEIGHT)
		isVisible = false;

	if (!isVisible)
		return;

	Vector2 pos2 = m_pos + m_vel * time;
	m_vel -= Vector2(0.0f, g_CurrentSettings.gravity);
	m_pos = pos2;
}








class ParticleEffect
{

public:
	Particle* particles;
	int count;
	float birthTime;

	~ParticleEffect();
	void Update(float time);
	void Init(int n);
	void Render();
	void Emit(int x, int y);
	void Destroy();
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
//	private:
	int			m_num_particles;
	ParticleEffect* mp_particles;

	int lastParticleId;
	int nextDeadParticleId;

	int activeParticlesCount;

	std::mutex mutex_;
};

CParticleManager	g_ParticleManager;


CParticleManager::CParticleManager()
{
	mp_particles = nullptr;
	m_num_particles = 0;

	lastParticleId = 0;
	nextDeadParticleId = 0;
	activeParticlesCount = 0;
}

CParticleManager::~CParticleManager()
{
	if (mp_particles)
		delete[] mp_particles;
}

void CParticleManager::Init(int n)
{
	mp_particles = new ParticleEffect[n];
	m_num_particles = n;
	for (int i = 0; i<n; i++)
	{
		mp_particles[i].Init(g_CurrentSettings.particlesPerEffectCount);
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
		if (mp_particles[i].birthTime > 0 && mp_particles[i].birthTime + g_CurrentSettings.particleLifetime < t)
		{
			mp_particles[i].Destroy();
			lastParticleId = (lastParticleId + 1) % m_num_particles;
			activeParticlesCount--;	
			continue;
		}

		mp_particles[i].Update(time);		
	}

	nvtxRangePop();
}

void CParticleManager::Render()
{
	std::lock_guard<std::mutex> lock(mutex_);

	for (int index = 0; index < activeParticlesCount; index++)
	{
		int i = (lastParticleId + index) % m_num_particles;
		mp_particles[i].Render();
	}
}

void CParticleManager::Emit(int x, int y)
{
//	std::cout << activeParticlesCount << "\n";
//
//	if (activeParticlesCount > m_num_particles)
//		std::cout << "alert\n";

//	std::lock_guard<std::mutex> lock(mutex_);

	mp_particles[nextDeadParticleId].Emit(x, y);

	nextDeadParticleId = (nextDeadParticleId + 1) % m_num_particles;	

	if (nextDeadParticleId == lastParticleId)
		lastParticleId = (lastParticleId + 1) % m_num_particles;
	else
		activeParticlesCount++;
}








ParticleEffect::~ParticleEffect()
{
	if (particles)
		delete[] particles;
}

void ParticleEffect::Init(int n)
{
	particles = new Particle[n];
	count = n;
	birthTime = -1;
}

void ParticleEffect::Update(float time)
{
	for (int i = 0; i < count; ++i)
	{
		particles[i].Update(time);
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
	g_ParticleManager.Render();

	// some code
}

void test::update(int dt)
{
	// some code
	std::cout << g_ParticleManager.activeParticlesCount << "\n";
	
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