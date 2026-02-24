/*!
@file       shapebindings.cpp
@author     lee.r@digipen.edu
@date       16/09/2025
@brief      definition of shape component bindings functions

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
*/
#include "shapebindings.h"
#include "shapescomponent.h" // Ensure this path is correct
#include "entitymanager.h"
#include "mono/metadata/loader.h"
#include "mono/metadata/object.h"
#include <string>

using namespace CrashoutsEngine;
using namespace Component;

namespace {

    // --- Helper ---
    static ShapeComponent* GetShape(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        if (ecs && ecs->HasComponent<ShapeComponent>(entityID)) {
            return ecs->GetComponent<ShapeComponent>(entityID);
        }
        return nullptr;
    }

    // --- Logic Functions ---

    // 1. Get Color
    static void InternalGetColor(uint32_t entityID, CEColor* outColor) {
        if (!outColor) return;

        auto* ecs = ECS::getInstance();
        if (ecs->HasComponent<Component::ShapeComponent>(entityID)) {
            auto* shape = ecs->GetComponent<Component::ShapeComponent>(entityID);
            *outColor = shape->color; // Copy the C++ color directly into C# memory
        }
        else {
            *outColor = CEColor{ 1.0f, 1.0f, 1.0f, 1.0f }; // Fallback white
        }
    }

    // 2. Set Color
    static void InternalSetColor(uint32_t entityID, CEColor* inColor) {
        if (!inColor) return;

        auto* ecs = ECS::getInstance();
        if (ecs->HasComponent<Component::ShapeComponent>(entityID)) {
            auto* shape = ecs->GetComponent<Component::ShapeComponent>(entityID);
            shape->color = *inColor; // Overwrite the C++ color
        }
    }

    static int InternalGetType(uint32_t id) {
        if (auto* s = GetShape(id)) {
            return static_cast<int>(s->type);
        }
        return 0; // Rectangle default
    }

    static void InternalSetType(uint32_t id, int type) {
        if (auto* s = GetShape(id)) {
            s->type = static_cast<ShapeType>(type);
        }
    }

    // --- Lifecycle Functions ---
    static void InternalAddShape(uint32_t id) {
        ECS::getInstance()->AddComponent<ShapeComponent>(id);
    }

    static void InternalRemoveShape(uint32_t id) {
        ECS::getInstance()->RemoveComponent<ShapeComponent>(id);
    }

    static bool InternalEntityHasShape(uint32_t id) {
        return ECS::getInstance()->HasComponent<ShapeComponent>(id);
    }

    // Dedicated Alpha Control
    static void InternalSetAlpha(uint32_t id, float alpha) {
        if (auto* s = GetShape(id)) {
            // Keep existing RGB, update only Alpha
            s->color.w() = alpha;
        }
    }
}

void CrashoutsEngine::Bindings::RegisterShapeBindings() {
    // 1. Logic (Registered to Component Class)
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetColor", (const void*)InternalGetColor);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetColor", (const void*)InternalSetColor);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetAlpha", (const void*)InternalSetAlpha);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetType", (const void*)InternalGetType);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetType", (const void*)InternalSetType);

    // 2. Lifecycle (Registered to InternalCalls)
    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddShape", (const void*)InternalAddShape);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveShape", (const void*)InternalRemoveShape);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasShape", (const void*)InternalEntityHasShape);
}