/*!
@file       particlecomponent.h
@author     Renfred Lee
@author     Ranvitha
@author     Renfred (RTIS)
@date       01/12/2026
@brief      Particle Component data.
@details    Declare data types and functions that would be used for particles.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#ifndef PARTICLE_COMPONENT_H
#define PARTICLE_COMPONENT_H

#include "engine_pch.h"

namespace ParticleSystem {
    /*
    @brief  ParticleShape enum determines the way particle emits over lifetime.
    */
    enum class ParticleShape { LINE, CIRCLE, SQUARE };

    enum class ParticlePreset { Custom = 0, Smoke = 1, Fire = 2, Explosion = 3, SuperheroLanding = 4, Lava = 5, Ember = 6 };

    enum class ParticleRenderShape { Square = 0, Circle = 1, Triangle = 2 };
}

namespace Component {

    /*
	@brief  Stores individual live particle state.
    */
    /** @brief ParticleState */
    struct ParticleState {
        // Current live count
        size_t activeCount{ 0 };
        size_t nextSpawnIndex{ 0 };

		// Individual particle attributes
        std::vector<CEVec3> positions;
        std::vector<CEVec3> velocities;
        std::vector<CEVec3> accelerations;  // For "Force over lifetime"
        std::vector<CEColor> colors;
        std::vector<f32>  rotations;
        std::vector<CEVec2>  scales;
		std::vector<f32>  lives;          // Current life remaining
		std::vector<f32>  maxLives;       // Maximum life assigned
        std::vector<f32>  spawnTimes;
    };

    /*
    @brief  Particle Component data.
    */
    /** @brief ParticleComponent */
    struct ParticleComponent {
        ParticleSystem::ParticlePreset preset{ ParticleSystem::ParticlePreset::Custom };
        bool burstRequested{ false };
        int burstCount{ 0 };

        ParticleSystem::ParticleRenderShape renderShape{ ParticleSystem::ParticleRenderShape::Square };
        float squareSizeMultiplier{ 1.0f };
        float circleRadiusMultiplier{ 1.0f };
        int circleSegments{ 48 };
        float triangleWidthMultiplier{ 1.0f };
        float triangleHeightMultiplier{ 1.0f };

        f32 superheroLandingAngleDeg{ 8.0f };
        f32 superheroLandingAngleSpreadDeg{ 18.0f };

        bool playOnStop{ false };
        bool isEmitting{ false };
        bool useWorldSpace{ false };
        ParticleSystem::ParticleShape emissionShape{ ParticleSystem::ParticleShape::LINE };
        f32 lengthMin{ 0.f }, lengthMax{ 5.f };     // For if ParticleShape is Line
        f32 radiusMin{ 0.f }, radiusMax{ 5.f };     // For if ParticleShape is Circle
        CEVec2 areaMin{ 0.f, 0.f }, areaMax{ 5.f, 5.f };     // For if ParticleShape is Square

        // Emitter Settings
		int maxParticles{ 1000 };
		f32 emitRate{ 100.f };                      // Particles per second
        f32 emitAccumulator = 0.0f;
        f32 runtimeTime = 0.0f;
        bool gpuSimulation{ true };

        float flowStrength = 0.0f;
        float flowSpeed = 0.0f;
        float flowFrequency = 0.0f;
        bool additiveBlend{ false };
        bool emberRandomDirections{ false };
        CEVec2 prevEmitterScale{ 1.0f, 1.0f };
        bool prevEmitterScaleValid{ false };

        // Initial values
        f32 startLifeMin = 1.f, startLifeMax = 1.f;
        f32 startRotationMin = 0.f, startRotationMax = 0.f;
        f32 startSpeedMin = 1.f, startSpeedMax = 1.f;

        bool uniformSize{ true };
        f32 startSizeXMin = 1.f, startSizeXMax = 1.f;
        f32 startSizeYMin = 1.f, startSizeYMax = 1.f;
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
    @brief Init for particle ( reserve space )
    */
    void ParticleInit(Component::ParticleComponent& comp);

    /*
    @brief Update for particle
    @param entityId - ID of entity to reference
    @param comp     - The particle component of this entity to reference
    @param dt       - Delta time to reference
    */
    void ParticleUpdate(CrashoutsEngine::Entity const entityId, Component::ParticleComponent& comp, f32 dt);

    void ApplyPreset(Component::ParticleComponent& comp, ParticlePreset preset);
    
}

#endif
