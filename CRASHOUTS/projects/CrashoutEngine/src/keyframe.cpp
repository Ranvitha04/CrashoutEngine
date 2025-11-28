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
#include "spritecomponent.h" // Assuming this holds Rows/Cols info
#include <cmath>

namespace {
    // Helper: Linear Interpolation
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
            return k1.value; // Snap behavior (good for sprites)
        }
        else {
            return Lerp(k1.value, k2.value, pct); // Smooth behavior (good for position)
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
        }
        return true;
    }

    void Update(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e, KeyframeAnimatorComponent& a, float dt) {
        if (!a.playing || a.current.empty()) return;

        // 1. Find Clip
        if (a.clips.find(a.current) == a.clips.end()) return;
        const AnimationClip& clip = a.clips.at(a.current);

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
                // We need the SpriteSheet info (Rows/Cols) to calculate UVs
                // Assuming you have a component that stores this?
                // Let's check for SpriteSheetComponent
                using namespace Component;

                // DEFAULT GRID (If no component found)
                int cols = 1;
                int rows = 1;

                // Try to find grid data (Modify this type if your component is named differently)
                if (ecs.HasComponent<SpriteSheetComponent>(e)) {
                    auto* sheet = ecs.GetComponent<SpriteSheetComponent>(e);
                    cols = std::max(1, sheet->cols); // Ensure no division by zero
                    rows = std::max(1, sheet->rows);
                }

                // 1. Calculate UV Size (1.0 / cols)
                float uSize = 1.0f / cols;
                float vSize = 1.0f / rows;

                // 2. Determine Row/Col for this Frame Index
                int frameIdx = (int)val;
                int r = frameIdx / cols;
                int c = frameIdx % cols;

                // 3. Calculate UV Offset
                float uOffset = c * uSize;
                float vOffset = r * vSize;

                // 4. Apply to ShapeComponent (Visuals)
                if (auto* shape = ecs.GetComponent<ShapeComponent>(e)) {
                    // Ensure the shape knows the tile size (Zoom)
                    shape->uvSize.x = uSize;
                    shape->uvSize.y = vSize;

                    // *** CRITICAL: You must have an offset field in ShapeComponent ***
                    // If you don't have 'uvOffset', you need to add it to ShapeComponent.h
                    // shape->uvOffset.x = uOffset;
                    // shape->uvOffset.y = vOffset;

                    // WORKAROUND if you lack uvOffset: Use WrapMode/Offset hacking or add the field.
                    // Ideally:
                    shape->uvOffset = { uOffset, vOffset };
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

