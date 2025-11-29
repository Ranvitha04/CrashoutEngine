/*!
@file        keyframe.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/11/2025
@brief       Implements a simple keyframe animation system that drives transforms (position/rotation/scale) 
             and sprites over time using curves.
*/

#include "engine_pch.h"
#include "keyframe.h"
#include "entitymanager.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "spritecomponent.h" 
#include <cmath>

namespace {
    
    float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // Helper: Evaluate a single curve at time t
    float EvaluateCurve(const AnimationCurve& curve, float t) {
        if (curve.keys.empty()) return 0.0f;
        if (curve.keys.size() == 1) return curve.keys[0].value;

        // 1. Find the keyframe just before time t
        // (Simple linear search for now, binary search is better for large curves)
        size_t i = 0;
        while (i < curve.keys.size() - 1 && curve.keys[i + 1].time < t) {
            i++;
        }

        const KFKey& k1 = curve.keys[i];

        // If we are past the last key, clamp to last value
        if (i >= curve.keys.size() - 1) return k1.value;

        const KFKey& k2 = curve.keys[i + 1];

        // 2. Calculate percentage (0.0 to 1.0) between k1 and k2
        float range = k2.time - k1.time;
        float pct = (range > 0.00001f) ? (t - k1.time) / range : 0.0f;

        // 3. Interpolate (Constant vs Linear)
        if (k1.interp == KFInterp::Constant) {
            return k1.value; 
        }
        else {
            return Lerp(k1.value, k2.value, pct); 
        }
    }
}

void AnimationCurve::Add(const KFKey& k) {
    keys.push_back(k);

    // Lambda to sort keys by time (ascending)
    std::sort(keys.begin(), keys.end(), [](const KFKey& a, const KFKey& b) {
        return a.time < b.time;
        });
}

namespace KeyframeUnity {

    // Helper to get/add component
    KeyframeAnimatorComponent& AddAnimator(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) {
        if (!ecs.HasComponent<KeyframeAnimatorComponent>(e)) {
            ecs.AddComponent<KeyframeAnimatorComponent>(e);
        }
        return *ecs.GetComponent<KeyframeAnimatorComponent>(e);
    }

    // Helper to set clip (used by C# bindings potentially)
    void SetClip(KeyframeAnimatorComponent& a, const AnimationClip& clip) {
        a.clips[clip.name] = clip;
    }

    bool Play(KeyframeAnimatorComponent& a, const std::string& clipName, bool restart) {
        if (a.clips.find(clipName) == a.clips.end()) return false;

        if (a.current != clipName || restart) {
            a.current = clipName;
            a.t = 0.0f;
            a.playing = true;
        
            //const AnimationClip& clip = a.clips.at(clipName);
            a.needStateUpdate = true;
        }
        return true;
    }

    void Update(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e, KeyframeAnimatorComponent& a, float dt) {
        if (!a.playing || a.current.empty()) return;

        // 1. Find Clip
        if (a.clips.find(a.current) == a.clips.end()) return;
        const AnimationClip& clip = a.clips.at(a.current);

        // Apply State Changes (Texture / Size) once
        if (a.needStateUpdate) {
            using namespace Component;

            // Apply Texture & Grid Settings from the Animation Clip
            if (!clip.textureAsset.empty()) {
                // Update SpriteSheetComponent (Critical for UV Math)
                if (auto* sheet = ecs.GetComponent<SpriteSheetComponent>(e)) {
                    sheet->textureAsset = clip.textureAsset;
                    // Prevent division by zero
                    sheet->cols = std::max(1, clip.spriteCols);
                    sheet->rows = std::max(1, clip.spriteRows);
                }

                // Update ShapeComponent (Critical for Rendering)
                if (auto* shape = ecs.GetComponent<ShapeComponent>(e)) {
                    shape->textureName = clip.textureAsset;
                    shape->enableTexture = true;
                }
            }

            // Apply Size Settings
            if (clip.useCustomSize) {
                if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
                    t->size.x = clip.targetWidth;
                    t->size.y = clip.targetHeight;
                }
            }

            a.needStateUpdate = false;
        }

        // 2. Advance Time
        a.t += dt;
        float duration = clip.Length();

        // 3. Handle Wrapping (Loop vs Clamp)
        float evalTime = a.t;
        if (duration > 0.0001f) {
            if (clip.wrap == KFWrap::Loop) {
                evalTime = std::fmod(a.t, duration);
            }
            else {
                if (a.t > duration) evalTime = duration;
            }
        }

        // 4. Apply Bindings
        for (const auto& binding : clip.bindings) {
            float val = EvaluateCurve(binding.curve, evalTime);

            switch (binding.property) {
                // --- TRANSFORM ANIMATION ---
            case KFProperty::PosX:
                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) t->position.x = val;
                break;
            case KFProperty::PosY:
                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) t->position.y = val;
                break;
            case KFProperty::PosZ:
                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) t->position.z = val;
                break;
            case KFProperty::RotZ:
                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) t->rotation = val;
                break;
            case KFProperty::ScaleX:
                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) t->size.x = val;
                break;
            case KFProperty::ScaleY:
                if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) t->size.y = val;
                break;
                // --- SPRITE ANIMATION (The Important Part) ---
            case KFProperty::SpriteIndex:
            {
                using namespace Component;

                // A. Fetch current grid settings (now updated by the block above)
                int cols = 1;
                int rows = 1;

                // 2. PRIORITY: Use data from the AnimationClip if valid
                if (clip.spriteCols > 0 && clip.spriteRows > 0) {
                    cols = clip.spriteCols;
                    rows = clip.spriteRows;
                }
                // 3. FALLBACK: Check SpriteSheetComponent (Legacy support)
                else if (ecs.HasComponent<SpriteSheetComponent>(e)) {
                    auto* sheet = ecs.GetComponent<SpriteSheetComponent>(e);
                    cols = std::max(1, sheet->cols);
                    rows = std::max(1, sheet->rows);
                }

                // B. Determine Row and Column from Frame Index
                int frameIdx = (int)val; // e.g., 3

                // Standard Row-Major Layout calculation
                int r = frameIdx / cols; // 3 / 6 = 0 (Row 0)
                int c = frameIdx % cols; // 3 % 6 = 3 (Col 3)

                // C. Calculate Normalized UV Data (0.0 to 1.0)
                float uSize = 1.0f / cols; // 1.0 / 6 = 0.166
                float vSize = 1.0f / rows; // 1.0 / 4 = 0.25
                //std::cout << "Max col? " << cols << " Max rows? " << rows << std::endl;
                float uOffset = c * uSize; // 3 * 0.166 = 0.5

                // NOTE: Coordinate system matters here.
                // If Row 0 is Top, and UV 0 is Bottom, you might need: vOffset = (rows - 1 - r) * vSize;
                // For now, assuming Row 0 starts at UV 0.
                float vOffset = r * vSize;

                // D. Apply to Shape
                if (auto* shape = ecs.GetComponent<ShapeComponent>(e)) {
                    shape->uvSize.x = uSize;
                    shape->uvSize.y = vSize;
                    shape->uvOffset.x = uOffset;
                    shape->uvOffset.y = vOffset;
                }
                break;
            }
            }
        }
    }

    void UpdateAll(CrashoutsEngine::ECS& ecs, float dt) {
        auto& arr = ecs.GetComponentArray<KeyframeAnimatorComponent>();
        for (size_t i = 0; i < arr.dense.size(); ++i) {
            CrashoutsEngine::Entity e = arr.indexToEntity[i];
            Update(ecs, e, arr.dense[i], dt);
        }
    }
}

