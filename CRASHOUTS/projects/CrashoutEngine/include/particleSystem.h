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

}