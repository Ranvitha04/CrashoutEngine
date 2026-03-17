/*!
@file       particleSystem.cpp
@author     Ranvitha
@author     Renfred Lee
@author     Renfred (RTIS)
@date       26/11/2025
@brief      Particle and trajectory emitters with instanced rendering.
@details    Implements multiple emitters (Fire, Fireball, Explosion, Trajectory)
            that drive ECS entities for GPU instanced rendering. Physics uses
            constant-acceleration kinematics and randomized sampling. The
            trajectory preview computes a launch angle to reach the mouse
            target, samples the ballistic curve by arc length for even spacing,
            and scales marker size with the duck.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "particleSystem.h"
#include "transformcomponent.h"
#include "namecomponent.h"
#include "shapescomponent.h"
#include "rigidbodycomponent.h"
#include "collidercomponent.h"
#include "colliders.h"
#include "inputmanager.h"
#include "CEMath.h"
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
      m_startColor01(CEColor(1.f,0.8f,0.2f)),
      m_endColor01(CEColor(0.1f,0.1f,0.1f)),
      m_maxParticles(maxCount),
      m_emitAccumulator(0.f),
      m_u01(0.f,1.f)
{
    std::random_device rd;
    m_rng = std::mt19937(rd());
    m_particles.reserve(m_maxParticles);
}

//normal fire emiiter 

FireEmitter::FireEmitter(int maxCount)
    : m_origin(CEVec3(0.f,0.f,0.f)),
      m_gravity(CEVec3(0.f,20.f,0.f)),
      m_emitRate(1500.f),
      m_lifeMin(0.4f),
      m_lifeMax(0.9f),
      m_speedMin(6.f),
      m_speedMax(0.5f),
      m_startSize(0.6f),
      m_endSize(0.0f),
      m_startColor01(CEColor(1.f,0.9f,0.4f)),
      m_endColor01(CEColor(0.3f,0.05f,0.0f)),
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
            sc.color = CEColor{ 255.f, 200.f, 100.f, 1.f };
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
void FireEmitter::SetColorRange(const CEColor& startColor01, const CEColor& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }
void FireEmitter::SetSpread(float radians) { m_spread = radians; }

/*! 
@brief    Update fire particles via explicit Euler integration.
@details  
- Emission uses a rate accumulator; new particles sample speed and direction
  around an upward base angle with spread.
- Motion: Euler under gravity (`v += g·dt`, `x += v·dt`) with slight X jitter.
- Visuals: lifetime decay and linear interpolation of size/color over age.
*/
void FireEmitter::Update(float dt) {
    m_emitAccumulator += m_emitRate * dt;
    int toEmit = static_cast<int>(std::floor(m_emitAccumulator));
    if (toEmit > 0) m_emitAccumulator -= static_cast<float>(toEmit);

    toEmit = std::min(toEmit, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));
    for (int i = 0; i < toEmit; ++i) {
        float base = 1.5f;
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
        p.color = CEColor(
            m_startColor01.r * (1.f - t) + m_endColor01.r * t,
            m_startColor01.g * (1.f - t) + m_endColor01.g * t,
            m_startColor01.b * (1.f - t) + m_endColor01.b * t);
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p){ return p.life <= 0.f; }),
        m_particles.end()
    );
}

/*! 
@brief   Push particle state into ECS entities and render instanced rectangles.
@details Each particle maps to one rectangle entity; transforms and colors are
         updated, then a single instanced draw submits the batch.
*/
void FireEmitter::Render(bool useTexture,
                         const std::string& texName,
                         const CEColor& tint,
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
        s->color = CEColor{ p.color.r * 255.f, p.color.g * 255.f, p.color.b * 255.f, 1.f };
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = CEColor(tint.r, tint.g, tint.b);
    params.uvSize = uvSize;

    InstancingSystem::PopulateRectanglesFromEntities(m_entities, true);
    InstancingSystem::RenderInstancedRectangles(params);
}

//fireball emiter using circles  

FireballEmitter::FireballEmitter(int maxCount)
    : m_corePos(CEVec3(0.f,0.f,0.f)),
      m_coreVel(CEVec3(40.f,0.f,0.f)),
      m_coreRadius(4.f),
      m_coreColor(CEColor(1.f,0.95f,0.7f)),
      m_gravity(CEVec3(0.f,0.f,0.f)),
      m_emitRate(1600.f),
      m_lifeMin(0.35f),
      m_lifeMax(0.85f),
      m_speedMin(8.f),
      m_speedMax(18.f),
      m_spread(0.5f),
      m_startColor01(CEColor(1.f,0.8f,0.3f)),
      m_endColor01(CEColor(0.4f,0.05f,0.0f)),
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

//exlposie emitter 

ExplosionEmitter::ExplosionEmitter(int maxCount)
    : m_origin(CEVec3(0.f,0.f,0.f)),
      m_gravity(CEVec3(0.f,0.f,0.f)),
      m_emitRate(100.f),
      m_lifeMin(0.1f),
      m_lifeMax(0.4f),
      m_speedMin(1.f),
      m_speedMax(3.f),
      m_spread(0.3f),
      m_startColor01(CEColor(1.f,0.5f,0.1f)),
      m_endColor01(CEColor(0.3f,0.1f,0.0f)),
      m_maxParticles(maxCount),
      m_emitAccumulator(0.f),
      m_u01(0.f,1.f)
{
    std::random_device rd;
    m_rng = std::mt19937(rd());
    m_particles.reserve(m_maxParticles);
}

//trajectory emitter for aiming, currently uses mouse input to draw it out

TrajectoryEmitter::TrajectoryEmitter(int samples)
    : m_samples(samples) {}

/*! 
@brief   Create preview circle entities and set initial origin/target.
@details One ECS entity per trajectory sample is created with circle shape and
         white tint for clear visualization; no projectile is spawned.
*/
void TrajectoryEmitter::Initialize(const CEVec3& origin, const CEVec3& target) {
    m_origin = origin;
    m_target = target;
    m_points.clear();
    if (m_entities.empty()) {
        for (int i = 0; i < m_samples; ++i) {
            auto e = CrashoutsEngine::ECS::getInstance()->CreateEntity();
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "TrajectoryPoint_" + std::to_string(e) });
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ 0.f, CEVec2{0.5f,0.5f}, origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Circle;
            sc.color = CEColor{ 0.7f, 0.6f, 1.0f, 1.0f };
            sc.enableTexture = false;
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(e, sc);
            m_entities.push_back(e);
        }
        // no projectile entity; preview-only points
    }
    Recompute();
}

void TrajectoryEmitter::SetMass(float m) { m_mass = m; }
void TrajectoryEmitter::SetSpeed(float v) { m_speed = v; }
void TrajectoryEmitter::SetGravity(float g) { m_g = g; }

/*! 
@brief   Recompute evenly spaced points along the ballistic trajectory.
@details Physics and sampling pipeline:
1) Target displacement: `x = target.x − origin.x`, `y = target.y − origin.y`
2) Angle solver (given speed `v` and gravity `g`)
3) Kinematics under constant acceleration:
4) Arc-length sampling:
   - Time-sample positions, accumulate segment length
*/
void TrajectoryEmitter::Recompute() {
    m_points.clear();
    // distance between mouse and target
    CEVec3 toTarget = CEVec3(m_target.x - m_origin.x, m_target.y - m_origin.y, 0.f);
    
    float deltaX = toTarget.x;
    float deltaY = toTarget.y;

    float dirSignX = (deltaX >= 0.f) ? 1.f : -1.f;
    float absDeltaX = std::abs(deltaX);
    //speed is predefined in .h
    float launchSpeed = std::max(0.1f, m_speed);
    float gravityAcc = std::max(0.001f, m_g);
    float launchSpeedSq = launchSpeed * launchSpeed;
    //solve tan theta to get the angle of the arc
    float discriminant = launchSpeedSq * launchSpeedSq - gravityAcc * (gravityAcc * absDeltaX * absDeltaX + 2.f * deltaY * launchSpeedSq);
    //default is 45 degrees
    float launchAngle = 0.25f * static_cast<float>(CEMath::PI);
    //as long as there is solution continue
    if (discriminant >= 0.f && absDeltaX > 0.0001f) {
        float sqrtDisc = std::sqrt(discriminant);
        //                              - sign for lower arc, change to + for higher arc
        float tanAngle = (launchSpeedSq - sqrtDisc) / (gravityAcc * absDeltaX);
        launchAngle = std::atan(tanAngle);
    }

    //calc horizontal direction of particle
    float velocityX = dirSignX * launchSpeed * std::cos(launchAngle);
    //call vertical direction of particle
    float velocityY = launchSpeed * std::sin(launchAngle);
    float flightTime = absDeltaX / std::max(0.0001f, std::abs(velocityX));
    m_tFlight = flightTime;

    //example sampl
    const int arcSampleCount = 256;
    std::vector<CEVec3> arcPositions; arcPositions.reserve(arcSampleCount);
    std::vector<float> arcLengthCum; arcLengthCum.reserve(arcSampleCount);
    float prevX = m_origin.x; float prevY = m_origin.y; float arcLenAccum = 0.f;

    //use time parameter to calculate evenly space points
    for (int i = 0; i < arcSampleCount; ++i) {
		// parameter of time where within the time specifed, it uses all the sample point between 0 and time so that its easier to calc how much time it takes 
        // for each point
        float tParam = (static_cast<float>(i) / static_cast<float>(arcSampleCount - 1)) * flightTime;
        // calc x poistion at each calcuted specified time
        float px = m_origin.x + velocityX * tParam;
        // calc y poistion at each specified time
        float py = m_origin.y + velocityY * tParam - 0.5f * gravityAcc * tParam * tParam;
        arcPositions.emplace_back(CEVec3(px, py, 0.f));
        if (i == 0) arcLengthCum.push_back(0.f);
        else {
            float segDX = px - prevX; float segDY = py - prevY;
            arcLenAccum += CEMath::Length(CEVec2(segDX, segDY));
            arcLengthCum.push_back(arcLenAccum);
        }
        prevX = px; prevY = py;
    }
    float totalArcLength = arcLengthCum.back();
    m_points.reserve(m_samples);
    //use binary search to find the actual closest point to be rendered based on above calcuaiotns
    for (int i = 0; i < m_samples; ++i) {
        //divide total arc length by sample points for equal diestanct between rach point
        float targetArcLen = (static_cast<float>(i) / static_cast<float>(m_samples - 1)) * totalArcLength;
        //binrary search to find the closest point to render it 
        auto it = std::lower_bound(arcLengthCum.begin(), arcLengthCum.end(), targetArcLen);
		//retrieve index from iterator
        size_t idx = static_cast<size_t>(std::distance(arcLengthCum.begin(), it));
        //clamp index by size of arc poirtion vector
        if (idx >= arcPositions.size()) idx = arcPositions.size() - 1;
        m_points.emplace_back(arcPositions[idx]);
    }
}

/*! 
@brief   Drive origin from duck, target from mouse; adapt density to scale.
@details 
- Grab duck transform for origin and scale; mouse world for target.
- Estimate marker radius from distance and duck scale; spacing ≈ `2.25×radius`.
- Target sample count ≈ `distance/spacing` scaled by wheel density, clamped to
  `[m_minSamples, m_maxSamples]`. Recreate ECS entities on change.
- Recompute ballistic points and Update entity transforms.
*/
void TrajectoryEmitter::Update(float dt) {
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    if (auto e1 = InstancingSystem::GetSingleDuckEntity()) {
        if (auto* t1 = ecs.GetComponent<Component::TransformComponent>(e1)) {
            m_origin = t1->position;
            m_duckScale = t1->size.x;
        }
    }
    {
        CEVec2 mouse = InputManager::GetMouseWorldPos();
        m_target = CEVec3(mouse.x, mouse.y, 0.f);
    }

    float mouseDX = m_target.x - m_origin.x;
    float mouseDY = m_target.y - m_origin.y;
    float mouseDistance = CEMath::Length(CEVec2(mouseDX, mouseDY));
    float wheelDelta = InputManager::GetMouseWheelDelta();
    if (wheelDelta != 0.f) {
        m_densityMultiplier = std::max(0.1f, m_densityMultiplier + wheelDelta * 0.1f);
    }
    float duckScale = std::max(0.1f, m_duckScale);
    float baseRadius = std::clamp(mouseDistance * 0.02f, 0.15f, 0.6f);
    float estimatedRadius = std::clamp(baseRadius * duckScale, 0.15f * duckScale, 0.6f * duckScale);
    float sampleSpacing = std::max(0.0001f, estimatedRadius * 2.25f);
    int dynamicMin = std::max(4, static_cast<int>(m_minSamples / std::max(1.0f, duckScale)));
    int targetSampleCount = std::clamp(static_cast<int>((mouseDistance * m_densityMultiplier) / sampleSpacing), dynamicMin, m_maxSamples);
    bool sampleCountChanged = targetSampleCount != m_samples;
    if (sampleCountChanged) m_samples = targetSampleCount;

    bool needRecreate = false;
    if (m_entities.size() != static_cast<size_t>(m_samples)) {
        needRecreate = true;
    } else {
        for (auto id : m_entities) {
            if (!ecs.HasComponent<Component::TransformComponent>(id)) {
                needRecreate = true; break;
            }
        }
    }
    if (needRecreate || sampleCountChanged) {
        for (auto id : m_entities) {
            CrashoutsEngine::ECS::getInstance()->DestroyEntity(id);
        }
        m_entities.clear();
        for (int i = 0; i < m_samples; ++i) {
            auto e = CrashoutsEngine::ECS::getInstance()->CreateEntity();
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "TrajectoryPoint_" + std::to_string(e) });
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ 0.f, CEVec2{0.6f,0.6f}, m_origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Circle;
            sc.color = CEColor{ 1.0f, 1.0f, 1.0f, 1.0f };
            sc.enableTexture = false;
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(e, sc);
            m_entities.push_back(e);
        }
    }

    Recompute();

    size_t renderCount = std::min(m_points.size(), m_entities.size());
    for (size_t i = 0; i < renderCount; ++i) {
        auto e = m_entities[i];
        if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) {
            t->position = m_points[i];
            float dirDX = m_target.x - m_origin.x;
            float dirDY = m_target.y - m_origin.y;
            float dirDist = CEMath::Length(CEVec2(dirDX, dirDY));
            float duckScaleNow = std::max(0.1f, m_duckScale);
            float baseRadiusNow = std::clamp(dirDist * 0.02f, 0.15f, 0.6f);
            float radiusNow = std::clamp(baseRadiusNow * duckScaleNow, 0.15f * duckScaleNow, 0.6f * duckScaleNow);
            t->size = CEVec2{ radiusNow, radiusNow };
        }
    }

    // no projectile Update

    // Advance loop timer and restart when flight time reached or projectile passes target x
    m_tAccum += dt;
    if (m_tAccum >= m_tFlight) {
        m_tAccum = 0.f;
    }
}

/*! 
@brief   Render trajectory points as instanced circles.
@details Updates instanced params (texture, tint, UV) and submits the batch
         with a segment budget derived from sample count for smooth discs.
*/
void TrajectoryEmitter::Render(bool useTexture,
                               const std::string& texName,
                               const CEColor& tint,
                               const CEVec2& uvSize) const {
    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = CEColor(tint.r, tint.g, tint.b);
    params.uvSize = uvSize;
    int segments = std::clamp(24 + m_samples / 2, 32, 96);
    InstancingSystem::RenderInstancedCirclesFromEntities(m_entities, true, segments, params);
}

/*! 
@brief   Destroy ECS entities owned by the trajectory preview.
*/
void TrajectoryEmitter::Cleanup() {
    for (auto e : m_entities) CrashoutsEngine::ECS::getInstance()->DestroyEntity(e);
    m_entities.clear();
    if (m_projEntity) CrashoutsEngine::ECS::getInstance()->DestroyEntity(m_projEntity);
    m_projEntity = 0;
}

const std::vector<CrashoutsEngine::Entity>& TrajectoryEmitter::GetEntities() const {
    return m_entities;
}

void ExplosionEmitter::Initialize(const CEVec3& origin) {
    m_origin = origin;
    m_particles.clear();
    if (m_entities.empty()) {
        for (int i = 0; i < m_maxParticles; ++i) {
            auto e = CrashoutsEngine::ECS::getInstance()->CreateEntity();
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "ExplosionParticle_" + std::to_string(e) });
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ 0.f, CEVec2{0.f,0.f}, origin });
            Component::ShapeComponent sc{};
            sc.type = Component::ShapeType::Circle;
            sc.color = CEColor{ 1.0f, 0.78f, 0.47f, 1.0f };
            sc.enableTexture = false;
            sc.preview = true;
            CrashoutsEngine::ECS::getInstance()->AddComponent<Component::ShapeComponent>(e, sc);
            m_entities.push_back(e);
        }
    }
}

void ExplosionEmitter::SetOrigin(const CEVec3& o) { m_origin = o; }
void ExplosionEmitter::SetEmissionRate(float particlesPerSecond) { m_emitRate = particlesPerSecond; }
void ExplosionEmitter::SetLifeRange(float minLife, float maxLife) { m_lifeMin = minLife; m_lifeMax = maxLife; }
void ExplosionEmitter::SetSpeedRange(float minSpeed, float maxSpeed) { m_speedMin = minSpeed; m_speedMax = maxSpeed; }
void ExplosionEmitter::SetGravity(const CEVec3& g) { m_gravity = g; }
void ExplosionEmitter::SetColorRange(const CEColor& startColor01, const CEColor& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }
void ExplosionEmitter::SetSpread(float radians) { m_spread = radians; }

void ExplosionEmitter::Update(float dt) {
    m_cycleTimer += dt;
    if (!m_inBurst && m_cycleTimer >= m_cycleInterval && m_particles.empty()) {
        TriggerBurst();
        m_inBurst = true;
        m_cycleTimer = 0.f;
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
        p.size = 0.6f + 0.9f * t;
        p.color.r = m_startColor01.r * (1.f - t) + m_endColor01.r * t;
        p.color.g = m_startColor01.g * (1.f - t) + m_endColor01.g * t;
        p.color.b = m_startColor01.b * (1.f - t) + m_endColor01.b * t;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p){ return p.life <= 0.f; }),
        m_particles.end()
    );

    if (m_inBurst && m_particles.empty()) {
        m_inBurst = false;
    }
}

void ExplosionEmitter::Render(bool useTexture,
                              const std::string& texName,
                              const CEColor& tint,
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
        float t01 = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        float alpha = std::max(0.f, 1.f - t01);
        s->color = CEColor{ p.color.r, p.color.g, p.color.b, alpha };
        s->preview = true;
    }

    for (size_t i = count; i < m_entities.size(); ++i) {
        auto e = m_entities[i];
        if (auto* s = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(e)) {
            s->preview = false;
        }
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = CEColor(tint.r, tint.g, tint.b);
    params.uvSize = uvSize;

    InstancingSystem::RenderInstancedCirclesFromEntities(m_entities, true, 64, params);
}

void ExplosionEmitter::Cleanup() {
    for (auto e : m_entities) {
        CrashoutsEngine::ECS::getInstance()->DestroyEntity(e);
    }
    m_entities.clear();
}

const std::vector<CrashoutsEngine::Entity>& ExplosionEmitter::GetEntities() const {
    return m_entities;
}

void ExplosionEmitter::SyncEcs() {
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
        float t01 = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        float alpha = std::max(0.f, 1.f - t01);
        s->color = CEColor{ p.color.r, p.color.g, p.color.b, alpha };
    }
}

void ExplosionEmitter::SetBurst(int count, float intervalSeconds) {
    m_burstCount = std::max(1, count);
    m_cycleInterval = std::max(0.1f, intervalSeconds);
}

void ExplosionEmitter::TriggerBurst() {
    int emitCount = std::min(m_burstCount, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));
    for (int i = 0; i < emitCount; ++i) {
        // Uniform angle in [0, 2π) for radial emission
        float angleRad = m_u01(m_rng) * (2.0f * static_cast<float>(CEMath::PI));
        // Initial speed sampled from range
        float speed = m_speedMin + (m_speedMax - m_speedMin) * m_u01(m_rng);
        // v0 = speed * [cos(angle), sin(angle)]
        CEVec3 initialVelocity(std::cos(angleRad) * speed, std::sin(angleRad) * speed, 0.f);
        // Lifetime assigned for constant-duration bursts
        float lifetime = m_lifeMax;
        Particle p;
        p.position = m_origin;
        p.velocity = initialVelocity;
        p.color = m_startColor01;
        p.rotation = 0.f;
        p.size = 1.0f * (0.8f + 0.4f * m_u01(m_rng));
        p.life = lifetime;
        p.maxLife = lifetime;
        m_particles.push_back(p);
    }
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
            sc.color = CEColor{ m_coreColor.r, m_coreColor.g, m_coreColor.b, 1.f };
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
            sc.type = Component::ShapeType::Circle;
            sc.color = CEColor{ 1.0f, 0.78f, 0.47f, 1.0f };
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
void FireballEmitter::SetColorRange(const CEColor& startColor01, const CEColor& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }
void FireballEmitter::SetSpread(float radians) { m_spread = radians; }
void FireballEmitter::SetBounds(float worldW, float worldH) { m_worldW = worldW; m_worldH = worldH; }

void FireballEmitter::Update(float dt) {
    // Explicit Euler integration for core under gravity: v += g·dt, x += v·dt
    m_coreVel.x += m_gravity.x * dt;
    m_coreVel.y += m_gravity.y * dt;
    m_coreVel.z += m_gravity.z * dt;
    m_corePos.x += m_coreVel.x * dt;
    m_corePos.y += m_coreVel.y * dt;
    m_corePos.z += m_coreVel.z * dt;

    // Emit count from rate via accumulator (Poisson-like) then clamp to capacity
    m_emitAccumulator += m_emitRate * dt;
    int emitCount = static_cast<int>(std::floor(m_emitAccumulator));
    if (emitCount > 0) m_emitAccumulator -= static_cast<float>(emitCount);
    emitCount = std::min(emitCount, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));

    // Heading angle from velocity: θ = atan2(vy, vx). Use heading for backward trail with spread
    float coreHeadingRad = (std::abs(m_coreVel.x) < 1e-4f && std::abs(m_coreVel.y) < 1e-4f) ? 0.5f * static_cast<float>(CEMath::PI) : std::atan2(m_coreVel.y, m_coreVel.x);
    float emitDirectionRad = coreHeadingRad;
    for (int i = 0; i < emitCount; ++i) {
        // Angle sampled around heading within spread
        float angleRad = emitDirectionRad + (m_u01(m_rng) * 2.f - 1.f) * m_spread;
        float speed = m_speedMin + (m_speedMax - m_speedMin) * m_u01(m_rng);
        // v0 = speed * [cos(angle), sin(angle)]
        CEVec3 initialVelocity(std::cos(angleRad) * speed, std::sin(angleRad) * speed, 0.f);
        float lifetime = m_lifeMin + (m_lifeMax - m_lifeMin) * m_u01(m_rng);
        Particle p;
        p.position = m_corePos;
        p.velocity = initialVelocity;
        p.color = m_startColor01;
        // Rotation in degrees from heading (θ·180/π)
        p.rotation = coreHeadingRad * 57.0f;
        p.size = m_coreRadius * (0.9f + 0.4f * m_u01(m_rng));
        p.life = lifetime;
        p.maxLife = lifetime;
        m_particles.push_back(p);
    }

    for (auto& p : m_particles) {
        // Apply X-axis jitter for flicker and mild damping
        float noiseX = (m_u01(m_rng) * 2.f - 1.f) * 8.f;
        p.velocity.x += noiseX * dt;
        p.velocity.y *= 0.995f;
        p.velocity.x *= 0.99f;

        // Particle Euler integration under gravity
        p.velocity.x += m_gravity.x * dt;
        p.velocity.y += m_gravity.y * dt;
        p.velocity.z += m_gravity.z * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;
        p.life -= dt;
        // Normalized age: age01 = 1 - life/maxLife
        float age01 = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        // Size decay linear in age01
        p.size = m_coreRadius * (1.f - age01);
        // Color linear interpolation: start*(1-age01) + end*age01
        p.color.r = m_startColor01.r * (1.f - age01) + m_endColor01.r * age01;
        p.color.g = m_startColor01.g * (1.f - age01) + m_endColor01.g * age01;
        p.color.b = m_startColor01.b * (1.f - age01) + m_endColor01.b * age01;
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
                             const CEColor& tint,
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
        t->size = CEVec2{ p.size, p.size };
        s->color = CEColor{ p.color.r, p.color.g, p.color.b, 1.f };
    }

    auto* ct = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(m_coreEntity);
    auto* cs = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(m_coreEntity);
    if (ct && cs) {
        ct->position = m_corePos;
        ct->rotation = 0.f;
        ct->size = CEVec2{ m_coreRadius, m_coreRadius };
        cs->color = CEColor{ m_coreColor.r, m_coreColor.g, m_coreColor.b, 1.f };
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = CEColor(tint.r, tint.g, tint.b);
    params.uvSize = uvSize;

    InstancingSystem::RenderInstancedCirclesFromEntities(m_entities, true, 64, params);
    std::vector<CrashoutsEngine::Entity> coreIds; coreIds.push_back(m_coreEntity);
    InstancingSystem::RenderInstancedCirclesFromEntities(coreIds, true, 64, params);
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
            sc.color = CEColor{ 255.f, 255.f, 255.f, 1.f };
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
void ParticleEmitter::SetColorRange(const CEColor& startColor01, const CEColor& endColor01) { m_startColor01 = startColor01; m_endColor01 = endColor01; }

void ParticleEmitter::Update(float dt) {
    // Emit count from rate via accumulator then clamp
    m_emitAccumulator += m_emitRate * dt;
    int emitCount = static_cast<int>(std::floor(m_emitAccumulator));
    if (emitCount > 0) m_emitAccumulator -= static_cast<float>(emitCount);

    emitCount = std::min(emitCount, std::max(0, m_maxParticles - static_cast<int>(m_particles.size())));
    for (int i = 0; i < emitCount; ++i) {
        
        float angleRad = m_u01(m_rng) * (2.0f * static_cast<float>(CEMath::PI));
        float speed = m_speedMin + (m_speedMax - m_speedMin) * m_u01(m_rng);
        
        CEVec3 initialVelocity(std::cos(angleRad) * speed, std::sin(angleRad) * speed, 0.f);
        float lifetime = m_lifeMin + (m_lifeMax - m_lifeMin) * m_u01(m_rng);
        Particle p;
        p.position = m_origin;
        p.velocity = initialVelocity;
        p.color = m_startColor01;
        p.rotation = 0.f;
        p.size = m_startSize;
        p.life = lifetime;
        p.maxLife = lifetime;
        m_particles.push_back(p);
    }

    for (auto& p : m_particles) {
        // Euler integration under gravity
        p.velocity.x += m_gravity.x * dt;
        p.velocity.y += m_gravity.y * dt;
        p.velocity.z += m_gravity.z * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;
        p.life -= dt;
        // Normalized age and linear size/color interpolation
        float age01 = 1.f - std::max(0.f, p.life) / std::max(0.0001f, p.maxLife);
        p.size = m_startSize * (1.f - age01) + m_endSize * age01;
        p.color.r = m_startColor01.r * (1.f - age01) + m_endColor01.r * age01;
        p.color.g = m_startColor01.g * (1.f - age01) + m_endColor01.g * age01;
        p.color.b = m_startColor01.b * (1.f - age01) + m_endColor01.b * age01;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p){ return p.life <= 0.f; }),
        m_particles.end()
    );
}

void ParticleEmitter::Render(bool useTexture,
                             const std::string& texName,
                             const CEColor& tint,
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
        s->color = CEColor{ p.color.r * 255.f, p.color.g * 255.f, p.color.b * 255.f, 1.f };
    }

    InstancingSystem::InstancedParams params{};
    params.textured = useTexture;
    params.textureName = texName;
    params.tint = CEColor(tint.r, tint.g, tint.b);
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
        t->size = CEVec2{ p.size, p.size };
        s->color = CEColor{ p.color.r, p.color.g, p.color.b, 1.f };
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
        s->color = CEColor{ p.color.r * 255.f, p.color.g * 255.f, p.color.b * 255.f, 1.f };
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
        s->color = CEColor{ p.color.r * 255.f, p.color.g * 255.f, p.color.b * 255.f, 1.f };
    }
    auto* ct = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::TransformComponent>(m_coreEntity);
    auto* cs = CrashoutsEngine::ECS::getInstance()->GetComponent<Component::ShapeComponent>(m_coreEntity);
    if (ct && cs) {
        ct->position = m_corePos;
        ct->rotation = 0.f;
        ct->size = CEVec2{ m_coreRadius, m_coreRadius };
        cs->color = CEColor{ m_coreColor.r, m_coreColor.g, m_coreColor.b, 1.f };
    }
}

}
