/*!
@file        animatorbindings.cpp
@author
@date
@brief       Definition of animator bindings functions
*/

 
#include "CEVec2.h"       
#include "animatorbindings.h"
#include "spriteAnimation.h"
#include "mono/metadata/loader.h"
#include "entitymanager.h"
#include "mono/metadata/appdomain.h"
#include "mono/metadata/object.h"

using namespace CrashoutsEngine;
using namespace Component;

namespace Component
{
    void SetSpriteSheet(CrashoutsEngine::Entity e,
        const std::string& assetName,
        int cols,
        int rows,
        CEVec2 worldSize);
}
namespace
{
    // get Animator* from entity
    static Animator* GetAnimatorFromEntity(uint32_t entityID)
    {
        auto* ecs = ECS::getInstance();
        if (!ecs) return nullptr;
        return ecs->GetComponent<Animator>(entityID);
    }
    // C# to C++: change the SpriteSheet for an entity
    static void InternalSetSpriteSheet(uint32_t id, MonoString* monoPath, int cols, int rows, float worldWidth, float worldHeight)
    {
        if (!monoPath)
            return;

        char* cstr = mono_string_to_utf8(monoPath);
        std::string path = cstr ? cstr : "";
        mono_free(cstr);

        if (path.empty())
            return;

        CEVec2 worldSize{ worldWidth, worldHeight };

       
        Component::SetSpriteSheet(
            static_cast<CrashoutsEngine::Entity>(id),path,cols, rows, worldSize      );
    }


	// internal calls

    static void InternalPlay(uint32_t id, MonoString* stateName)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return;

        std::string state = mono_string_to_utf8(stateName);
        anim->Play(state);
    }

    static void InternalSetBool(uint32_t id, MonoString* key, bool value)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return;

        std::string k = mono_string_to_utf8(key);
        anim->SetBool(k, value);
    }

    static void InternalSetFloat(uint32_t id, MonoString* key, float value)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return;

        std::string k = mono_string_to_utf8(key);
        anim->SetFloat(k, value);
    }

    static void InternalSetTrigger(uint32_t id, MonoString* key)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return;

        std::string k = mono_string_to_utf8(key);
        anim->SetTrigger(k);
    }

    static void InternalSetFlip(uint32_t id, bool flip)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return;

        anim->SetFlip(flip);
    }

    static MonoString* InternalGetCurrentState(uint32_t id)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return nullptr;

        return mono_string_new(mono_domain_get(), anim->GetCurrentState().c_str());
    }

    static bool InternalGetBool(uint32_t id, MonoString* key)
    {
        Animator* anim = GetAnimatorFromEntity(id);
        if (!anim) return false;

        std::string k = mono_string_to_utf8(key);
        return anim->GetBool(k);
    }


}

void CrashoutsEngine::Bindings::RegisterAnimatorBindings()
{
    mono_add_internal_call("CrashoutsEngine.Animator::InternalPlay", (const void*)InternalPlay);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetBool", (const void*)InternalSetBool);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFloat", (const void*)InternalSetFloat);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetTrigger", (const void*)InternalSetTrigger);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFlip", (const void*)InternalSetFlip);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetCurrentState", (const void*)InternalGetCurrentState);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetBool", (const void*)InternalGetBool);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetSpriteSheet", (const void*)InternalSetSpriteSheet);

}
