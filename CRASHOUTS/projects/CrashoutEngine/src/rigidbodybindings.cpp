/*!
@file		rigidbodybindings.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of rigid body bindings functions
*/
#include "rigidbodybindings.h"
#include "mono/metadata/loader.h"
#include "entitymanager.h"
#include "PhysicsManager.h"
#include "rigidbodycomponent.h"

using namespace CrashoutsEngine;
using namespace Component;

static void InternalAddForce(uint32_t entityID, float x, float y, float z)
{
    auto& ecs = *ECS::getInstance();
    if (!ecs.HasComponent<Component::RigidBodyComponent>(entityID)) {
        std::cout << "[Mono] AddForce: entity " << entityID << " has NO rigidbody\n";
        return;
    }
    ecs.GetComponent<Component::RigidBodyComponent>(entityID)->addForce({ x,y,z });
}

static bool EntityHasRigidBody(uint32_t entityID)
{
    auto& ecs = *ECS::getInstance();
    return ecs.HasComponent<Component::RigidBodyComponent>(entityID);
}

static float InternalGetMass(uint32_t entityID)
{
    auto& ecs = *ECS::getInstance();
    if (EntityHasRigidBody(entityID)) {
        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
        return rb->getMass();
    }
    return 0.f;
}


void CrashoutsEngine::Bindings::RegisterRigidBodyBindings()
{
    mono_add_internal_call("CrashoutsEngine.RigidBody::InternalAddForce", reinterpret_cast<const void*>(InternalAddForce));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasRigidBody", reinterpret_cast<const void*>(EntityHasRigidBody));
    mono_add_internal_call("CrashoutsEngine.RigidBody::InternalGetMass", reinterpret_cast<const void*>(InternalGetMass));
    std::cout << "[Mono] Registered: CrashoutsEngine.RigidBody::InternalAddForce" << std::endl;
}
