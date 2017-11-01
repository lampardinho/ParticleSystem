#include <stdlib.h>

#include "ParticleEffect.h"
#include "test.h"
#include <iostream>


void Particle::Update(float time, float gravity)
{
	if (position.x < 0 || position.x > test::SCREEN_WIDTH)
		isVisible = false;

	if (position.y < 0 || position.y > test::SCREEN_HEIGHT)
		isVisible = false;

	if (!isVisible)
		return;

	position += velocity * time;
	velocity.y -= gravity;	
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
	this->gravity = gravity;

	particles = new Particle[n];
	count = n;
	birthTime = -1;

	for (int i = 0; i < count; i++)
	{
		particles[i].isVisible = true;
		particles[i].position = Vector2(0, 0);
		particles[i].velocity = Vector2(0, 0);
	}
}

void ParticleEffect::Update(float time)
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
		{
			platform::drawPoint(particles[i].position.x, particles[i].position.y, 1, 1, 1, 1);
		}
	}
}

void ParticleEffect::Emit(int x, int y, float time)
{
	birthTime = time;

	for (int i = 0; i < count; i++)
	{
		particles[i].isVisible = true;
		particles[i].position = Vector2(x, y);
		particles[i].velocity = Vector2(rand() % 101 - 50, rand() % 101 - 50);
		particles[i].velocity = particles[i].velocity.Normal();
		particles[i].velocity = particles[i].velocity * (rand() % particleMaxInitialVelocity);
	}
}

void ParticleEffect::Destroy(std::function<void(int, int, float)> emitFunc, float time)
{
	birthTime = -1;

	for (int i = 0; i < count; i++)
	{
		if (!particles[i].isVisible)
			continue;

		if (rand() % 101 < emitOnDestroyProbability * 100)
			emitFunc(particles[i].position.x, particles[i].position.y, time);
	}
}