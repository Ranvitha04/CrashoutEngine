#if 0
/*!
@file        animatorbindings.cpp
@brief       Definition of animator bindings functions
*/

#include "animatorbindings.h"
#include "spriteAnimation.h"
#include "spritecomponent.h"
#include "CEVec2.h"
#include "entitymanager.h"

#include "mono/metadata/loader.h"
#include "mono/metadata/appdomain.h"
#include "mono/metadata/object.h"

using namespace CrashoutsEngine;
using namespace Component;

namespace
{
    // Get Animator* from ECS
    static Animator* GetAnimatorFromEntity(uint32_t entityID)
    {
        auto* ecs = ECS::getInstance();
        if (!ecs) return nullptr;
        return ecs->GetComponent<Animator>(entityID);
    }

    // C# -> C++: change the SpriteSheet for an entity
    static void InternalSetSpriteSheet(uint32_t id,
        MonoString* monoPath,
        int cols,
        int rows,
        float worldWidth,
        float worldHeight)
    {
        auto* ecs = ECS::getInstance();
        if (!ecs || !monoPath)
            return;

        char* cstr = mono_string_to_utf8(monoPath);
        std::string path = cstr ? cstr : "";
        mono_free(cstr);

        if (path.empty())
            return;

        CEVec2 worldSize{ worldWidth, worldHeight };

        // This uses your SetSpriteSheet helper in spriteAnimation.cpp
        Component::SetSpriteSheet(static_cast<Entity>(id),
            path,
            cols,
            rows,
            worldSize);
    }

    // ---------------- Animator parameter / state calls --------------

    static void InternalPlay(uint32_t id, MonoString* stateName)
    {
        if (!stateName) return;

        char* cstr = mono_string_to_utf8(stateName);
        if (!cstr) return;

        std::string state(cstr);
        mono_free(cstr);

        if (state.empty()) return;

        // Drive ECS SpriteAnimComponent instead of private Animator
        Component::AnimatorPlayOnSprite(static_cast<CrashoutsEngine::Entity>(id), state);
    }

    static void InternalSetBool(uint32_t id, MonoString* key, bool value)
    {
        auto* anim = GetAnimatorFromEntity(id);
        if (!anim || !key) return;

        char* cstr = mono_string_to_utf8(key);
        std::string k = cstr ? cstr : "";
        mono_free(cstr);
        if (k.empty()) return;

        anim->SetBool(k, value);
    }

    static void InternalSetFloat(uint32_t id, MonoString* key, float value)
    {
        auto* anim = GetAnimatorFromEntity(id);
        if (!anim || !key) return;

        char* cstr = mono_string_to_utf8(key);
        std::string k = cstr ? cstr : "";
        mono_free(cstr);
        if (k.empty()) return;

        anim->SetFloat(k, value);
    }

    static void InternalSetTrigger(uint32_t id, MonoString* key)
    {
        auto* anim = GetAnimatorFromEntity(id);
        if (!anim || !key) return;

        char* cstr = mono_string_to_utf8(key);
        std::string k = cstr ? cstr : "";
        mono_free(cstr);
        if (k.empty()) return;

        anim->SetTrigger(k);
    }

    static void InternalSetFlip(uint32_t id, bool flip)
    {
        if (auto* anim = GetAnimatorFromEntity(id))
            anim->SetFlip(flip); // optional native flip

        Component::AnimatorSetFlipOnSprite(static_cast<Entity>(id), flip);
    }

    static MonoString* InternalGetCurrentState(uint32_t id)
    {
        auto* anim = GetAnimatorFromEntity(id);
        if (!anim) return nullptr;

        return mono_string_new(mono_domain_get(),
            anim->GetCurrentState().c_str());
    }

    static bool InternalGetBool(uint32_t id, MonoString* key)
    {
        auto* anim = GetAnimatorFromEntity(id);
        if (!anim || !key) return false;

        char* cstr = mono_string_to_utf8(key);
        std::string k = cstr ? cstr : "";
        mono_free(cstr);
        if (k.empty()) return false;

        return anim->GetBool(k);
    }
}

// ------------ Registration ------------

void CrashoutsEngine::Bindings::RegisterAnimatorBindings()
{
    mono_add_internal_call("CrashoutsEngine.Animator::InternalPlay", (const void*)InternalPlay);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetBool", (const void*)InternalSetBool);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFloat", (const void*)InternalSetFloat);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetTrigger", (const void*)InternalSetTrigger);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFlip", (const void*)InternalSetFlip);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetCurrentState", (const void*)InternalGetCurrentState);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetBool", (const void*)InternalGetBool);

    // NEW – for switching sheets from C#
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetSpriteSheet",
        (const void*)InternalSetSpriteSheet);
}
#endif

#include "engine_pch.h"
#include "animatorbindings.h"

// Engine Systems
#include "keyframe.h" // <--- Include your new header here
#include "entitymanager.h"
#include "monomanager.h"

// Mono Headers
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

using namespace CrashoutsEngine;

// --- HELPER ---
static KeyframeAnimatorComponent* GetKeyframeAnimator(uint32_t entityID) {
    auto* ecs = ECS::getInstance();
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
    KeyframeUnity::AddAnimator(*ECS::getInstance(), entityID);
}

static void RemoveKeyframeAnimator(uint32_t entityID) {
    ECS::getInstance()->RemoveComponent<KeyframeAnimatorComponent>(entityID);
}

static bool EntityHasKeyframeAnimator(uint32_t entityID) {
    return ECS::getInstance()->HasComponent<KeyframeAnimatorComponent>(entityID);
}

// =============================================================
// 2. METHODS
// =============================================================

// C#: void Play(string clipName, bool restart)
static void InternalPlay(uint32_t entityID, MonoString* clipName, bool restart) {
    if (auto* ka = GetKeyframeAnimator(entityID)) {
        char* str = mono_string_to_utf8(clipName);

        // Call the Runtime API from your header
        KeyframeUnity::Play(*ka, std::string(str), restart);

        mono_free(str);
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

// =============================================================
// 3. REGISTRATION
// =============================================================

void CrashoutsEngine::Bindings::RegisterAnimatorBindings() {

    // Lifecycle Mapping
    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddKeyframeAnimator", reinterpret_cast<void*>(AddKeyframeAnimator));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveKeyframeAnimator", reinterpret_cast<void*>(RemoveKeyframeAnimator));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasKeyframeAnimator", reinterpret_cast<void*>(EntityHasKeyframeAnimator));

    // Method Mapping
    mono_add_internal_call("CrashoutsEngine.Animator::InternalPlay", reinterpret_cast<void*>(InternalPlay));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetPlaying", reinterpret_cast<void*>(InternalGetPlaying));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetPlaying", reinterpret_cast<void*>(InternalSetPlaying));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetCurrentClip", reinterpret_cast<void*>(InternalGetCurrentClip));

    std::cout << "[Mono] Registered Keyframe Animator Bindings" << std::endl;
}