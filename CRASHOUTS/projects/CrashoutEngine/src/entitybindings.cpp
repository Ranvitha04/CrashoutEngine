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

        void RegisterEntityBindings() {
            mono_add_internal_call("CrashoutsEngine.Entity::InternalGetTransform",(const void*)InternalGetTransform);
            mono_add_internal_call("CrashoutsEngine.Entity::InternalGetRigidBody",(const void*)InternalGetRigidBody);
            std::cout << "[Mono] Registered: CrashoutsEngine.Entity bindings\n";
        }

    } // namespace Bindings
} // namespace CrashoutsEngine
