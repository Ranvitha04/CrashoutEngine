/*!
@file       particleSystem.h
@author     Ranvitha
@author     Renfred Lee
@author     Renfred (RTIS)
@date       26/11/2025
@brief      Declarations for particle and trajectory emitters.
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
        ParticleEmitter(int maxCount = 1000);

        void Initialize(const CEVec3& origin);
        void Update(float dt);
        void Render(bool useTexture = false,
            const std::string& texName = std::string{},
            const CEColor& tint = CEColor(1.f, 1.f, 1.f),
            const CEVec2& uvSize = CEVec2(1.f, 1.f)) const;

        void SetOrigin(const CEVec3& o);
        void SetEmissionRate(float particlesPerSecond);
        void SetLifeRange(float minLife, float maxLife);
        void SetSpeedRange(float minSpeed, float maxSpeed);
        void SetGravity(const CEVec3& g);
        void SetSizeRange(float startSize, float endSize);
        void SetColorRange(const CEColor& startColor01, const CEColor& endColor01);

        void Cleanup();

        const std::vector<CrashoutsEngine::Entity>& GetEntities() const;
        void SyncEcs();

        // ------------------------------------------------------------------
        // [EDITOR] Exposed Members (Moved to Public)
        // ------------------------------------------------------------------
        CEVec3 m_origin;
        CEVec3 m_gravity;
        float m_emitRate;
        float m_lifeMin;
        float m_lifeMax;
        float m_speedMin;
        float m_speedMax;
        float m_startSize;
        float m_endSize;
        CEColor m_startColor01;
        CEColor m_endColor01;

    private:
        struct Particle {
            CEVec3 position;
            CEVec3 velocity;
            CEColor color;
            float rotation;
            float size;
            float life;
            float maxLife;
        };

        int m_maxParticles;
        float m_emitAccumulator;
        std::vector<Particle> m_particles;

        std::vector<CEVec3> m_positions;
        std::vector<CEColor> m_colors;
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
        FireEmitter(int maxCount = 2000);
        void Initialize(const CEVec3& origin);
        void Update(float dt);
        void Render(bool useTexture = false,
            const std::string& texName = std::string{},
            const CEColor& tint = CEColor(1.f, 1.f, 1.f),
            const CEVec2& uvSize = CEVec2(1.f, 1.f)) const;

        void SetOrigin(const CEVec3& o);
        void SetEmissionRate(float particlesPerSecond);
        void SetLifeRange(float minLife, float maxLife);
        void SetSpeedRange(float minSpeed, float maxSpeed);
        void SetGravity(const CEVec3& g);
        void SetSizeRange(float startSize, float endSize);
        void SetColorRange(const CEColor& startColor01, const CEColor& endColor01);
        void SetSpread(float radians);

        void Cleanup();
        const std::vector<CrashoutsEngine::Entity>& GetEntities() const;
        void SyncEcs();

        // ------------------------------------------------------------------
        // [EDITOR] Exposed Members (Moved to Public)
        // ------------------------------------------------------------------
        CEVec3 m_origin;
        CEVec3 m_gravity;
        float m_emitRate;
        float m_lifeMin;
        float m_lifeMax;
        float m_speedMin;
        float m_speedMax;
        float m_startSize;
        float m_endSize;
        CEColor m_startColor01;
        CEColor m_endColor01;
        float m_spread;

    private:
        struct Particle {
            CEVec3 position;
            CEVec3 velocity;
            CEColor color;
            float rotation;
            float size;
            float life;
            float maxLife;
        };

        int m_maxParticles;
        float m_emitAccumulator;
        std::vector<Particle> m_particles;

        std::vector<CEVec3> m_positions;
        std::vector<CEColor> m_colors;
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
        FireballEmitter(int maxCount = 2500);
        void Initialize(const CEVec3& origin);
        void Update(float dt);
        void Render(bool useTexture = false,
            const std::string& texName = std::string{},
            const CEColor& tint = CEColor(1.f, 1.f, 1.f),
            const CEVec2& uvSize = CEVec2(1.f, 1.f)) const;

        void SetOrigin(const CEVec3& o);
        void SetVelocity(const CEVec3& v);
        void SetRadius(float r);
        void SetEmissionRate(float particlesPerSecond);
        void SetLifeRange(float minLife, float maxLife);
        void SetSpeedRange(float minSpeed, float maxSpeed);
        void SetGravity(const CEVec3& g);
        void SetColorRange(const CEColor& startColor01, const CEColor& endColor01);
        void SetSpread(float radians);
        void SetBounds(float worldW, float worldH);

        void Cleanup();

        const std::vector<CrashoutsEngine::Entity>& GetTrailEntities() const;
        CrashoutsEngine::Entity GetCoreEntity() const;
        void SyncEcs();

        // ------------------------------------------------------------------
        // [EDITOR] Exposed Members (Moved to Public)
        // ------------------------------------------------------------------
        CEVec3 m_corePos;
        CEVec3 m_coreVel;
        float  m_coreRadius;
        CEColor m_coreColor;

        CEVec3 m_gravity;
        float m_emitRate;
        float m_lifeMin;
        float m_lifeMax;
        float m_speedMin;
        float m_speedMax;
        float m_spread;
        CEColor m_startColor01;
        CEColor m_endColor01;
        float m_worldW;
        float m_worldH;

    private:
        struct Particle {
            CEVec3 position;
            CEVec3 velocity;
            CEColor color;
            float rotation;
            float size;
            float life;
            float maxLife;
        };

        int m_maxParticles;
        float m_emitAccumulator;
        mutable std::vector<Particle> m_particles;

        mutable std::vector<CEVec3> m_positions;
        mutable std::vector<CEColor> m_colors;
        mutable std::vector<float>  m_rotations;
        mutable std::vector<float>  m_scales;
        mutable std::vector<float>  m_widths;
        mutable std::vector<float>  m_heights;

        mutable std::vector<CEVec3> m_corePosArr;
        mutable std::vector<CEColor> m_coreColorArr;
        mutable std::vector<float>  m_coreRotArr;
        mutable std::vector<float>  m_coreScaleArr;
        mutable std::vector<float>  m_coreRadiusArr;

        std::mt19937 m_rng;
        std::uniform_real_distribution<float> m_u01;
        CrashoutsEngine::Entity m_coreEntity{ 0 };
        std::vector<CrashoutsEngine::Entity> m_entities;
    };

    class ExplosionEmitter {
    public:
        ExplosionEmitter(int maxCount = 2000);
        void Initialize(const CEVec3& origin);
        void Update(float dt);
        void Render(bool useTexture = false,
            const std::string& texName = std::string{},
            const CEColor& tint = CEColor(1.f, 1.f, 1.f),
            const CEVec2& uvSize = CEVec2(1.f, 1.f)) const;

        void SetOrigin(const CEVec3& o);
        void SetEmissionRate(float particlesPerSecond);
        void SetLifeRange(float minLife, float maxLife);
        void SetSpeedRange(float minSpeed, float maxSpeed);
        void SetGravity(const CEVec3& g);
        void SetColorRange(const CEColor& startColor01, const CEColor& endColor01);
        void SetSpread(float radians);
        void SetBurst(int count, float intervalSeconds);
        void TriggerBurst();

        void Cleanup();
        const std::vector<CrashoutsEngine::Entity>& GetEntities() const;
        void SyncEcs();

        // ------------------------------------------------------------------
        // [EDITOR] Exposed Members (Moved to Public)
        // ------------------------------------------------------------------
        CEVec3 m_origin;
        CEVec3 m_gravity;
        float m_emitRate;
        float m_lifeMin;
        float m_lifeMax;
        float m_speedMin;
        float m_speedMax;
        float m_spread;
        CEColor m_startColor01;
        CEColor m_endColor01;
        int m_burstCount{ 1200 };
        float m_cycleTimer{ 0.f };
        float m_cycleInterval{ 1.5f };

    private:
        struct Particle {
            CEVec3 position;
            CEVec3 velocity;
            CEColor color;
            float rotation;
            float size;
            float life;
            float maxLife;
        };

        int m_maxParticles;
        float m_emitAccumulator;
        std::vector<Particle> m_particles;

        std::vector<CrashoutsEngine::Entity> m_entities;
        std::mt19937 m_rng;
        std::uniform_real_distribution<float> m_u01;
        bool m_inBurst{ false };
    };

    class TrajectoryEmitter {
    public:
        TrajectoryEmitter(int samples = 48);
        void Initialize(const CEVec3& origin, const CEVec3& target);
        void SetMass(float m);
        void SetSpeed(float v);
        void SetGravity(float g);
        void Update(float dt);
        void Render(bool useTexture = false,
            const std::string& texName = std::string{},
            const CEColor& tint = CEColor(1.f, 1.f, 1.f),
            const CEVec2& uvSize = CEVec2(1.f, 1.f)) const;
        void Cleanup();
        const std::vector<CrashoutsEngine::Entity>& GetEntities() const;

        // ------------------------------------------------------------------
        // [EDITOR] Exposed Members (Moved to Public)
        // ------------------------------------------------------------------
        CEVec3 m_origin;
        CEVec3 m_target;
        float m_mass{ 1.f };
        float m_speed{ 20.f };
        float m_g{ 9.8f };
        int m_samples{ 48 };
        float m_duckScale{ 1.f };

    private:
        void Recompute();

        int m_minSamples{ 16 };
        int m_maxSamples{ 256 };
        float m_densityMultiplier{ 0.5f };
        float m_tFlight{ 0.f };
        float m_tAccum{ 0.f };

        std::vector<CEVec3> m_points;
        std::vector<CrashoutsEngine::Entity> m_entities;
        CrashoutsEngine::Entity m_projEntity{ 0 };
    };
}