#include "nvToolsExt.h"
#include "test.h"
#include "ParticleManager.h"

#include <thread>
#include <atomic>
#include <future>

static std::atomic<float> globalTime;
static std::atomic_bool workerMustExit = false;
static std::shared_ptr<ParticleManager> particleManager;

struct Settings 
{
	int effectsCount = 0;
	int particlesPerEffectCount = 0;
	int particleMaxInitialVelocity = 0;
	float particleLifetime = 0;
	float gravity = 0;
	float emitOnDestroyProbability = 0;
};

Settings settings;

void InitializeSettings()
{
	settings.effectsCount = 2048;
	settings.particlesPerEffectCount = 64;
	settings.particleMaxInitialVelocity = 100;
	settings.particleLifetime = 2000;
	settings.gravity = 1;
	settings.emitOnDestroyProbability = 0.5;
}

void WorkerThread(void)
{
	std::shared_ptr<ParticleManager> manager = particleManager;

	while (!workerMustExit.load())
	{
		nvtxRangePush(__FUNCTION__);

		static float lastTime = 0.f;
		float time = globalTime.load();
		float delta = time - lastTime;
		lastTime = time;

		ParticleBuffer* buffer = manager->buffers[manager->currentUpdatingBufferId];
		int count = buffer->activeParticlesCount;
		int first = 0;
		float dt = delta / 1000.f;

		unsigned long const max_chunk_size = 64;
		if (count <= max_chunk_size)
		{
			manager->Update(first, count, dt, time);
		}
		else
		{
			int mid_point = count / 2;
			std::future<void> first_half = std::async(&ParticleManager::Update, manager.get(), first, mid_point, dt, time);
			manager->Update(mid_point, count, dt, time);
			first_half.get();
		}
		
		manager->SwapUpdateBuffer();

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

	particleManager = std::make_unique<ParticleManager>();
	particleManager->Init(settings.effectsCount, settings.particlesPerEffectCount, settings.particleLifetime,
		settings.gravity, settings.particleMaxInitialVelocity, settings.emitOnDestroyProbability);

	std::thread workerThread(WorkerThread);
	workerThread.detach(); // Glut + MSVC = join hangs in atexit()		
}

void test::term(void)
{
	workerMustExit.store(true);
}

void test::render(void)
{
	particleManager->Render();
	particleManager->SwapRenderBuffer();
}

void test::update(int dt)
{
	float time = globalTime.load();
	globalTime.store(time + dt);
}

void test::on_click(int x, int y)
{
	std::lock_guard<std::mutex> lock(particleManager->emitMutex);

	float time = globalTime.load();
	particleManager->Emit(x, SCREEN_HEIGHT - y, time);
}