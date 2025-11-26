/*!
@file       particleSystem.h
@author     Ranvitha
@date       26/11/2025
@brief      Declarations for particle and trajectory emitters.
@details    Provides emitter interfaces that update ECS entities for instanced
            rendering. Emitters implement randomized spawning, explicit Euler
            integration under gravity, and configurable appearance. The
            trajectory emitter computes launch angle/flight-time kinematics and
            samples by arc length for evenly spaced preview markers.
*/
#pragma once

#include <vector>
#include <random>
#include <string>
#include "CEVec2.h"
#include "CEVec3.h"
#include "instanceSystem.h"
#include "entitymanager.h"

namespace Component {

class ParticleEmitter {
public:
    /*!
    @brief  General-purpose rectangle particle emitter.
    @details Spawns particles with configurable lifetime, speed, gravity, and
             color/size interpolation. Designed for flame/smoke-like effects.
    */
public:
    ParticleEmitter(int maxCount = 1000);

    void Initialize(const CEVec3& origin);
    void Update(float dt);
    void Render(bool useTexture = false,
                const std::string& texName = std::string{},
                const CEVec3& tint = CEVec3(1.f,1.f,1.f),
                const CEVec2& uvSize = CEVec2(1.f,1.f)) const;

    void SetOrigin(const CEVec3& o);
    void SetEmissionRate(float particlesPerSecond);
    void SetLifeRange(float minLife, float maxLife);
    void SetSpeedRange(float minSpeed, float maxSpeed);
    void SetGravity(const CEVec3& g);
    void SetSizeRange(float startSize, float endSize);
    void SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01);

    void Cleanup();
    
    const std::vector<CrashoutsEngine::Entity>& GetEntities() const;

    void SyncEcs();

private:
    struct Particle {
        CEVec3 position;
        CEVec3 velocity;
        CEVec3 color;
        float rotation;
        float size;
        float life;
        float maxLife;
    };

    CEVec3 m_origin;
    CEVec3 m_gravity;                                                                                                          
    float m_emitRate;
    float m_lifeMin;
    float m_lifeMax;
    float m_speedMin;
    float m_speedMax;
    float m_startSize;
    float m_endSize;
    CEVec3 m_startColor01;
    CEVec3 m_endColor01;

    int m_maxParticles;
    float m_emitAccumulator;
    std::vector<Particle> m_particles;

    std::vector<CEVec3> m_positions;
    std::vector<CEVec3> m_colors;
    std::vector<float>  m_rotations;
    std::vector<float>  m_scales;
    std::vector<float>  m_widths;
    std::vector<float>  m_heights;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_u01;
    std::vector<CrashoutsEngine::Entity> m_entities;
};

class FireEmitter {
public:
    /*!
    @brief  Upward-biased fire emitter using rectangles.
    @details Emits around a base angle with spread; integrates motion under
             gravity; linearly decays size and lerps color over lifetime.
    */
public:
    FireEmitter(int maxCount = 2000);
    void Initialize(const CEVec3& origin);
    void Update(float dt);
    void Render(bool useTexture = false,
                const std::string& texName = std::string{},
                const CEVec3& tint = CEVec3(1.f,1.f,1.f),
                const CEVec2& uvSize = CEVec2(1.f,1.f)) const;

    void SetOrigin(const CEVec3& o);
    void SetEmissionRate(float particlesPerSecond);
    void SetLifeRange(float minLife, float maxLife);
    void SetSpeedRange(float minSpeed, float maxSpeed);
    void SetGravity(const CEVec3& g);
    void SetSizeRange(float startSize, float endSize);
    void SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01);
    void SetSpread(float radians);

    void Cleanup();
    
    const std::vector<CrashoutsEngine::Entity>& GetEntities() const;

    void SyncEcs();

private:
    struct Particle {
        CEVec3 position;
        CEVec3 velocity;
        CEVec3 color;
        float rotation;
        float size;
        float life;
        float maxLife;
    };

    CEVec3 m_origin;
    CEVec3 m_gravity;
    float m_emitRate;
    float m_lifeMin;
    float m_lifeMax;
    float m_speedMin;
    float m_speedMax;
    float m_startSize;
    float m_endSize;
    CEVec3 m_startColor01;
    CEVec3 m_endColor01;
    float m_spread;

    int m_maxParticles;
    float m_emitAccumulator;
    std::vector<Particle> m_particles;

    std::vector<CEVec3> m_positions;
    std::vector<CEVec3> m_colors;
    std::vector<float>  m_rotations;
    std::vector<float>  m_scales;
    std::vector<float>  m_widths;
    mutable std::vector<float>  m_heights;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_u01;
    std::vector<CrashoutsEngine::Entity> m_entities;
};

class FireballEmitter {
public:
    /*!
    @brief  Moving core with trailing circular particles.
    @details Core integrates with gravity; trail particles spawn behind the
             heading with spread and decay in size/color over age.
    */
public:
    FireballEmitter(int maxCount = 2500);
    void Initialize(const CEVec3& origin);
    void Update(float dt);
    void Render(bool useTexture = false,
                const std::string& texName = std::string{},
                const CEVec3& tint = CEVec3(1.f,1.f,1.f),
                const CEVec2& uvSize = CEVec2(1.f,1.f)) const;

    void SetOrigin(const CEVec3& o);
    void SetVelocity(const CEVec3& v);
    void SetRadius(float r);
    void SetEmissionRate(float particlesPerSecond);
    void SetLifeRange(float minLife, float maxLife);
    void SetSpeedRange(float minSpeed, float maxSpeed);
    void SetGravity(const CEVec3& g);
    void SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01);
    void SetSpread(float radians);
    void SetBounds(float worldW, float worldH);

    void Cleanup();
    
    const std::vector<CrashoutsEngine::Entity>& GetTrailEntities() const;
    CrashoutsEngine::Entity GetCoreEntity() const;

    void SyncEcs();

private:
    struct Particle {
        CEVec3 position;
        CEVec3 velocity;
        CEVec3 color;
        float rotation;
        float size;
        float life;
        float maxLife;
    };

    CEVec3 m_corePos;
    CEVec3 m_coreVel;
    float  m_coreRadius;
    CEVec3 m_coreColor;

    CEVec3 m_gravity;
    float m_emitRate;
    float m_lifeMin;
    float m_lifeMax;
    float m_speedMin;
    float m_speedMax;
    float m_spread;
    CEVec3 m_startColor01;
    CEVec3 m_endColor01;

    float m_worldW;
    float m_worldH;

    int m_maxParticles;
    float m_emitAccumulator;
    mutable std::vector<Particle> m_particles;

    mutable std::vector<CEVec3> m_positions;
    mutable std::vector<CEVec3> m_colors;
    mutable std::vector<float>  m_rotations;
    mutable std::vector<float>  m_scales;
    mutable std::vector<float>  m_widths;
    mutable std::vector<float>  m_heights;

    mutable std::vector<CEVec3> m_corePosArr;
    mutable std::vector<CEVec3> m_coreColorArr;
    mutable std::vector<float>  m_coreRotArr;
    mutable std::vector<float>  m_coreScaleArr;
    mutable std::vector<float>  m_coreRadiusArr;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_u01;
    CrashoutsEngine::Entity m_coreEntity{0};
    std::vector<CrashoutsEngine::Entity> m_entities;
};

class ExplosionEmitter {
public:
    /*!
    @brief  Radial burst emitter.
    @details Spawns short-lived particles in all directions; supports periodic
             bursts and basic gravity influence.
    */
public:
    ExplosionEmitter(int maxCount = 2000);
    void Initialize(const CEVec3& origin);
    void Update(float dt);
    void Render(bool useTexture = false,
                const std::string& texName = std::string{},
                const CEVec3& tint = CEVec3(1.f,1.f,1.f),
                const CEVec2& uvSize = CEVec2(1.f,1.f)) const;

    void SetOrigin(const CEVec3& o);
    void SetEmissionRate(float particlesPerSecond);
    void SetLifeRange(float minLife, float maxLife);
    void SetSpeedRange(float minSpeed, float maxSpeed);
    void SetGravity(const CEVec3& g);
    void SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01);
    void SetSpread(float radians);
    void SetBurst(int count, float intervalSeconds);
    void TriggerBurst();

    void Cleanup();
    const std::vector<CrashoutsEngine::Entity>& GetEntities() const;
    void SyncEcs();

private:
    struct Particle {
        CEVec3 position;
        CEVec3 velocity;
        CEVec3 color;
        float rotation;
        float size;
        float life;
        float maxLife;
    };

    CEVec3 m_origin;
    CEVec3 m_gravity;
    float m_emitRate;
    float m_lifeMin;
    float m_lifeMax;
    float m_speedMin;
    float m_speedMax;
    float m_spread;
    CEVec3 m_startColor01;
    CEVec3 m_endColor01;

    int m_maxParticles;
    float m_emitAccumulator;
    std::vector<Particle> m_particles;

    std::vector<CrashoutsEngine::Entity> m_entities;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_u01;

    int m_burstCount{1200};
    float m_cycleTimer{0.f};
    float m_cycleInterval{1.5f};
    bool m_inBurst{false};
};

class TrajectoryEmitter {
public:
    /*!
    @brief  Ballistic trajectory preview using evenly spaced circles.
    @details Computes a launch angle to reach the target with speed `m_speed`
             under gravity `m_g`, parameterizes motion with constant-accel
             kinematics, and samples the arc by length for uniform spacing.
             Sample count adapts to distance, mouse wheel density, and duck
             scale to prevent visual clumping.
    */
public:
    TrajectoryEmitter(int samples = 48);
    void Initialize(const CEVec3& origin, const CEVec3& target);
    void SetMass(float m);
    void SetSpeed(float v);
    void SetGravity(float g);
    void Update(float dt);
    void Render(bool useTexture = false,
                const std::string& texName = std::string{},
                const CEVec3& tint = CEVec3(1.f,1.f,1.f),
                const CEVec2& uvSize = CEVec2(1.f,1.f)) const;
    void Cleanup();
    const std::vector<CrashoutsEngine::Entity>& GetEntities() const;

private:
    void Recompute();

    CEVec3 m_origin;
    CEVec3 m_target;
    float m_mass{1.f};
    float m_speed{20.f};
    float m_g{9.8f};
    int m_samples{48};
    int m_minSamples{16};
    int m_maxSamples{256};
    float m_densityMultiplier{0.5f};
    float m_tFlight{0.f};
    float m_tAccum{0.f};
    float m_duckScale{1.f};

    std::vector<CEVec3> m_points;
    std::vector<CrashoutsEngine::Entity> m_entities;
    CrashoutsEngine::Entity m_projEntity{0};
};

}
