/*!
@file       particleManager.cpp
@author     Renfred (lee.r@digipen.edu)
@date       01/12/2026
@brief      Particle Functionalities.
@details    Functions that would be used to sync particles with the engine's update.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "particlecomponent.h"
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

	void ApplyPreset(Component::ParticleComponent& comp, ParticlePreset preset) {
		comp.preset = preset;

		auto ResetRuntime = [&]() {
			comp.emitAccumulator = 0.0f;
			comp.data.activeCount = 0;
			ParticleInit(comp);
		};

		switch (preset) {
		case ParticlePreset::Smoke: {
			comp.burstRequested = false;
			comp.burstCount = 0;
			comp.isEmitting = true;
			comp.playOnStop = true;
			comp.useWorldSpace = true;

			comp.renderShape = ParticleRenderShape::Circle;
			comp.circleSegments = 48;
			comp.circleRadiusMultiplier = 1.0f;

			comp.maxParticles = 1200;
			comp.emitRate = 220.0f;

			comp.emissionShape = ParticleShape::CIRCLE;
			comp.radiusMin = 0.0f;
			comp.radiusMax = 0.7f;

			comp.startLifeMin = 1.4f;
			comp.startLifeMax = 2.6f;
			comp.startSpeedMin = 0.4f;
			comp.startSpeedMax = 1.2f;

			comp.uniformSize = true;
			comp.startSizeXMin = 0.35f;
			comp.startSizeXMax = 0.75f;
			comp.startSizeYMin = 0.35f;
			comp.startSizeYMax = 0.75f;

			comp.sizeOverTime = 0.25f;
			comp.rotationOverTime = 10.0f;
			comp.startRotationMin = 0.0f;
			comp.startRotationMax = 360.0f;

			comp.gravity = { 0.0f, 0.9f, 0.0f };
			comp.startVelocity = { 0.0f, 0.9f, 0.0f };

			comp.useTexture = false;
			comp.textureName.clear();
			comp.textureUV = { 1.0f, 1.0f };

			comp.startcolor = { 0.45f, 0.45f, 0.45f, 0.35f };
			comp.endColor = { 0.15f, 0.15f, 0.15f, 0.0f };

			ResetRuntime();
		} break;
		case ParticlePreset::Fire: {
			comp.burstRequested = false;
			comp.burstCount = 0;
			comp.isEmitting = true;
			comp.playOnStop = true;
			comp.useWorldSpace = true;

			comp.renderShape = ParticleRenderShape::Square;
			comp.squareSizeMultiplier = 1.0f;

			comp.maxParticles = 1800;
			comp.emitRate = 950.0f;

			comp.emissionShape = ParticleShape::CIRCLE;
			comp.radiusMin = 0.0f;
			comp.radiusMax = 0.35f;

			comp.startLifeMin = 0.35f;
			comp.startLifeMax = 0.85f;
			comp.startSpeedMin = 2.5f;
			comp.startSpeedMax = 6.5f;

			comp.uniformSize = true;
			comp.startSizeXMin = 0.18f;
			comp.startSizeXMax = 0.45f;
			comp.startSizeYMin = 0.18f;
			comp.startSizeYMax = 0.45f;

			comp.sizeOverTime = -0.35f;
			comp.rotationOverTime = 35.0f;
			comp.startRotationMin = 0.0f;
			comp.startRotationMax = 360.0f;

			comp.gravity = { 0.0f, 5.0f, 0.0f };
			comp.startVelocity = { 0.0f, 2.0f, 0.0f };

			comp.useTexture = false;
			comp.textureName.clear();
			comp.textureUV = { 1.0f, 1.0f };

			comp.startcolor = { 1.0f, 0.65f, 0.1f, 0.95f };
			comp.endColor = { 0.4f, 0.05f, 0.0f, 0.0f };

			ResetRuntime();
		} break;
		case ParticlePreset::Explosion: {
			comp.isEmitting = true;
			comp.playOnStop = true;
			comp.useWorldSpace = true;

			comp.renderShape = ParticleRenderShape::Circle;
			comp.circleSegments = 64;
			comp.circleRadiusMultiplier = 1.0f;

			comp.maxParticles = 1400;
			comp.emitRate = 1200.0f;
			comp.burstRequested = true;
			comp.burstCount = comp.maxParticles;

			comp.emissionShape = ParticleShape::CIRCLE;
			comp.radiusMin = 0.0f;
			comp.radiusMax = 0.2f;

			comp.startLifeMin = 0.25f;
			comp.startLifeMax = 0.6f;
			comp.startSpeedMin = 8.0f;
			comp.startSpeedMax = 20.0f;

			comp.uniformSize = true;
			comp.startSizeXMin = 0.18f;
			comp.startSizeXMax = 0.55f;
			comp.startSizeYMin = 0.18f;
			comp.startSizeYMax = 0.55f;

			comp.sizeOverTime = -0.55f;
			comp.rotationOverTime = 80.0f;
			comp.startRotationMin = 0.0f;
			comp.startRotationMax = 360.0f;

			comp.gravity = { 0.0f, -3.0f, 0.0f };
			comp.startVelocity = { 0.0f, 0.0f, 0.0f };

			comp.useTexture = false;
			comp.textureName.clear();
			comp.textureUV = { 1.0f, 1.0f };

			comp.startcolor = { 1.0f, 0.7f, 0.2f, 0.95f };
			comp.endColor = { 0.25f, 0.05f, 0.0f, 0.0f };

			ResetRuntime();
		} break;
		case ParticlePreset::SuperheroLanding: {
			comp.isEmitting = true;
			comp.playOnStop = true;
			comp.useWorldSpace = true;

			comp.renderShape = ParticleRenderShape::Circle;
			comp.circleSegments = 48;
			comp.circleRadiusMultiplier = 1.0f;

			comp.superheroLandingAngleDeg = 8.0f;
			comp.superheroLandingAngleSpreadDeg = 18.0f;

			comp.maxParticles = 600;
			comp.emitRate = 1600.0f;
			comp.burstRequested = true;
			comp.burstCount = 380;

			comp.emissionShape = ParticleShape::CIRCLE;
			comp.radiusMin = 0.0f;
			comp.radiusMax = 0.18f;

			comp.startLifeMin = 0.9f;
			comp.startLifeMax = 1.9f;
			comp.startSpeedMin = 2.5f;
			comp.startSpeedMax = 7.5f;

			comp.uniformSize = true;
			comp.startSizeXMin = 1.2f;
			comp.startSizeXMax = 3.0f;
			comp.startSizeYMin = 1.2f;
			comp.startSizeYMax = 3.0f;

			comp.sizeOverTime = 1.35f;
			comp.rotationOverTime = 8.0f;
			comp.startRotationMin = 0.0f;
			comp.startRotationMax = 360.0f;

			comp.gravity = { 0.0f, -0.8f, 0.0f };
			comp.startVelocity = { 0.0f, 0.6f, 0.0f };

			comp.useTexture = false;
			comp.textureName.clear();
			comp.textureUV = { 1.0f, 1.0f };

			comp.startcolor = { 0.62f, 0.58f, 0.52f, 0.75f };
			comp.endColor = { 0.32f, 0.30f, 0.28f, 0.0f };

			ResetRuntime();
		} break;
		case ParticlePreset::Custom:
		default:
			break;
		}
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

		// Get the TRUE capacity of the vectors.
		size_t actualCapacity = data.positions.size();

		if (actualCapacity == 0) {
			data.activeCount = 0;
			return; // Safety Exit: No memory allocated
		}

		// If vectors are mismatched size (rare, but possible if Init failed halfway), default to safest size.
		if (data.velocities.size() < actualCapacity) actualCapacity = data.velocities.size();
		if (data.colors.size() < actualCapacity) actualCapacity = data.colors.size();

		// If activeCount exceeds the actual physical memory size, we MUST clamp it.
		if (data.activeCount > actualCapacity) {
			data.activeCount = actualCapacity;
		}

		CrashoutsEngine::ECS* ecs = CrashoutsEngine::ECS::getInstance();

		// Transform of the object with the particle system component
		const Component::TransformComponent* t = 
			ecs->GetComponent<Component::TransformComponent>(entityId);

		if (!t) {
			data.activeCount = 0;
			return;
		}

		f32 scaleX = t->size.x;
		f32 scaleY = t->size.y;

		// 1. Emission
		bool stopEmittingAfterThisEmissionStage = false;
		if (comp.burstRequested && comp.burstCount > 0) {
			if (comp.emitRate <= 0.0f) comp.emitRate = 1.0f;
			f32 secondsPerParticle = 1.f / comp.emitRate;
			comp.emitAccumulator += secondsPerParticle * static_cast<f32>(comp.burstCount);
			comp.burstRequested = false;
			if (comp.preset == ParticlePreset::Explosion || comp.preset == ParticlePreset::SuperheroLanding) {
				stopEmittingAfterThisEmissionStage = true;
			}
		}

		if (comp.isEmitting && comp.emitRate > 0.f && comp.maxParticles > 0) {
			comp.emitAccumulator += dt;
			f32 secondsPerParticle = 1.f / comp.emitRate;

			// If can emit base on time and rate
			while (comp.emitAccumulator >= secondsPerParticle) {
				if (data.activeCount < comp.maxParticles && data.activeCount < actualCapacity) {
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
					if (comp.preset == ParticlePreset::SuperheroLanding) {
						f32 baseDeg = comp.superheroLandingAngleDeg;
						f32 spreadDeg = comp.superheroLandingAngleSpreadDeg;
						if (spreadDeg < 0.0f) spreadDeg = 0.0f;

						f32 minDeg = baseDeg - (spreadDeg * 0.5f);
						f32 maxDeg = baseDeg + (spreadDeg * 0.5f);
						if (minDeg < 0.0f) minDeg = 0.0f;
						if (maxDeg > 89.0f) maxDeg = 89.0f;
						if (minDeg > maxDeg) minDeg = maxDeg;

						f32 thetaDeg = RandomRange(minDeg, maxDeg);
						f32 theta = thetaDeg * (CEMath::PI / 180.0f);
						f32 x = cosf(theta);
						f32 y = sinf(theta);

						f32 sign = (localPos.x >= 0.0f) ? 1.0f : -1.0f;
						if (fabsf(localPos.x) < 0.0001f) {
							sign = (RandomRange(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
						}

						CEVec3 heroLocalDir{ sign * x, y, 0.0f };
						rotatedDir.x = heroLocalDir.x * c - heroLocalDir.y * s;
						rotatedDir.y = heroLocalDir.x * s + heroLocalDir.y * c;
					}

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
		if (stopEmittingAfterThisEmissionStage) {
			comp.isEmitting = false;
			comp.emitAccumulator = 0.0f;
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
			if (comp.preset == ParticlePreset::SuperheroLanding) {
				const float baseY = t->position.y;
				if (data.positions[i].y < baseY) {
					data.positions[i].y = baseY;
					if (data.velocities[i].y < 0.0f) data.velocities[i].y = 0.0f;
				}

				float a = comp.superheroLandingAngleDeg;
				if (a < 0.0f) a = 0.0f;
				if (a > 89.0f) a = 89.0f;
				float t01 = a / 89.0f;

				const float dragX = 3.0f;
				const float dragY = 3.0f - (2.2f * t01);
				const float dampX = std::max(0.0f, 1.0f - dragX * dt);
				const float dampY = std::max(0.0f, 1.0f - dragY * dt);
				data.velocities[i].x *= dampX;
				data.velocities[i].y *= dampY;
			}

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

