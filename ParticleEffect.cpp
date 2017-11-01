#include <stdlib.h>

#include "ParticleEffect.h"
#include "ParticleManager.h"
#include "test.h"


void Particle::Update(float time, float gravity)
{
	if (m_pos.x < 0 || m_pos.x > test::SCREEN_WIDTH)
		isVisible = false;

	if (m_pos.y < 0 || m_pos.y > test::SCREEN_HEIGHT)
		isVisible = false;

	if (!isVisible)
		return;

	m_pos += m_vel * time;
	m_vel -= Vector2(0.0f, gravity);	
}

ParticleEffect::~ParticleEffect()
{
	if (particles)
	{
		delete[] particles;
		particles = nullptr;
	}
}

void ParticleEffect::Init(int n, float gravity, float particleMaxInitialVelocity, float emitOnDestroyProbability)
{
	this->particleMaxInitialVelocity = particleMaxInitialVelocity;
	this->emitOnDestroyProbability = emitOnDestroyProbability;

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
		particles[i].Update(time, gravity);
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

void ParticleEffect::Emit(int x, int y, float time)
{
	birthTime = time;

	for (int i = 0; i < count; i++)
	{
		particles[i].isVisible = true;
		particles[i].m_pos = Vector2(x, y);
		particles[i].m_vel = Vector2(rand() % 101 - 50, rand() % 101 - 50);
		particles[i].m_vel = particles[i].m_vel.Normal();
		particles[i].m_vel = particles[i].m_vel * (rand() % particleMaxInitialVelocity);
	}
}

void ParticleEffect::Destroy(std::function<void(int, int, float)> emitFunc) //no time passed
{
	birthTime = -1;

	for (int i = 0; i < count; i++)
	{
		if (!particles[i].isVisible)
			continue;

		if (rand() % 101 < emitOnDestroyProbability * 100)
			emitFunc(particles[i].m_pos.x, particles[i].m_pos.y);
	}
}