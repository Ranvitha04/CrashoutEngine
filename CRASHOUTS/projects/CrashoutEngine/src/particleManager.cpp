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

	void ParticleUpdate(CrashoutsEngine::Entity const entityId, Component::ParticleComponent& comp, f32 dt) {
		auto& data = comp.data;

		// Transform of the object with the particle system component
		const Component::TransformComponent* t = 
			CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(entityId);

		// 1. Emission
		if (comp.isEmitting && comp.emitRate > 0.f) {
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

					float speed = RandomRange(comp.startSpeedMin, comp.startSpeedMax);

					CEVec3 localSpawnPos{ 0, 0, 0 };
					CEVec3 localSpawnDir{ 0, 1, 0 }; // Default pointing "Up"

					switch (comp.emissionShape) {
					case ParticleShape::LINE:
						localSpawnPos.x = RandomRange(-1.0f, 1.0f); // Random spot on a horizontal line
						localSpawnDir = { 0, 1, 0 };
						break;
					case ParticleShape::CIRCLE: {
						float angle = RandomRange(0, 2.0f * 3.14159f);
						localSpawnPos = { cosf(angle), sinf(angle), 0 };
						localSpawnDir = localSpawnPos; // Shoot outwards from the center
						break;
					}
					case ParticleShape::SQUARE:
						localSpawnPos = { RandomRange(-1, 1), RandomRange(-1, 1), 0 };
						localSpawnDir = { 0, 1, 0 };
						break;
					}

					// Rotate and Translate for World Space
					float speed = RandomRange(comp.startSpeedMin, comp.startSpeedMax);

					if (comp.useWorldSpace && t) {
						float radians = t->rotation * (CEMath::PI / 180.0f);
						// Manually rotate the velocity vector using the Transform's rotation (theta)
						float cosT = cosf(radians);
						float sinT = sinf(radians);

						// Rotate the local direction to match entity orientation
						CEVec3 rotatedDir;
						rotatedDir.x = localSpawnDir.x * cosT - localSpawnDir.y * sinT;
						rotatedDir.y = localSpawnDir.x * sinT + localSpawnDir.y * cosT;

						data.velocities[i] = rotatedDir * speed;

						// Position becomes Emitter Pos + (Rotated Local Offset)
						CEVec3 rotatedPos;
						rotatedPos.x = localSpawnPos.x * cosT - localSpawnPos.y * sinT;
						rotatedPos.y = localSpawnPos.x * sinT + localSpawnPos.y * cosT;
						data.positions[i] = t->position + rotatedPos;
					}
					else {
						// Pure Local Space: we don't rotate by the entity's rotation here, 
						// because the Renderer will handle the rotation of the entire "container"
						data.velocities[i] = localSpawnDir * speed;
						data.positions[i] = localSpawnPos;
					}

					data.colors[i] = comp.startcolor;
					data.scales[i] = RandomRange(comp.startSizeMin, comp.startSizeMax);
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
			data.scales[i] += comp.sizeOverTime * dt;

			// Simulate change over lifetime
			f32 t = 1.0f - (data.lives[i] / data.maxLives[i]);

			// Interpolate items
			//float startSize = data.scales[i];
			//data.scales[i] = std::lerp(data.scales[i], comp.endSize, t * 0.1f);
			data.colors[i] = Lerp(data.colors[i], comp.endColor, t * 0.1f);

			i++;
		}
	}

}

