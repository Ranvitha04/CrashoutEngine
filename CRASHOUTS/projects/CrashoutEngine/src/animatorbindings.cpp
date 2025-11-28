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
        std::string state = cstr ? cstr : "";
        mono_free(cstr);

        if (state.empty()) return;

        auto* ecs = ECS::getInstance();
        if (!ecs) return;

        auto* anim = ecs->GetComponent<Component::Animator>(id);
        if (!anim) {
            std::cout << "[Animator] No Animator component on entity " << id << "\n";
            return;
        }

        anim->Play(state);
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
