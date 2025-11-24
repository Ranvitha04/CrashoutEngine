/*!
@file        uibindings.cpp
@author      lee.r@digipen.edu
@date        24/11/2025
@brief       Definition of button bindings functions
*/

#include "uibindings.h"
#include "buttoncomponent.h"
#include "entitymanager.h"

#include "mono/metadata/loader.h"
#include "mono/metadata/appdomain.h"
#include "mono/metadata/object.h"

using namespace CrashoutsEngine;
using namespace Component;

namespace
{
    // get Button* from entity
    static ButtonComponent* GetButtonFromEntity(uint32_t entityID)
    {
        auto* ecs = ECS::getInstance();
        if (!ecs) return nullptr;
        return ecs->GetComponent<ButtonComponent>(entityID);
    }

	// internal calls
    /*
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
    */

    static bool InternalIsClicked(uint32_t id)
    {
        ButtonComponent* btn = GetButtonFromEntity(id);
        if (!btn) return false;

        return (btn->buttonState == ButtonState::PRESSED);
    }

    static bool InternalIsHovered(uint32_t id, MonoString* key)
    {
        ButtonComponent* btn = GetButtonFromEntity(id);
        if (!btn) return false;

        return (btn->buttonState == ButtonState::HOVERED);
    }
}

void CrashoutsEngine::Bindings::RegisterUIBindings()
{
    /*
    mono_add_internal_call("CrashoutsEngine.Animator::InternalPlay", (const void*)InternalPlay);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetBool", (const void*)InternalSetBool);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFloat", (const void*)InternalSetFloat);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetTrigger", (const void*)InternalSetTrigger);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalSetFlip", (const void*)InternalSetFlip);
    mono_add_internal_call("CrashoutsEngine.Animator::InternalGetCurrentState", (const void*)InternalGetCurrentState);
    */

    mono_add_internal_call("CrashoutsEngine.Button::InternalIsClicked", (const void*)InternalIsClicked);
    mono_add_internal_call("CrashoutsEngine.Button::InternalIsHovered", (const void*)InternalIsHovered);

}
