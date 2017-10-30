
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
class CParticle
{

public:
//	Vector2	m_old_pos;
	Vector2	m_pos;
	Vector2	m_vel;
	void	Update(float time);
};

void CParticle::Update(float time)
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
	circular_buffer<CParticle> mp_particles;
//	circular_buffer<bool> isAlive;
	circular_buffer<float> birthTime;

//	int lastParticleId;
//	int nextDeadParticleId;

//	int activeParticlesCount;
};

CParticleManager	g_ParticleManager;

std::list<Vector2> activeParticlesPositions;

CParticleManager::CParticleManager()
{
	//	mp_particles (m_num_particles);
	//	isAlive = NULL;
	m_num_particles = 0;

//	lastParticleId = 0;
//	nextDeadParticleId = 0;
//	activeParticlesCount = 0;
}

CParticleManager::~CParticleManager()
{
//	if (mp_particles)
///		delete[] mp_particles;
///
///	if (isAlive)
///		delete[] isAlive;
///
///	if (birthTime)
///		delete[] birthTime;
}

void CParticleManager::Init(int n)
{
	mp_particles.reserve(n);
//	isAlive.reserve(n);
	birthTime.reserve(n);
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

bool CParticleManager::removeOld(int& i)
{
//	nvtxRangePush(__FUNCTION__);

	float time = globalTime.load();
	bool res = false;
	if (birthTime[i] > 0 && birthTime[i] + 2000 < time)
	{
		mp_particles.removeLast();
		birthTime.removeLast();
		if (rand() % 10 == 0)
			Emit(mp_particles[i].m_pos.x, mp_particles[i].m_pos.y);
		i--;
		res = true;
	}

//	nvtxRangePop();
	return res;
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

	

	for (int i = 0; i < mp_particles.size(); i++)
	{
		updateOffscreen(i);

		if (removeOld(i)) continue;

		if (birthTime[i] > 0)
		{
			mp_particles[i].Update(time);
		}
	}

	nvtxRangePop();
}

void CParticleManager::Render()
{
	for (int i = 0; i < mp_particles.size(); i++)
	{
		if (birthTime[i] > 0)
			platform::drawPoint(mp_particles[i].m_pos.x, mp_particles[i].m_pos.y, 1, 1, 1, 1);
	}
}

void CParticleManager::Emit(int x, int y)
{
//	int start = nextDeadParticleId;
//	int end = std::min(start + 64, m_num_particles);
//	nextDeadParticleId = end % m_num_particles;
//	activeParticlesCount += 64;
	//fix buffer overlaping

	nvtxRangePush(__FUNCTION__);

	for (int i = 0; i<64; i++)
	{
//		isAlive.put(true);
		float& time = birthTime.put();
		time = globalTime.load(); //store in variable

		CParticle& particle = mp_particles.put();
		particle.m_pos = Vector2(x, y);
		particle.m_vel = Vector2(rand() % 100 - 50, rand() % 100 - 50);
		particle.m_vel = particle.m_vel.Normal();
		particle.m_vel = particle.m_vel * (rand() % 100);

		
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
		
		if (delta < 100)
		{
			int duration = 100 - static_cast<int>(delta/**1000.f*/);
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
	
	std::cout << g_ParticleManager.mp_particles.size() << "\n";
//	g_ParticleManager.Update(dt / 1000.f);

	float time = globalTime.load();
	globalTime.store(time + dt);

	// some code
}

void test::on_click(int x, int y)
{
	g_ParticleManager.Emit(x, SCREEN_HEIGHT - y);
}