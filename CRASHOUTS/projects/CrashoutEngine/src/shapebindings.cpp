/*!
@file       shapebindings.cpp
@author     Renfred Lee
@author     Ranvitha
@author     Shahrin Idzuan
@date       16/09/2025
@brief      definition of shape component bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
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

    static void InternalGetP1(uint32_t id, CEVec3* outP1) {
        if (!outP1) return;
        if (auto* s = GetShape(id)) *outP1 = s->p1;
        else *outP1 = CEVec3{ 0.0f, 0.0f, 0.0f };
    }

    static void InternalSetP1(uint32_t id, CEVec3* inP1) {
        if (!inP1) return;
        if (auto* s = GetShape(id)) s->p1 = *inP1;
    }

    static void InternalGetP2(uint32_t id, CEVec3* outP2) {
        if (!outP2) return;
        if (auto* s = GetShape(id)) *outP2 = s->p2;
        else *outP2 = CEVec3{ 0.0f, 0.0f, 0.0f };
    }

    static void InternalSetP2(uint32_t id, CEVec3* inP2) {
        if (!inP2) return;
        if (auto* s = GetShape(id)) s->p2 = *inP2;
    }

    static float InternalGetThickness(uint32_t id) {
        if (auto* s = GetShape(id)) return s->thickness;
        return 1.0f;
    }

    static void InternalSetThickness(uint32_t id, float thickness) {
        if (auto* s = GetShape(id)) s->thickness = thickness;
    }
}

void CrashoutsEngine::Bindings::RegisterShapeBindings() {
    // 1. Logic (Registered to Component Class)
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetColor", (const void*)InternalGetColor);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetColor", (const void*)InternalSetColor);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetAlpha", (const void*)InternalSetAlpha);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetType", (const void*)InternalGetType);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetType", (const void*)InternalSetType);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetP1", (const void*)InternalGetP1);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetP1", (const void*)InternalSetP1);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetP2", (const void*)InternalGetP2);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetP2", (const void*)InternalSetP2);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalGetThickness", (const void*)InternalGetThickness);
    mono_add_internal_call("CrashoutsEngine.ShapeComponent::InternalSetThickness", (const void*)InternalSetThickness);

    // 2. Lifecycle (Registered to InternalCalls)
    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddShape", (const void*)InternalAddShape);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveShape", (const void*)InternalRemoveShape);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasShape", (const void*)InternalEntityHasShape);
}
