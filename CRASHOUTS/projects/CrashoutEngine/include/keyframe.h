/*!
@file        keyframe.h
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Declares the keyframe() function that registers all the keyframe when it starts.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>


namespace CrashoutsEngine {
    using Entity = unsigned int;
    class ECS;
}

enum class KFInterp { Constant, Linear, Cubic };
enum class KFWrap { Loop, Clamp };

struct KFKey {
    float time = 0.f;     
    float value = 0.f;     
    float inTan = 0.f;     
    float outTan = 0.f;     
    KFInterp interp = KFInterp::Linear;
};

struct AnimationCurve {
    std::vector<KFKey> keys;  

    void  Add(const KFKey& k);
    //float Evaluate(float t) const;      
    float StartTime() const { return keys.empty() ? 0.f : keys.front().time; }
    float EndTime()   const { return keys.empty() ? 0.f : keys.back().time; }
};


enum class KFProperty {
    PosX, PosY, PosZ,
    RotZ,
    ScaleX, ScaleY,
    SpriteIndex   
};

struct KFBinding {
    KFProperty     property;
    AnimationCurve curve;
};

struct AnimationClip {
    std::string            name;
    KFWrap                 wrap = KFWrap::Loop;
    std::vector<KFBinding> bindings;

    std::string textureAsset = ""; // If set, switch texture when clip starts
    int spriteCols = 0;            // If > 0, update SpriteSheetComponent
    int spriteRows = 0;            // If > 0, update SpriteSheetComponent

    bool useCustomSize = false;    // Should we resize the entity?
    float targetWidth = 1.0f;      // New Scale/Size X
    float targetHeight = 1.0f;     // New Scale/Size Y

    float Length() const {
        float t = 0.f;
        for (auto const& b : bindings) t = std::max(t, b.curve.EndTime());
        return t;
    }
};

struct KeyframeAnimatorComponent {
    std::unordered_map<std::string, AnimationClip> clips;

    std::string current;    
    float       t = 0.f;    
    bool        playing = true;

    
    std::function<std::string(int)> spriteResolver;
    bool needStateUpdate{ true };
};

// ---------- Runtime API ----------
namespace KeyframeUnity {

    KeyframeAnimatorComponent& AddAnimator(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e);

    // Add/replace a clip
    void SetClip(KeyframeAnimatorComponent& a, const AnimationClip& clip);

    // Switch clip; restart when requested or when changing to a new clip
    bool Play(KeyframeAnimatorComponent& a, const std::string& clipName, bool restart = true);

    // Update a single entity�s animator
    void Update(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e, KeyframeAnimatorComponent& a, float dt);

    // Update all animators in ECS 
    void UpdateAll(CrashoutsEngine::ECS& ecs, float dt);
}
