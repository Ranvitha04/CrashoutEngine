/*!
@file        uibindings.cpp
@author      lee.r@digipen.edu
@date        24/11/2025
@brief       Definition of button bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "uibindings.h"
#include "buttoncomponent.h"
#include "textcomponent.h"
#include "sceneutils.h"
#include "entitymanager.h"
#include "monomanager.h"
#include "mono/metadata/loader.h"
#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>

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

    // --- Text / Dialogue Functions ---
    static void InternalPlayDialogue(uint32_t id, MonoString* file, MonoString* dialogueID) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return;
        TextComponent* txt = ecs->GetComponent<TextComponent>(id);
        if (txt) {
            char* fileC = mono_string_to_utf8(file);
            char* idC = mono_string_to_utf8(dialogueID);

            txt->dialogueFilePath = fileC;
            txt->dialogueID = idC;
            Scene::UI::LoadDialogue(*txt);

            mono_free(fileC);
            mono_free(idC);
        }
    }

    static void InternalLoadDialogue(uint32_t id) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return;
        TextComponent* txt = ecs->GetComponent<TextComponent>(id);
        if (txt) {
            Scene::UI::LoadDialogue(*txt);
        }
    }

    // --- Dynamic Text Functions ---
    static void InternalSetText(uint32_t id, MonoString* newText) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return;

        TextComponent* txt = ecs->GetComponent<TextComponent>(id);
        if (txt) {
            char* cStr = mono_string_to_utf8(newText);
            txt->text = cStr; // Assuming txt->text is a std::string
            mono_free(cStr);
        }
    }

    static MonoString* InternalGetText(uint32_t id) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return nullptr;

        TextComponent* txt = ecs->GetComponent<TextComponent>(id);
        if (txt) {
            // Create a new C# string from our C++ std::string
            return mono_string_new(MonoManager::GetDomain(), txt->text.c_str());
        }
        return mono_string_new(MonoManager::GetDomain(), "");
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

    // 3. Register Dialogue Functions
    mono_add_internal_call("CrashoutsEngine.InternalCalls::PlayDialogue", (const void*)InternalPlayDialogue);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::LoadDialogue", (const void*)InternalLoadDialogue);

    // 4. NEW: Register Text Functions
    mono_add_internal_call("CrashoutsEngine.InternalCalls::SetText", (const void*)InternalSetText);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::GetText", (const void*)InternalGetText);
}
