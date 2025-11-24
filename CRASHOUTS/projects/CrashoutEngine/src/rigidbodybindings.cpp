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

static void InternalGetVelocity(uint32_t entityID, CEVec3* outVelocity)
{
    auto& ecs = *ECS::getInstance();

    if (EntityHasRigidBody(entityID))
    {
        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
        *outVelocity = rb->getVelocity();
        return;
    }

    // If no rigidbody, return zero
    *outVelocity = CEVec3(0.f, 0.f, 0.f);
}

static void InternalSetVelocity(uint32_t entityID, const CEVec3* newVelocity)
{
    auto& ecs = *ECS::getInstance();
    if (EntityHasRigidBody(entityID))
    {
        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
        rb->setVelocity(*newVelocity);
    }
}


void CrashoutsEngine::Bindings::RegisterRigidBodyBindings()
{
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalAddForce", reinterpret_cast<const void*>(InternalAddForce));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasRigidBody", reinterpret_cast<const void*>(EntityHasRigidBody));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalGetMass", reinterpret_cast<const void*>(InternalGetMass));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalGetVelocity", reinterpret_cast<const void*>(InternalGetVelocity));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalSetVelocity", reinterpret_cast<const void*>(InternalSetVelocity));

    std::cout << "[Mono] Registered: CrashoutsEngine.RigidBodyComponent::InternalAddForce" << std::endl;
}
