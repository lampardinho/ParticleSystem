
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

const int	NUM_PARTICLES = 2048 * 64;


// Just some hints on implementation
// You could remove all of them

static std::atomic<float> globalTime;
static volatile bool workerMustExit = false;


// some code
class Particle
{

public:
	//	Vector2	m_old_pos;
	Vector2	m_pos;
	Vector2	m_vel;
	void	Update(float time);
};

void Particle::Update(float time)
{
	//	m_old_pos = m_pos;
	Vector2 pos2 = m_pos + m_vel * time;

	// bit of randomness
	//	m_vel += Vector2(rand() % 10000 / 1000.0f - 5.0f, rand() % 10000 / 1000.0f - 5.0f);
	// bit of gravity
	m_vel -= Vector2(0.0f, 1.0f);
	// friction
	//	m_vel -= m_vel * 0.002f;

	m_pos = pos2;
}








class ParticleEffect
{

public:
	Particle* particles;
	int count;
	float birthTime;

	~ParticleEffect();
	void	Update(float time);
	void Init(int n);
	void updateOffscreen(int i);
	bool removeOld(int& i);
	void Render();
	void Emit(int x, int y);
};

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
		if (birthTime > 0)
			platform::drawPoint(particles[i].m_pos.x, particles[i].m_pos.y, 1, 1, 1, 1);
	}
}








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
//	circular_buffer<bool> isAlive;
//	float* birthTime;

	int lastParticleId;
	int nextDeadParticleId;

	int activeParticlesCount;

	std::mutex mutex_;
};

CParticleManager	g_ParticleManager;

std::list<Vector2> activeParticlesPositions;

CParticleManager::CParticleManager()
{
		mp_particles = nullptr;
//		birthTime = nullptr;
	//	isAlive = NULL;
	m_num_particles = 0;

	lastParticleId = 0;
	nextDeadParticleId = 0;
	activeParticlesCount = 0;
}

CParticleManager::~CParticleManager()
{
	if (mp_particles)
		delete[] mp_particles;
///
///	if (isAlive)
///		delete[] isAlive;
///
	if (birthTime)
		delete[] birthTime;
}

void CParticleManager::Init(int n)
{
	mp_particles = new ParticleEffect[n];
//	isAlive.reserve(n);
	birthTime = new float[n];
	m_num_particles = n;
	for (int i = 0; i<n; i++)
	{
//		isAlive[i] = false;
		birthTime[i] = -1;
	}
}

void CParticleManager::updateOffscreen(int i)
{
//	nvtxRangePush(__FUNCTION__);

	if (mp_particles[i].m_pos.x < 0 || mp_particles[i].m_pos.x > test::SCREEN_WIDTH)
		birthTime[i] = -1;

	if (mp_particles[i].m_pos.y < 0 || mp_particles[i].m_pos.y > test::SCREEN_HEIGHT)
		birthTime[i] = -1;

//	nvtxRangePop();
}



// Call the particle update function, either directly
// or by starting the thread(s) and then waiting for them to finish
void CParticleManager::Update(float time)
{
	//may return 0 when not able to detect
//	unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
//	std::vector<std::thread> threads;
//
//	std::thread workerThread(WorkerThread);

	nvtxRangePush(__FUNCTION__);

	
	int count = activeParticlesCount;
	int last = lastParticleId;
	for (int index = 0; index < count; index++)
	{
		int i = (last + index) % m_num_particles;
		updateOffscreen(i);

		//	nvtxRangePush(__FUNCTION__);

		float t = globalTime.load();
		if (birthTime[i] > 0 && birthTime[i] + 2000 < t)
		{
			birthTime[i] = -1;
			lastParticleId++;
			activeParticlesCount--;
					if (rand() % 10 == 0)
						Emit(mp_particles[i].m_pos.x, mp_particles[i].m_pos.y);
			
		}

		//	nvtxRangePop();

		if (birthTime[i] > 0)
		{
			mp_particles[i].Update(time);
		}
	}

	nvtxRangePop();
}

void CParticleManager::Render()
{
	for (int index = 0; index < activeParticlesCount; index++)
	{
		int i = (lastParticleId + index) % m_num_particles;
		if (birthTime[i] > 0)
			platform::drawPoint(mp_particles[i].m_pos.x, mp_particles[i].m_pos.y, 1, 1, 1, 1);
	}
}

void CParticleManager::Emit(int x, int y)
{
	int start = nextDeadParticleId;
	int end = std::min(start + 64, m_num_particles);
	nextDeadParticleId = end % m_num_particles; //error fix last effect
	

	if (lastParticleId == start && activeParticlesCount > 0)
	{
		lastParticleId += 64;
	}
	else
	{
		activeParticlesCount += 64;
	}

	//fix buffer overlaping
//	std::lock_guard<std::mutex> lock(mutex_);
	nvtxRangePush(__FUNCTION__);

	for (int i = start; i<end; i++)
	{
//		isAlive.put(true);
		birthTime[i] = globalTime.load(); //store in variable

		mp_particles[i].m_pos = Vector2(x, y);
//		mp_particles[i].m_vel = Vector2(0, 0);
		mp_particles[i].m_vel = Vector2(rand() % 100 - 50, rand() % 100 - 50);
		mp_particles[i].m_vel = mp_particles[i].m_vel.Normal();
		mp_particles[i].m_vel = mp_particles[i].m_vel * (rand() % 100);

//		activeParticlesCount++;
//		nextDeadParticleId++;
	}
	
	nvtxRangePop();
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
//			if (duration != 10)
//			{
//				printf("%i", duration);
//			}
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
	g_ParticleManager.Init(NUM_PARTICLES);
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
	g_ParticleManager.Emit(x, SCREEN_HEIGHT - y);
}