/*!
@file		entitybindings.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of entity bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "entitymanager.h"
#include "transformcomponent.h"
#include "rigidbodycomponent.h"
#include "namecomponent.h"
#include "monomanager.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <iostream>

#include "collidercomponent.h"
#include "entitybindings.h"

namespace CrashoutsEngine::Bindings
{

    static MonoObject* InternalGetTransform(uint32_t entityID) {
        auto& ecs = *ECS::getInstance();
        if (!ecs.HasComponent<Component::TransformComponent>(entityID))
            return nullptr;

        MonoClass* cls = mono_class_from_name(MonoManager::GetImage(), "CrashoutsEngine", "Transform");
        MonoObject* obj = mono_object_new(MonoManager::GetDomain(), cls);
        mono_runtime_object_init(obj);
        return obj;
    }

    static MonoObject* InternalGetRigidBody(uint32_t entityID) {
        auto& ecs = *ECS::getInstance();
        if (!ecs.HasComponent<Component::RigidBodyComponent>(entityID))
            return nullptr;

        MonoClass* cls = mono_class_from_name(MonoManager::GetImage(), "CrashoutsEngine", "RigidBody");
        MonoObject* obj = mono_object_new(MonoManager::GetDomain(), cls);
        mono_runtime_object_init(obj);
        return obj;
    }

    // Helper to find ID by Name
    static uint32_t InternalFindEntityByName(MonoString* name) {
        if (!name) return 0;

        char* cName = mono_string_to_utf8(name);
        std::string searchName(cName);
        mono_free(cName);

        auto* ecs = ECS::getInstance();
        // Access the NameComponent array directly for speed
        auto& nameArray = ecs->GetComponentArray<Component::NameComponent>();

        for (size_t i = 0; i < nameArray.dense.size(); ++i) {
            if (nameArray.dense[i]->name == searchName) {
                // Return the Entity ID associated with this component
                return nameArray.indexToEntity[i];
            }
        }

        return std::numeric_limits<uint32_t>::max(); // Not found
    }

	// add component functions
    static void InternalAddTransform(uint32_t entityID)
    {
        ECS::getInstance()->AddComponent<Component::TransformComponent>(entityID);
    }

    static void InternalAddRigidBody(uint32_t entityID)
    {
        ECS::getInstance()->AddComponent<Component::RigidBodyComponent>(entityID);
    }

    static void InternalAddCollider(uint32_t entityID)
    {
        ECS::getInstance()->AddComponent<Component::ColliderComponent>(entityID);
    }

	// remove component functions
    static void InternalRemoveTransform(uint32_t entityID)
    {
        ECS::getInstance()->RemoveComponent<Component::TransformComponent>(entityID);
    }

    static void InternalRemoveRigidBody(uint32_t entityID)
    {
        ECS::getInstance()->RemoveComponent<Component::RigidBodyComponent>(entityID);
    }

    static void InternalRemoveCollider(uint32_t entityID)
    {
        ECS::getInstance()->RemoveComponent<Component::ColliderComponent>(entityID);
    }

    void RegisterEntityBindings() {
        mono_add_internal_call("CrashoutsEngine.Entity::InternalGetTransform",(const void*)InternalGetTransform);
        mono_add_internal_call("CrashoutsEngine.Entity::InternalGetRigidBody",(const void*)InternalGetRigidBody);

        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddTransform", (const void*)InternalAddTransform);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddRigidBody", (const void*)InternalAddRigidBody);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddCollider", (const void*)InternalAddCollider);

        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveTransform", (const void*)InternalRemoveTransform);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveRigidBody", (const void*)InternalRemoveRigidBody);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveCollider", (const void*)InternalRemoveCollider);

        mono_add_internal_call("CrashoutsEngine.InternalCalls::FindEntityByName", (const void*)InternalFindEntityByName);

        std::cout << "[Mono] Registered: CrashoutsEngine.Entity bindings\n";
    }
}