/*!
@file		rigidbodybindings.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of rigid body bindings functions

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "rigidbodybindings.h"
#include "mono/metadata/loader.h"
#include "entitymanager.h"
#include "PhysicsManager.h"
#include "rigidbodycomponent.h"
#include "physicsutils.h"

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

static void InternalGetMass(uint32_t entityID, float* outMass)
{
    auto& ecs = *ECS::getInstance();

    if (EntityHasRigidBody(entityID))
    {
        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
        *outMass = rb->getMass();
        return;
    }

    // If no rigidbody, return zero
    *outMass = 0.f;
}

static void InternalSetMass(uint32_t entityID, const float* newMass)
{
    auto& ecs = *ECS::getInstance();
    if (EntityHasRigidBody(entityID))
    {
        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
        rb->setMass(*newMass);
    }
}

static void InternalGetVelocity(uint32_t entityID, CEVec3* outVelocity)
{
    auto& ecs = *ECS::getInstance();

    if (EntityHasRigidBody(entityID))
    {
        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
        *outVelocity = rb->GetVelocity();
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

//static void InternalSetDynamic(uint32_t entityID, const bool newDynamic)
//{
//    auto& ecs = *ECS::getInstance();
//    if (EntityHasRigidBody(entityID))
//    {
//        auto* rb = ecs.GetComponent<Component::RigidBodyComponent>(entityID);
//        if (newDynamic) {
//            rb->getFlags().set(Physics::SIMULATE_DYNAMICS);
//        }
//        else rb->getFlags().reset(Physics::SIMULATE_DYNAMICS);
//    }
//}

void CrashoutsEngine::Bindings::RegisterRigidBodyBindings()
{
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalAddForce", reinterpret_cast<const void*>(InternalAddForce));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasRigidBody", reinterpret_cast<const void*>(EntityHasRigidBody));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalGetMass", reinterpret_cast<const void*>(InternalGetMass));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalSetMass", reinterpret_cast<const void*>(InternalSetMass));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalGetVelocity", reinterpret_cast<const void*>(InternalGetVelocity));
    mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalSetVelocity", reinterpret_cast<const void*>(InternalSetVelocity));
    //mono_add_internal_call("CrashoutsEngine.RigidBodyComponent::InternalSetDynamic", reinterpret_cast<const void*>(InternalSetDynamic));

    std::cout << "[Mono] Registered: CrashoutsEngine.RigidBodyComponent::InternalAddForce" << std::endl;
}
