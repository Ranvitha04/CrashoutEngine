#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>
/*
// ---- Forward minimal engine types (adjust if your names differ)
namespace CrashoutsEngine {
    using Entity = unsigned int;
    class ECS;
}

enum class KFInterp { Constant, Linear, Cubic };
enum class KFWrap { Loop, Clamp };

struct KFKey {
    float time = 0.f;     // seconds
    float value = 0.f;     // generic numeric value (pos, angle, scale, spriteIndex)
    float inTan = 0.f;     // incoming tangent  (Cubic only)
    float outTan = 0.f;     // outgoing tangent (Cubic only)
    KFInterp interp = KFInterp::Linear;
};

struct AnimationCurve {
    std::vector<KFKey> keys;  // keep time-sorted

    void  Add(const KFKey& k);
    float Evaluate(float t) const;      // evaluate without wrapping
    float StartTime() const { return keys.empty() ? 0.f : keys.front().time; }
    float EndTime()   const { return keys.empty() ? 0.f : keys.back().time; }
};

// Which property a curve controls (expand as needed)
enum class KFProperty {
    PosX, PosY, PosZ,
    RotZ,
    ScaleX, ScaleY,
    SpriteIndex   // treated as integer-ish; rounded at apply time
};

struct KFBinding {
    KFProperty     property;
    AnimationCurve curve;
};

struct AnimationClip {
    std::string            name;
    KFWrap                 wrap = KFWrap::Loop;
    std::vector<KFBinding> bindings;

    float Length() const {
        float t = 0.f;
        for (auto const& b : bindings) t = std::max(t, b.curve.EndTime());
        return t;
    }
};

struct KeyframeAnimatorComponent {
    std::unordered_map<std::string, AnimationClip> clips;

    std::string current;    // active clip name
    float       t = 0.f;    // time in current clip
    bool        playing = true;

    // Optional: map sprite frame index -> texture name
    std::function<std::string(int)> spriteResolver;
};

// ---------- Runtime API ----------
namespace KeyframeUnity {

    // Ensure the component exists and return it
    KeyframeAnimatorComponent& AddAnimator(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e);

    // Add/replace a clip
    void SetClip(KeyframeAnimatorComponent& a, const AnimationClip& clip);

    // Switch clip; restart when requested or when changing to a new clip
    bool Play(KeyframeAnimatorComponent& a, const std::string& clipName, bool restart = true);

    // Update a single entity’s animator
    void Update(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e, KeyframeAnimatorComponent& a, float dt);

    // Update all animators in ECS (assumes your ECS exposes GetComponentArray<T>())
    void UpdateAll(CrashoutsEngine::ECS& ecs, float dt);
}
*/