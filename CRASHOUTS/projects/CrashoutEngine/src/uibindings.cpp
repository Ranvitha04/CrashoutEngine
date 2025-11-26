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

using namespace CrashoutsEngine;
using namespace Component;

namespace
{
    // --- Helper to get Component ---
    static ButtonComponent* GetButtonFromEntity(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return nullptr;
        return ecs->GetComponent<ButtonComponent>(entityID);
    }

    // --- Logic Functions (For ButtonComponent.cs) ---
    static bool InternalIsClicked(uint32_t id) {
        ButtonComponent* btn = GetButtonFromEntity(id);
        // Ensure button exists AND is pressed
        return btn && (btn->buttonState == ButtonState::PRESSED);
    }

    static bool InternalIsHovered(uint32_t id) {
        ButtonComponent* btn = GetButtonFromEntity(id);
        // Ensure button exists AND is hovered
        return btn && (btn->buttonState == ButtonState::HOVERED);
    }

    // --- Lifecycle Functions (For InternalCalls.cs) ---
    static void InternalAddButton(uint32_t id) {
        ECS::getInstance()->AddComponent<ButtonComponent>(id);
    }

    static void InternalRemoveButton(uint32_t id) {
        ECS::getInstance()->RemoveComponent<ButtonComponent>(id);
    }

    static bool InternalEntityHasButton(uint32_t id) {
        return ECS::getInstance()->HasComponent<ButtonComponent>(id);
    }
}

void CrashoutsEngine::Bindings::RegisterUIBindings()
{
    // 1. Register Logic (Matches ButtonComponent class in C#)
    mono_add_internal_call("CrashoutsEngine.ButtonComponent::InternalIsClicked", (const void*)InternalIsClicked);
    mono_add_internal_call("CrashoutsEngine.ButtonComponent::InternalIsHovered", (const void*)InternalIsHovered);

    // 2. Register Lifecycle (Matches InternalCalls class in C#)
    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddButton", (const void*)InternalAddButton);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveButton", (const void*)InternalRemoveButton);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasButton", (const void*)InternalEntityHasButton);
}
