/*!
@file		entitybindings.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of entity bindings functions
*/
#include "entitymanager.h"
#include "transformcomponent.h"
#include "rigidbodycomponent.h"
#include "monomanager.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <iostream>

#include "collidercomponent.h"

namespace CrashoutsEngine {
    namespace Bindings {

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

            std::cout << "[Mono] Registered: CrashoutsEngine.Entity bindings\n";
        }

    } // namespace Bindings
} // namespace CrashoutsEngine