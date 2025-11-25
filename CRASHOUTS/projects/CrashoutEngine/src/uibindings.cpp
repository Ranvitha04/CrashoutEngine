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

    static bool InternalIsClicked(uint32_t id)
    {
        ButtonComponent* btn = GetButtonFromEntity(id);
        if (!btn) return false;

        return (btn->buttonState == ButtonState::PRESSED);
    }

    static bool InternalIsHovered(uint32_t id)
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

    mono_add_internal_call("CrashoutsEngine.InternalCall::InternalIsClicked", (const void*)InternalIsClicked);
    mono_add_internal_call("CrashoutsEngine.InternalCall::InternalIsHovered", (const void*)InternalIsHovered);

}
