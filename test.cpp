#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>

#include "./nvToolsExt.h"

#include "test.h"
#include "ParticleManager.h"

static std::atomic<float> globalTime;
static volatile bool workerMustExit = false;
static std::unique_ptr<ParticleManager> g_ParticleManager;


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

void WorkerThread(void)
{
	while (!workerMustExit)
	{
		nvtxRangePush(__FUNCTION__);

		static float lastTime = 0.f;
		float time = globalTime.load();
		float delta = time - lastTime;
		lastTime = time;

		ParticleBuffer* buffer = g_ParticleManager->buffers[g_ParticleManager->currentUpdatingBufferId];
		int count = buffer->activeParticlesCount;

		int first = 0;
		float dt = delta / 1000.f;

		unsigned long const max_chunk_size = 64;
		if (count <= max_chunk_size)
		{
			g_ParticleManager->Update(first, count, dt, time);
		}
		else
		{
			int mid_point = count / 2;
			std::future<void> first_half = std::async(&ParticleManager::Update, g_ParticleManager.get(), first, mid_point, dt, time);
			g_ParticleManager->Update(mid_point, count, dt, time);
			first_half.get();
		}
		
		g_ParticleManager->SwapUpdateBuffer();

		if (delta < 10)
		{
			int duration = 10 - static_cast<int>(delta);
			std::this_thread::sleep_for(std::chrono::milliseconds(duration));
		}

		nvtxRangePop();
	}
}

void test::init(void)
{
	InitializeSettings();

	g_ParticleManager = std::make_unique<ParticleManager>();
	g_ParticleManager->Init(g_CurrentSettings.effectsCount, g_CurrentSettings.particlesPerEffectCount, g_CurrentSettings.particleLifetime,
		g_CurrentSettings.gravity, g_CurrentSettings.particleMaxInitialVelocity, g_CurrentSettings.emitOnDestroyProbability);

	std::thread workerThread(WorkerThread);
	workerThread.detach(); // Glut + MSVC = join hangs in atexit()		
}

void test::term(void)
{
	workerMustExit = true;
}

void test::render(void)
{
	g_ParticleManager->Render();
	g_ParticleManager->SwapRenderBuffer();
}

void test::update(int dt)
{
	float time = globalTime.load();
	globalTime.store(time + dt);
}

void test::on_click(int x, int y)
{
	float time = globalTime.load();
	g_ParticleManager->Emit(x, SCREEN_HEIGHT - y, time);
}