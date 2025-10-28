#include "keyframe.h"
#include <cassert>
/*
// ---- Your engine headers (adjust include paths/names if different)
#include "entitymanager.h"  // ECS, components access
#include "shapes.h"         // TransformComponent, ShapeComponent

using namespace CrashoutsEngine;
using Component::TransformComponent;
using Component::ShapeComponent;

// ---------------- AnimationCurve ----------------
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Cubic Hermite (explicit tangents, Unity-like)
static float hermite(float p0, float m0, float p1, float m1, float t01, float dt) {
    const float t = t01;
    const float tt = t * t;
    const float ttt = tt * t;
    const float h00 = 2 * ttt - 3 * tt + 1;
    const float h10 = ttt - 2 * tt + t;
    const float h01 = -2 * ttt + 3 * tt;
    const float h11 = ttt - tt;
    return h00 * p0 + h10 * (m0 * dt) + h01 * p1 + h11 * (m1 * dt);
}

void AnimationCurve::Add(const KFKey& k) {
    keys.push_back(k);
    std::sort(keys.begin(), keys.end(), [](auto const& a, auto const& b) { return a.time < b.time; });
}

float AnimationCurve::Evaluate(float t) const {
    if (keys.empty()) return 0.f;
    if (t <= keys.front().time) return keys.front().value;
    if (t >= keys.back().time)  return keys.back().value;

    // find segment [i, i+1]
    size_t i = 0;
    while (i + 1 < keys.size() && keys[i + 1].time <= t) ++i;

    const KFKey& A = keys[i];
    const KFKey& B = keys[i + 1];

    const float dt = (B.time - A.time);
    const float u = dt > 0.f ? (t - A.time) / dt : 1.f;

    switch (A.interp) {
    case KFInterp::Constant: return A.value; // step
    case KFInterp::Linear:   return lerp(A.value, B.value, u);
    case KFInterp::Cubic:
    default:                 return hermite(A.value, A.outTan, B.value, B.inTan, std::clamp(u, 0.f, 1.f), dt);
    }
}

// ---------------- Property application ----------------
static void applyProperty(ECS& ecs, Entity e, KFProperty p, float v,
    const std::function<std::string(int)>& spriteResolver)
{
    switch (p) {
    case KFProperty::PosX:
    case KFProperty::PosY:
    case KFProperty::PosZ:
    case KFProperty::RotZ:
    case KFProperty::ScaleX:
    case KFProperty::ScaleY:
    {
        auto* T = ecs.GetComponent<TransformComponent>(e);
        if (!T) return;
        if (p == KFProperty::PosX)   T->position.x = v;
        if (p == KFProperty::PosY)   T->position.y = v;
        if (p == KFProperty::PosZ)   T->position.z = v;
        if (p == KFProperty::RotZ)   T->rotation = v;
        if (p == KFProperty::ScaleX) T->size.x = v;
        if (p == KFProperty::ScaleY) T->size.y = v;
        break;
    }
    case KFProperty::SpriteIndex:
    {
        if (!spriteResolver) return;
        auto* S = ecs.GetComponent<ShapeComponent>(e);
        if (!S) return;
        const int idx = (int)std::lround(v);
        const std::string tex = spriteResolver(idx);
        if (!tex.empty()) { S->enableTexture = true; S->textureName = tex; }
        break;
    }
    }
}

// ---------------- Animator runtime ----------------
namespace KeyframeUnity {

    KeyframeAnimatorComponent& AddAnimator(ECS& ecs, Entity e) {
        if (!ecs.HasComponent<KeyframeAnimatorComponent>(e))
            ecs.AddComponent<KeyframeAnimatorComponent>(e);
        return *ecs.GetComponent<KeyframeAnimatorComponent>(e);
    }

    void SetClip(KeyframeAnimatorComponent& a, const AnimationClip& in) {
        AnimationClip clip = in;
        for (auto& b : clip.bindings) {
            std::sort(b.curve.keys.begin(), b.curve.keys.end(),
                [](auto const& x, auto const& y) { return x.time < y.time; });
        }
        a.clips[clip.name] = std::move(clip);
    }

    bool Play(KeyframeAnimatorComponent& a, const std::string& name, bool restart) {
        if (!a.clips.count(name)) return false;
        if (restart || a.current != name) {
            a.current = name;
            a.t = 0.f;
            a.playing = true;
        }
        return true;
    }

    void Update(ECS& ecs, Entity e, KeyframeAnimatorComponent& a, float dt) {
        if (!a.playing) return;
        if (!a.clips.count(a.current)) return;

        AnimationClip& clip = a.clips.at(a.current);
        const float len = clip.Length();

        // advance & wrap/clamp
        a.t += dt;
        float localT = a.t;
        if (len > 0.f) {
            if (clip.wrap == KFWrap::Loop)  localT = std::fmod(localT, len);
            else                            localT = std::min(localT, len);
        }
        a.t = localT;

        // evaluate all bound curves
        for (auto const& b : clip.bindings) {
            const float v = b.curve.Evaluate(localT);
            applyProperty(ecs, e, b.property, v, a.spriteResolver);
        }

        // stop at end if Clamp
        if (clip.wrap == KFWrap::Clamp && len > 0.f && a.t >= len)
            a.playing = false;
    }

    void UpdateAll(CrashoutsEngine::ECS& ecs, float dt) {
        auto& store = ecs.GetComponentArray<KeyframeAnimatorComponent>();
        const auto& ids = store.indexToEntity;
        for (size_t i = 0; i < ids.size(); ++i) {
            CrashoutsEngine::Entity e = ids[i];
            if (!e) continue;
            if (auto* a = ecs.GetComponent<KeyframeAnimatorComponent>(e)) {
                Update(ecs, e, *a, dt);
            }
        }
    }



} // namespace KeyframeUnity
*/