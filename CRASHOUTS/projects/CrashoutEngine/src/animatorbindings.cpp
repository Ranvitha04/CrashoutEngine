/*!
@file        animatorbindings.cpp
@author      Renfred (40%) 
@author      Shahrin Idzuan (30%)
@author      Afiq (30%)
@date        30/11/2025
@brief       Definition of animator bindings functions (Updated for Keyframe System)

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "engine_pch.h"
#include "animatorbindings.h"

// Engine Systems
#include "keyframe.h" 
#include "entitymanager.h"
#include "monomanager.h"
#include "CEVec2.h"
#include "textcomponent.h"

// Mono Headers
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>

using namespace CrashoutsEngine;

namespace {

    // --- HELPER ---
    static KeyframeAnimatorComponent* GetKeyframeAnimator(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return nullptr;

        if (ecs->HasComponent<KeyframeAnimatorComponent>(entityID)) {
            return ecs->GetComponent<KeyframeAnimatorComponent>(entityID);
        }
        return nullptr;
    }

    // =============================================================
    // 1. LIFECYCLE (Matches [NativeComponent])
    // =============================================================

    static void AddKeyframeAnimator(uint32_t entityID) {
        // Uses the function from your header: KeyframeUnity::AddAnimator
        auto* ecs = ECS::getInstance();
        if (ecs) KeyframeUnity::AddAnimator(*ecs, entityID);
    }

    static void RemoveKeyframeAnimator(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        if (ecs && ecs->HasComponent<KeyframeAnimatorComponent>(entityID)) {
            ecs->RemoveComponent<KeyframeAnimatorComponent>(entityID);
        }
    }

    static bool EntityHasKeyframeAnimator(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        return ecs && ecs->HasComponent<KeyframeAnimatorComponent>(entityID);
    }

    // =============================================================
    // 2. METHODS
    // =============================================================

    // C#: void Play(string clipName, bool restart)
    static void InternalPlay(uint32_t entityID, MonoString* clipName, bool restart) {
        if (auto* ka = GetKeyframeAnimator(entityID)) {
            if (!clipName) return;

            char* str = mono_string_to_utf8(clipName);
            std::string cName = str ? str : "";
            mono_free(str);

            if (!cName.empty()) {
                // Call the Runtime API from your header
                KeyframeUnity::Play(*ka, cName, restart);
            }
        }
    }

    // C#: bool IsPlaying { get; }
    static bool InternalGetPlaying(uint32_t entityID) {
        if (auto* ka = GetKeyframeAnimator(entityID)) {
            return ka->playing;
        }
        return false;
    }

    // C#: bool IsPlaying { set; }
    static void InternalSetPlaying(uint32_t entityID, bool playing) {
        if (auto* ka = GetKeyframeAnimator(entityID)) {
            ka->playing = playing;
        }
    }

    // C#: string CurrentClip { get; }
    static MonoString* InternalGetCurrentClip(uint32_t entityID) {
        if (auto* ka = GetKeyframeAnimator(entityID)) {
            return mono_string_new(MonoManager::GetDomain(), ka->current.c_str());
        }
        return mono_string_new(MonoManager::GetDomain(), "");
    }

    // FOR TEXT 
    static bool Internal_IsTextFinished(uint32_t entityID) {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::TextComponent>(entityID)) return true; // Safe default

        auto* tc = ecs->GetComponent<Component::TextComponent>(entityID);

        // If we aren't in revealing mode, we are technically "finished" immediately
        if (!tc->isRevealing) return true;

        // Check if the revealed count has caught up to the total length
        return tc->revealedCharCount >= (int)tc->text.length();
    }

    static int Internal_GetTextRevealedCount(uint32_t entityID) {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (!ecs->HasComponent<Component::TextComponent>(entityID)) return 0;
        auto* tc = ecs->GetComponent<Component::TextComponent>(entityID);
        return tc->revealedCharCount;
    }

}

// =============================================================
// 3. REGISTRATION
// =============================================================

void CrashoutsEngine::Bindings::RegisterAnimatorBindings() {

    // Lifecycle Mapping
    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddKeyframeAnimator", (void*)AddKeyframeAnimator);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveKeyframeAnimator", (void*)RemoveKeyframeAnimator);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasKeyframeAnimator", (void*)EntityHasKeyframeAnimator);

    // Method Mapping
    // IMPORTANT: In your C# Animator class, you likely define these as:
    // [MethodImpl(MethodImplOptions.InternalCall)] private static extern void InternalPlay(uint id, string name, bool restart);

    mono_add_internal_call("CrashoutsEngine.Animator::InternalPlay", (void*)InternalPlay);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetPlaying", (void*)InternalGetPlaying);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetPlaying", (void*)InternalSetPlaying);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetCurrentClip", (void*)InternalGetCurrentClip);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::IsTextFinished", (const void*)Internal_IsTextFinished);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::GetTextRevealedCount", (const void*)Internal_GetTextRevealedCount);

    std::cout << "[Mono] Registered Keyframe Animator Bindings" << std::endl;
}