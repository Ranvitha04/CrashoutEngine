/*!
@file       particleComponent.h
@author     Renfred (lee.r@digipen.edu)
@date       01/12/2026
@brief      Particle Component data.
@details    Declare data types and functions that would be used for particles.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#ifndef PARTICLE_COMPONENT_H
#define PARTICLE_COMPONENT_H

#include "engine_pch.h"

namespace ParticleSystem {
    /*
    @brief  ParticleShape enum determines the way particle emits over lifetime.
    */
    enum class ParticleShape { LINE, CIRCLE, SQUARE };
}

namespace Component {

    /*
	@brief  Stores individual live particle state.
    */
    struct ParticleState {
        // Current live count
        size_t activeCount{ 0 };

		// Individual particle attributes
        std::vector<CEVec3> positions;
        std::vector<CEVec3> velocities;
        std::vector<CEVec3> accelerations;  // For "Force over lifetime"
        std::vector<CEColor> colors;
        std::vector<f32>  rotations;
        std::vector<f32>  scales;
		std::vector<f32>  lives;          // Current life remaining
		std::vector<f32>  maxLives;       // Maximum life assigned
    };

    /*
    @brief  Particle Component data.
    */
    struct ParticleComponent {
        bool isEmitting{ false };
        bool useWorldSpace{ false };
        ParticleSystem::ParticleShape emissionShape{ ParticleSystem::ParticleShape::LINE };
        f32 radiusMin{ 0.f }, radiusMax{ 5.f };     // For if ParticleShape is Circle

        // Emitter Settings
		int maxParticles{ 1000 };
		f32 emitRate{ 100.f };                      // Particles per second
        f32 emitAccumulator = 0.0f;

        // Initial values
        f32 startLifeMin = 1.f, startLifeMax = 1.f;
        f32 startRotationMin = 0.f, startRotationMax = 0.f;
        f32 startSpeedMin = 1.f, startSpeedMax = 1.f;
        f32 startSizeMin = 1.f, startSizeMax = 1.f;
        //f32 endSize = 1.f;
        CEColor startcolor{ 1.f, 1.f, 1.f, 1.f };
        CEColor endColor = { 1.f, 1.f, 1.f, 0.f };
        CEVec3 startVelocity = { 0.f, 0.f, 0.f };
        CEVec3 endVelocity = { 0.f, 0.f, 0.f };

        // Modifiers
        CEVec3 gravity = { 0.f, -9.8f, 0.f };
        f32 rotationOverTime = 0.f;
        f32 sizeOverTime = 0.f;

        // Texture / Rendering
        std::string textureName;
        bool useTexture = false;
        CEVec2 textureUV{ 1.f, 1.f };

        // Live Data
        ParticleState data;
    };
}

namespace ParticleSystem {

    /*
    @brief Update for particle
    @param dt - Delta time to reference
    */
    void ParticleUpdate(CrashoutsEngine::Entity const entityId, Component::ParticleComponent& comp, f32 dt);
    
}

#endif