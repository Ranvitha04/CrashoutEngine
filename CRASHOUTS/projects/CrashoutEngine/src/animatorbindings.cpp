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

// animatorbindings.cpp
#include "engine_pch.h"
#include "animatorbindings.h"

// Engine Systems
#include "entitymanager.h"
#include "monomanager.h"       // Need this for GetDomain()
#include "spriteAnimation.h"   // Your C++ Animator Component header

// Mono Headers
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>

using namespace CrashoutsEngine;
using namespace Component;

namespace {

    // --- HELPER: Get Component Safely ---
    static Animator* GetAnimator(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        if (ecs->HasComponent<Animator>(entityID)) {
            return ecs->GetComponent<Animator>(entityID);
        }
        // Warn if script tries to access missing component
        std::cout << "[Mono Warning] Entity " << entityID << " has no Animator!" << std::endl;
        return nullptr;
    }

    // =============================================================
    // 1. LIFECYCLE HANDLERS (Matches [NativeComponent] attributes)
    // =============================================================

    static void AddAnimator(uint32_t entityID) {
        ECS::getInstance()->AddComponent<Animator>(entityID);
    }

    static void RemoveAnimator(uint32_t entityID) {
        ECS::getInstance()->RemoveComponent<Animator>(entityID);
    }

    static bool EntityHasAnimator(uint32_t entityID) {
        return ECS::getInstance()->HasComponent<Animator>(entityID);
    }

    // =============================================================
    // 2. INTERNAL CALL IMPLEMENTATIONS
    // =============================================================

    // C#: private extern static void InternalPlay(uint entityID, string state);
    static void InternalPlay(uint32_t entityID, MonoString* stateName) {
        if (auto* anim = GetAnimator(entityID)) {
            char* str = mono_string_to_utf8(stateName); // Convert C# -> C++
            anim->Play(str);
            mono_free(str); // IMPORTANT: Free the temp string
        }
    }

    // C#: private extern static void InternalSetBool(uint entityID, string key, bool value);
    static void InternalSetBool(uint32_t entityID, MonoString* key, bool value) {
        if (auto* anim = GetAnimator(entityID)) {
            char* str = mono_string_to_utf8(key);
            anim->SetBool(str, value);
            mono_free(str);
        }
    }

    // C#: private extern static void InternalSetFloat(uint entityID, string key, float value);
    static void InternalSetFloat(uint32_t entityID, MonoString* key, float value) {
        if (auto* anim = GetAnimator(entityID)) {
            char* str = mono_string_to_utf8(key);
            anim->SetFloat(str, value);
            mono_free(str);
        }
    }

    // C#: private extern static void InternalSetTrigger(uint entityID, string key);
    static void InternalSetTrigger(uint32_t entityID, MonoString* key) {
        if (auto* anim = GetAnimator(entityID)) {
            char* str = mono_string_to_utf8(key);
            anim->SetTrigger(str);
            mono_free(str);
        }
    }

    // C#: private extern static void InternalSetFlip(uint entityID, bool flip);
    static void InternalSetFlip(uint32_t entityID, bool flip) {
        if (auto* anim = GetAnimator(entityID)) {
            anim->setFlipHorizontal(flip);
        }
    }

    // C#: private extern static string InternalGetCurrentState(uint entityID);
    static MonoString* InternalGetCurrentState(uint32_t entityID) {
        if (auto* anim = GetAnimator(entityID)) {
            std::string state = anim->GetCurrentState();
            // Convert C++ -> C# String
            return mono_string_new(MonoManager::GetDomain(), state.c_str());
        }
        return mono_string_new(MonoManager::GetDomain(), "");
    }

    // C#: private extern static bool InternalGetBool(uint entityID, string key);
    static bool InternalGetBool(uint32_t entityID, MonoString* key) {
        if (auto* anim = GetAnimator(entityID)) {
            char* str = mono_string_to_utf8(key);
            bool result = anim->GetBool(str);
            mono_free(str);
            return result;
        }
        return false;
    }
}

// =============================================================
// 3. REGISTRATION
// =============================================================

void CrashoutsEngine::Bindings::RegisterAnimatorBindings() {

    // Lifecycle
    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddAnimator", reinterpret_cast<void*>(AddAnimator));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveAnimator", reinterpret_cast<void*>(RemoveAnimator));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasAnimator", reinterpret_cast<void*>(EntityHasAnimator));

    // Methods
    // NOTE: The string MUST match "Namespace.Class::MethodName" exactly as written in C#
    mono_add_internal_call("CrashoutsEngine.Animator::InternalPlay", reinterpret_cast<void*>(InternalPlay));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetBool", reinterpret_cast<void*>(InternalSetBool));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFloat", reinterpret_cast<void*>(InternalSetFloat));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetTrigger", reinterpret_cast<void*>(InternalSetTrigger));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFlip", reinterpret_cast<void*>(InternalSetFlip));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetCurrentState", reinterpret_cast<void*>(InternalGetCurrentState));
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetBool", reinterpret_cast<void*>(InternalGetBool));

    std::cout << "[Mono] Registered Animator Bindings" << std::endl;
}