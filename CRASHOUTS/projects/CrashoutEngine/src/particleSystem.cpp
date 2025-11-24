#include "particleSystem.h"
#include "transformcomponent.h"
#include "namecomponent.h"
#include "shapescomponent.h"
#include <algorithm>
#include <cmath>

namespace Component {

ParticleEmitter::ParticleEmitter(int maxCount)
    : m_origin(CEVec3(0.f,0.f,0.f)),
      m_gravity(CEVec3(0.f,-9.81f,0.f)),
      m_emitRate(500.f),
      m_lifeMin(0.8f),
      m_lifeMax(1.6f),
      m_speedMin(10.f),
      m_speedMax(25.f),
      m_startSize(0.4f),
      m_endSize(0.0f),
      m_startColor01(CEVec3(1.f,0.8f,0.2f)),
      m_endColor01(CEVec3(0.1f,0.1f,0.1f)),
      m_maxParticles(maxCount),
      m_emitAccumulator(0.f),
      m_u01(0.f,1.f)
{
    std::random_device rd;
    m_rng = std::mt19937(rd());
    m_particles.reserve(m_maxParticles);
}

FireEmitter::FireEmitter(int maxCount)
    : m_origin(CEVec3(0.f,0.f,0.f)),
      m_gravity(CEVec3(0.f,20.f,0.f)),
      m_emitRate(1500.f),
      m_lifeMin(0.4f),
      m_lifeMax(0.9f),
      m_speedMin(6.f),
      m_speedMax(14.f),
      m_startSize(0.6f),
      m_endSize(0.0f),
      m_startColor01(CEVec3(1.f,0.9f,0.4f)),
      m_endColor01(CEVec3(0.3f,0.05f,0.0f)),
      m_spread(0.6f),
      m_maxParticles(maxCount),
      m_emitAccumulator(0.f),
      m_u01(0.f,1.f)
{
    std::random_device rd;
    m_rng = std::mt19937(rd());
    m_particles.reserve(m_maxParticles);
}

void FireEmitter::Initialize(const CEVec3& origin) {
    m_origin = origin;
    m_particles.clear();
    if (m_entities.empty()) {
        for (int i = 0; i < m_maxParticles; ++i) {
            auto e = CrashoutsEngine::ECS::getInstance()->CreateEntity();
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "FireParticle_" + std::to_string(e) });
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ 0.f, CEVec2{0.f,0.f}, origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Rectangle;
            sc.color = CEVec3{ 255.f, 200.f, 100.f };
            sc.enableTexture = false;
            sc.uvSize = CEVec2{1.f,1.f};
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(e, sc);
            m_entities.push_back(e);
        }
    }
}

void FireEmitter::SetOrigin(const CEVec3& o) { m_origin = o; }
void FireEmitter::SetEmissionRate(float particlesPerSecond) { m_emitRate = particlesPerSecond; }
void FireEmitter::SetLifeRange(float minLife, float maxLife) { m_lifeMin = minLife; m_lifeMax = maxLife; }
void FireEmitter::SetSpeedRange(float minSpeed, float maxSpeed) { m_speedMin = minSpeed; m_speedMax = maxSpeed; }
void FireEmitter::SetGravity(const CEVec3& g) { m_gravity = g; }
void FireEmitter::SetSizeRange(float startSize, float endSize) { m_startSize = startSize; m_endSize = endSize; }
void FireEmitter::SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }
void FireEmitter::SetSpread(float radians) { m_spread = radians; }

void FireEmitter::Update(float dt) {
    m_emitAccumulator += m_emitRate * dt;
    int toEmit = static_cast<int>(std::floor(m_emitAccumulator));
    if (toEmit > 0) m_emitAccumulator -= static_cast<float>(toEmit);

    toEmit = std::min(toEmit, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));
    for (int i = 0; i < toEmit; ++i) {
        float base = 1.57079632679f;
        float ang = base + (m_u01(m_rng) * 2.f - 1.f) * m_spread;
        float speed = m_speedMin + (m_speedMax - m_speedMin) * m_u01(m_rng);
        CEVec3 vel(std::cos(ang) * speed * 0.4f, std::sin(ang) * speed, 0.f);
        float life = m_lifeMin + (m_lifeMax - m_lifeMin) * m_u01(m_rng);
        Particle p;
        p.position = m_origin;
        p.velocity = vel;
        p.color = m_startColor01;
        p.rotation = (m_u01(m_rng) * 2.f - 1.f) * 180.f;
        p.size = m_startSize * (0.8f + 0.4f * m_u01(m_rng));
        p.life = life;
        p.maxLife = life;
        m_particles.push_back(p);
    }

    for (auto& p : m_particles) {
        float jitterX = (m_u01(m_rng) * 2.f - 1.f) * 4.f;
        p.velocity.x += jitterX * dt;
        p.velocity.x *= 0.98f;

        p.velocity.x += m_gravity.x * dt;
        p.velocity.y += m_gravity.y * dt;
        p.velocity.z += m_gravity.z * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;
        p.life -= dt;
        float t = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        float size = m_startSize * (1.f - t) + m_endSize * t;
        p.size = size;
        p.color.x = m_startColor01.x * (1.f - t) + m_endColor01.x * t;
        p.color.y = m_startColor01.y * (1.f - t) + m_endColor01.y * t;
        p.color.z = m_startColor01.z * (1.f - t) + m_endColor01.z * t;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p){ return p.life <= 0.f; }),
        m_particles.end()
    );
}

void FireEmitter::Render(bool useTexture,
                         const std::string& texName,
                         const CEVec3& tint,
                         const CEVec2& uvSize) const {
    size_t count = m_particles.size();
    if (count == 0) return;

    for (size_t i = 0; i < count; ++i) {
        const Particle& p = m_particles[i];
        auto e = m_entities[i];
        auto* t = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(e);
        auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e);
        if (!t || !s) continue;
        t->position = p.position;
        t->rotation = p.rotation;
        t->size = CEVec2{ p.size * 0.9f, p.size * 1.6f };
        s->color = CEVec3{ p.color.x * 255.f, p.color.y * 255.f, p.color.z * 255.f };
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = tint;
    params.uvSize = uvSize;

    InstancingSystem::PopulateRectanglesFromEntities(m_entities, true);
    InstancingSystem::RenderInstancedRectangles(params);
}

FireballEmitter::FireballEmitter(int maxCount)
    : m_corePos(CEVec3(0.f,0.f,0.f)),
      m_coreVel(CEVec3(40.f,0.f,0.f)),
      m_coreRadius(4.f),
      m_coreColor(CEVec3(1.f,0.95f,0.7f)),
      m_gravity(CEVec3(0.f,0.f,0.f)),
      m_emitRate(1600.f),
      m_lifeMin(0.35f),
      m_lifeMax(0.85f),
      m_speedMin(8.f),
      m_speedMax(18.f),
      m_spread(0.5f),
      m_startColor01(CEVec3(1.f,0.8f,0.3f)),
      m_endColor01(CEVec3(0.4f,0.05f,0.0f)),
      m_worldW(200.f),
      m_worldH(100.f),
      m_maxParticles(maxCount),
      m_emitAccumulator(0.f),
      m_u01(0.f,1.f)
{
    std::random_device rd;
    m_rng = std::mt19937(rd());
    m_particles.reserve(m_maxParticles);
}

void FireballEmitter::Initialize(const CEVec3& origin) {
    m_corePos = origin;
    m_particles.clear();
    if (m_coreEntity == 0) {
        m_coreEntity = CrashoutsEngine::ECS::getInstance()->CreateEntity();
        CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(m_coreEntity, Component::NameComponent{ "FireballCore" });
        CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(m_coreEntity, Component::TransformComponent{ 0.f, CEVec2{1.f,1.f}, origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Circle;
            sc.color = CEVec3{ m_coreColor.x*255.f, m_coreColor.y*255.f, m_coreColor.z*255.f };
            sc.enableTexture = false;
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(m_coreEntity, sc);
    }
    if (m_entities.empty()) {
        for (int i = 0; i < m_maxParticles; ++i) {
            auto e = CrashoutsEngine::ECS::getInstance()->CreateEntity();
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "FireballTrail_" + std::to_string(e) });
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ 0.f, CEVec2{0.f,0.f}, origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Rectangle;
            sc.color = CEVec3{ 255.f, 200.f, 100.f };
            sc.enableTexture = false;
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(e, sc);
            m_entities.push_back(e);
        }
    }
}

void FireballEmitter::SetOrigin(const CEVec3& o) { m_corePos = o; }
void FireballEmitter::SetVelocity(const CEVec3& v) { m_coreVel = v; }
void FireballEmitter::SetRadius(float r) { m_coreRadius = r; }
void FireballEmitter::SetEmissionRate(float particlesPerSecond) { m_emitRate = particlesPerSecond; }
void FireballEmitter::SetLifeRange(float minLife, float maxLife) { m_lifeMin = minLife; m_lifeMax = maxLife; }
void FireballEmitter::SetSpeedRange(float minSpeed, float maxSpeed) { m_speedMin = minSpeed; m_speedMax = maxSpeed; }
void FireballEmitter::SetGravity(const CEVec3& g) { m_gravity = g; }
void FireballEmitter::SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }
void FireballEmitter::SetSpread(float radians) { m_spread = radians; }
void FireballEmitter::SetBounds(float worldW, float worldH) { m_worldW = worldW; m_worldH = worldH; }

void FireballEmitter::Update(float dt) {
    m_coreVel.x += m_gravity.x * dt;
    m_coreVel.y += m_gravity.y * dt;
    m_coreVel.z += m_gravity.z * dt;
    m_corePos.x += m_coreVel.x * dt;
    m_corePos.y += m_coreVel.y * dt;
    m_corePos.z += m_coreVel.z * dt;

    m_emitAccumulator += m_emitRate * dt;
    int toEmit = static_cast<int>(std::floor(m_emitAccumulator));
    if (toEmit > 0) m_emitAccumulator -= static_cast<float>(toEmit);
    toEmit = std::min(toEmit, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));

    float dir = (std::abs(m_coreVel.x) < 1e-4f && std::abs(m_coreVel.y) < 1e-4f) ? 1.57079632679f : std::atan2(m_coreVel.y, m_coreVel.x);
    float back = dir;
    for (int i = 0; i < toEmit; ++i) {
        float ang = back + (m_u01(m_rng) * 2.f - 1.f) * m_spread;
        float speed = m_speedMin + (m_speedMax - m_speedMin) * m_u01(m_rng);
        CEVec3 vel(std::cos(ang) * speed, std::sin(ang) * speed, 0.f);
        float life = m_lifeMin + (m_lifeMax - m_lifeMin) * m_u01(m_rng);
        Particle p;
        p.position = m_corePos;
        p.velocity = vel;
        p.color = m_startColor01;
        p.rotation = dir * 57.2957795131f;
        p.size = m_coreRadius * (0.9f + 0.4f * m_u01(m_rng));
        p.life = life;
        p.maxLife = life;
        m_particles.push_back(p);
    }

    for (auto& p : m_particles) {
        float jitter = (m_u01(m_rng) * 2.f - 1.f) * 8.f;
        p.velocity.x += jitter * dt;
        p.velocity.y *= 0.995f;
        p.velocity.x *= 0.99f;

        p.velocity.x += m_gravity.x * dt;
        p.velocity.y += m_gravity.y * dt;
        p.velocity.z += m_gravity.z * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;
        p.life -= dt;
        float t = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        float size = m_coreRadius * (1.f - t);
        p.size = size;
        p.color.x = m_startColor01.x * (1.f - t) + m_endColor01.x * t;
        p.color.y = m_startColor01.y * (1.f - t) + m_endColor01.y * t;
        p.color.z = m_startColor01.z * (1.f - t) + m_endColor01.z * t;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p){ return p.life <= 0.f; }),
        m_particles.end()
    );

    float halfW = m_worldW * 0.5f;
    float halfH = m_worldH * 0.5f;
    if (m_coreVel.y > std::abs(m_coreVel.x)) {
        if (m_corePos.y > halfH + 20.f) {
            m_corePos.y = -halfH - 15.f;
            m_corePos.x = (m_u01(m_rng) * 2.f - 1.f) * (halfW * 0.6f);
        }
    } else {
        if (m_corePos.x > halfW + 20.f || m_corePos.y > halfH + 20.f || m_corePos.y < -halfH - 20.f) {
            m_corePos.x = -halfW - 15.f;
            m_corePos.y = (m_u01(m_rng) * 2.f - 1.f) * (halfH * 0.6f);
        }
    }
}

void FireballEmitter::Render(bool useTexture,
                             const std::string& texName,
                             const CEVec3& tint,
                             const CEVec2& uvSize) const {
    size_t count = m_particles.size();
    for (size_t i = 0; i < count; ++i) {
        const Particle& p = m_particles[i];
        auto e = m_entities[i];
        auto* t = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(e);
        auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e);
        if (!t || !s) continue;
        t->position = p.position;
        t->rotation = p.rotation;
        t->size = CEVec2{ p.size * 0.9f, p.size * 1.6f };
        s->color = CEVec3{ p.color.x * 255.f, p.color.y * 255.f, p.color.z * 255.f };
    }

    auto* ct = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(m_coreEntity);
    auto* cs = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(m_coreEntity);
    if (ct && cs) {
        ct->position = m_corePos;
        ct->rotation = 0.f;
        ct->size = CEVec2{ m_coreRadius, m_coreRadius };
        cs->color = CEVec3{ m_coreColor.x*255.f, m_coreColor.y*255.f, m_coreColor.z*255.f };
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = tint;
    params.uvSize = uvSize;

    InstancingSystem::PopulateRectanglesFromEntities(m_entities, true);
    InstancingSystem::RenderInstancedRectangles(params);
    std::vector<CrashoutsEngine::Entity> coreIds; coreIds.push_back(m_coreEntity);
    InstancingSystem::RenderInstancedCirclesFromEntities(coreIds, true, 24, params);
}

void ParticleEmitter::Initialize(const CEVec3& origin) {
    m_origin = origin;
    m_particles.clear();
    if (m_entities.empty()) {
        for (int i = 0; i < m_maxParticles; ++i) {
            auto e = CrashoutsEngine::ECS::getInstance()->CreateEntity();
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Particles_" + std::to_string(e) });
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ 0.f, CEVec2{0.f,0.f}, origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Rectangle;
            sc.color = CEVec3{ 255.f, 255.f, 255.f };
            sc.enableTexture = false;
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(e, sc);
            m_entities.push_back(e);
        }
    }
}

void ParticleEmitter::SetOrigin(const CEVec3& o) { m_origin = o; }
void ParticleEmitter::SetEmissionRate(float particlesPerSecond) { m_emitRate = particlesPerSecond; }
void ParticleEmitter::SetLifeRange(float minLife, float maxLife) { m_lifeMin = minLife; m_lifeMax = maxLife; }
void ParticleEmitter::SetSpeedRange(float minSpeed, float maxSpeed) { m_speedMin = minSpeed; m_speedMax = maxSpeed; }
void ParticleEmitter::SetGravity(const CEVec3& g) { m_gravity = g; }
void ParticleEmitter::SetSizeRange(float startSize, float endSize) { m_startSize = startSize; m_endSize = endSize; }
void ParticleEmitter::SetColorRange(const CEVec3& startColor01, const CEVec3& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }

void ParticleEmitter::Update(float dt) {
    m_emitAccumulator += m_emitRate * dt;
    int toEmit = static_cast<int>(std::floor(m_emitAccumulator));
    if (toEmit > 0) m_emitAccumulator -= static_cast<float>(toEmit);

    toEmit = std::min(toEmit, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));
    for (int i = 0; i < toEmit; ++i) {
        float angle = m_u01(m_rng) * 6.28318530718f;
        float speed = m_speedMin + (m_speedMax - m_speedMin) * m_u01(m_rng);
        CEVec3 vel(std::cos(angle) * speed, std::sin(angle) * speed, 0.f);
        float life = m_lifeMin + (m_lifeMax - m_lifeMin) * m_u01(m_rng);
        Particle p;
        p.position = m_origin;
        p.velocity = vel;
        p.color = m_startColor01;
        p.rotation = 0.f;
        p.size = m_startSize;
        p.life = life;
        p.maxLife = life;
        m_particles.push_back(p);
    }

    for (auto& p : m_particles) {
        p.velocity.x += m_gravity.x * dt;
        p.velocity.y += m_gravity.y * dt;
        p.velocity.z += m_gravity.z * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;
        p.life -= dt;
        float t = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        float size = m_startSize * (1.f - t) + m_endSize * t;
        p.size = size;
        p.color.x = m_startColor01.x * (1.f - t) + m_endColor01.x * t;
        p.color.y = m_startColor01.y * (1.f - t) + m_endColor01.y * t;
        p.color.z = m_startColor01.z * (1.f - t) + m_endColor01.z * t;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p){ return p.life <= 0.f; }),
        m_particles.end()
    );
}

void ParticleEmitter::Render(bool useTexture,
                             const std::string& texName,
                             const CEVec3& tint,
                             const CEVec2& uvSize) const {
    size_t count = m_particles.size();
    if (count == 0) return;

    for (size_t i = 0; i < count; ++i) {
        const Particle& p = m_particles[i];
        auto e = m_entities[i];
        auto* t = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(e);
        auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e);
        if (!t || !s) continue;
        t->position = p.position;
        t->rotation = p.rotation;
        t->size = CEVec2{ p.size, p.size };
        s->color = CEVec3{ p.color.x * 255.f, p.color.y * 255.f, p.color.z * 255.f };
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = tint;
    params.uvSize = uvSize;

    InstancingSystem::PopulateRectanglesFromECS(true);
    InstancingSystem::RenderInstancedRectangles(params);
}

void FireEmitter::Cleanup() {
    for (auto e : m_entities) {
        CrashoutsEngine::ECS::getInstance()->DestroyEntity(e);
    }
    m_entities.clear();
}

const std::vector<CrashoutsEngine::Entity>& FireEmitter::GetEntities() const {
    return m_entities;
}

void FireEmitter::SyncEcs() {
    size_t count = m_particles.size();
    for (size_t i = 0; i < count; ++i) {
        const Particle& p = m_particles[i];
        auto e = m_entities[i];
        auto* t = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(e);
        auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e);
        if (!t || !s) continue;
        t->position = p.position;
        t->rotation = p.rotation;
        t->size = CEVec2{ p.size * 0.9f, p.size * 1.6f };
        s->color = CEVec3{ p.color.x * 255.f, p.color.y * 255.f, p.color.z * 255.f };
    }
}

void ParticleEmitter::Cleanup() {
    for (auto e : m_entities) {
        CrashoutsEngine::ECS::getInstance()->DestroyEntity(e);
    }
    m_entities.clear();
}

const std::vector<CrashoutsEngine::Entity>& ParticleEmitter::GetEntities() const {
    return m_entities;
}

void ParticleEmitter::SyncEcs() {
    size_t count = m_particles.size();
    for (size_t i = 0; i < count; ++i) {
        const Particle& p = m_particles[i];
        auto e = m_entities[i];
        auto* t = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(e);
        auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e);
        if (!t || !s) continue;
        t->position = p.position;
        t->rotation = p.rotation;
        t->size = CEVec2{ p.size, p.size };
        s->color = CEVec3{ p.color.x * 255.f, p.color.y * 255.f, p.color.z * 255.f };
    }
}

void FireballEmitter::Cleanup() {
    for (auto e : m_entities) {
        CrashoutsEngine::ECS::getInstance()->DestroyEntity(e);
    }
    m_entities.clear();
    if (m_coreEntity) {
        CrashoutsEngine::ECS::getInstance()->DestroyEntity(m_coreEntity);
        m_coreEntity = 0;
    }
}

const std::vector<CrashoutsEngine::Entity>& FireballEmitter::GetTrailEntities() const {
    return m_entities;
}

CrashoutsEngine::Entity FireballEmitter::GetCoreEntity() const {
    return m_coreEntity;
}

void FireballEmitter::SyncEcs() {
    size_t count = m_particles.size();
    for (size_t i = 0; i < count; ++i) {
        const Particle& p = m_particles[i];
        auto e = m_entities[i];
        auto* t = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(e);
        auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e);
        if (!t || !s) continue;
        t->position = p.position;
        t->rotation = p.rotation;
        t->size = CEVec2{ p.size * 0.9f, p.size * 1.6f };
        s->color = CEVec3{ p.color.x * 255.f, p.color.y * 255.f, p.color.z * 255.f };
    }
    auto* ct = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(m_coreEntity);
    auto* cs = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(m_coreEntity);
    if (ct && cs) {
        ct->position = m_corePos;
        ct->rotation = 0.f;
        ct->size = CEVec2{ m_coreRadius, m_coreRadius };
        cs->color = CEVec3{ m_coreColor.x*255.f, m_coreColor.y*255.f, m_coreColor.z*255.f };
    }
}

}