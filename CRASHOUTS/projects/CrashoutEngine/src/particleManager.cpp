/*!
@file       particleManager.cpp
@author     Renfred (lee.r@digipen.edu)
@date       01/12/2026
@brief      Particle Functionalities.
@details    Functions that would be used to sync particles with the engine's update.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "particleComponent.h"
#include "entitymanager.h"
#include "transformcomponent.h"
#include "CEMath.h"
#include <random>

namespace ParticleSystem {

	// Color lerp helper
	CEColor Lerp(CEColor& first, CEColor& second, f32 factor) {
		return first + (second - first) * factor;
	}

	// Helper for random
	static std::mt19937 gRng{ std::random_device{}() };
	static std::uniform_real_distribution<f32> gDist01{ 0.0f, 1.0f };

	f32 RandomRange(f32 min, f32 max) {
		return min + gDist01(gRng) * (max - min);
	}

	void ParticleInit(Component::ParticleComponent& comp) {
		auto& data = comp.data;
		size_t maxParticles = (size_t)comp.maxParticles;

		data.positions.resize(maxParticles);
		data.velocities.resize(maxParticles);
		data.accelerations.resize(maxParticles);
		data.colors.resize(maxParticles);
		data.rotations.resize(maxParticles);
		data.scales.resize(maxParticles);
		data.lives.resize(maxParticles);
		data.maxLives.resize(maxParticles);

		// If we shrunk the arrays, we must reduce the active count to match.
		// Otherwise, the Update loop will access out-of-bounds memory.
		if (data.activeCount > maxParticles) {
			data.activeCount = maxParticles;
		}
	}

	void ParticleUpdate(CrashoutsEngine::Entity const entityId, Component::ParticleComponent& comp, f32 dt) {
		auto& data = comp.data;

		// Transform of the object with the particle system component
		const Component::TransformComponent* t = 
			CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(entityId);

		f32 scaleX = t->size.x;
		f32 scaleY = t->size.y;

		// 1. Emission
		if (comp.isEmitting && comp.emitRate > 0.f && comp.maxParticles > 0) {
			comp.emitAccumulator += dt;
			f32 secondsPerParticle = 1.f / comp.emitRate;

			// If can emit base on time and rate
			while (comp.emitAccumulator >= secondsPerParticle) {
				if (data.activeCount < comp.maxParticles) {
					// Init new particle at the activeCount index
					size_t i = data.activeCount;

					// Random initial values based on component 
					f32 startLife = RandomRange(comp.startLifeMin, comp.startLifeMax);
					data.lives[i] = startLife;
					data.maxLives[i] = startLife;

					f32 speed = RandomRange(comp.startSpeedMin, comp.startSpeedMax);

					CEVec3 localPos{ 0, 0, 0 };
					CEVec3 localDir{ 0, 1, 0 }; // Default pointing "Up"

					switch (comp.emissionShape) {
					case ParticleShape::LINE: {
						// Get random length, then scale by Object X scale
						f32 baseLen = RandomRange(comp.lengthMin, comp.lengthMax);
						f32 finalLen = baseLen * scaleX;
						f32 halfLen = finalLen * 0.5f;

						// Spawn anywhere along that line
						localPos.x = RandomRange(-halfLen, halfLen);
						localPos.y = 0.f;

						// Line direction usually points "Up" (perpendicular to the line)
						localDir = { 0, 1, 0 };
						break;
					}
					case ParticleShape::CIRCLE: {
						f32 angle = RandomRange(0, 2.0f * 3.14159f);
                        
                        // Get random radius
                        f32 baseRadius = RandomRange(comp.radiusMin, comp.radiusMax);

                        // Ellipse Logic: Scale X and Y independently based on Transform
                        // If scaleX == scaleY, it remains a perfect circle.
                        f32 rX = baseRadius * scaleX;
                        f32 rY = baseRadius * scaleY;

                        localPos = { cosf(angle) * rX, sinf(angle) * rY, 0 };
                        
                        // Direction is normalized vector from center
                        // (This ensures particles shoot outward even if shape is squashed)
                        localDir = { cosf(angle), sinf(angle), 0 };
                        break;
					}
					case ParticleShape::SQUARE: {
						// Get random width/height range
						f32 baseW = RandomRange(comp.areaMin.x, comp.areaMax.x);
						f32 baseH = RandomRange(comp.areaMin.y, comp.areaMax.y);

						// Scale by Object Transform
						f32 finalW = baseW * scaleX;
						f32 finalH = baseH * scaleY;

						f32 halfW = finalW * 0.5f;
						f32 halfH = finalH * 0.5f;

						localPos = { RandomRange(-halfW, halfW), RandomRange(-halfH, halfH), 0 };

						// Default square direction is usually Up, or you can randomize it
						localDir = { 0, 1, 0 };
						break;
					}
					}

					// --- ROTATION (Applies to both) ---
					// Rotate the local offset so it matches the emitter's orientation
					f32 rads = t->rotation * (CEMath::PI / 180.0f);
					f32 c = cosf(rads);
					f32 s = sinf(rads);

					CEVec3 rotatedPos;
					rotatedPos.x = localPos.x * c - localPos.y * s;
					rotatedPos.y = localPos.x * s + localPos.y * c;

					CEVec3 rotatedDir;
					rotatedDir.x = localDir.x * c - localDir.y * s;
					rotatedDir.y = localDir.x * s + localDir.y * c;

					// --- SPAWN LOGIC ---
					if (comp.useWorldSpace) {
						data.positions[i] = t->position + rotatedPos;
					}
					else {
						data.positions[i] = rotatedPos;
					}

					// Velocity
					speed = RandomRange(comp.startSpeedMin, comp.startSpeedMax);
					data.velocities[i] = comp.startVelocity + (rotatedDir * speed);

					// Properties
					data.lives[i] = RandomRange(comp.startLifeMin, comp.startLifeMax);
					data.maxLives[i] = data.lives[i];
					data.colors[i] = comp.startcolor;
					if (comp.uniformSize) {
						f32 size = RandomRange(comp.startSizeXMin, comp.startSizeXMax);
						data.scales[i] = CEVec2(size, size);
					}
					else {
						data.scales[i] = CEVec2(
							RandomRange(comp.startSizeXMin, comp.startSizeXMax),
							RandomRange(comp.startSizeYMin, comp.startSizeYMax)
						);
					}
					data.rotations[i] = RandomRange(comp.startRotationMin, comp.startRotationMax);

					data.activeCount++;
				}
				comp.emitAccumulator -= secondsPerParticle;
			}
		}

		// 2. Simulation & Recycling
		for (size_t i = 0; i < data.activeCount;) {
			// Update lifetime
			data.lives[i] -= dt;

			// Check if dead
			if (data.lives[i] <= 0.f) {
				// Recycle: Replace dead with the last active item
				size_t lastIndex = data.activeCount - 1;

				data.positions[i] = data.positions[lastIndex];
				data.velocities[i] = data.velocities[lastIndex];
				data.colors[i] = data.colors[lastIndex];
				data.scales[i] = data.scales[lastIndex];
				data.lives[i] = data.lives[lastIndex];
				data.maxLives[i] = data.maxLives[lastIndex];
				data.rotations[i] = data.rotations[lastIndex];

				data.activeCount -= 1;
				// Don't increment 'i' because new swapped item need to be evaluated too
				continue;
			}

			// Simulate Movement & Physics
			data.velocities[i] += comp.gravity * dt;
			data.positions[i] += data.velocities[i] * dt;

			// Rotation & Scale Over Time (Angular Velocity)
			data.rotations[i] += comp.rotationOverTime * dt;
			f32 sizeChange = comp.sizeOverTime * dt;
			data.scales[i] += CEVec2(sizeChange, sizeChange);

			// Simulate change over lifetime
			f32 factor = 1.0f - (data.lives[i] / data.maxLives[i]);

			// Interpolate items
			//float startSize = data.scales[i];
			//data.scales[i] = std::lerp(data.scales[i], comp.endSize, t * 0.1f);
			data.colors[i] = Lerp(data.colors[i], comp.endColor, factor * 0.1f);

			i++;
		}
	}

}

