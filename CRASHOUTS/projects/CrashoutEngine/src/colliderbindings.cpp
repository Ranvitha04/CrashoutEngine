/*!
@file		colliderbindings.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of collider bindings functions
*/
#include "colliderbindings.h"
#include "collidercomponent.h"
#include "colliders.h"
#include "transformcomponent.h"
#include "entitymanager.h"

static void InternalUpdateRotation(uint32_t entityID, float rot)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    if (!ecs.HasComponent<Component::ColliderComponent>(entityID))
    {
        return;
    }

    auto& collider = ecs.GetComponent<Component::ColliderComponent>(entityID)->collider;
    if (collider->type == Collision::ColliderType::OBB)
    {
        auto& obb = dynamic_cast<Collision::OBBRect&>(*collider);
        obb.updateRotation(rot);
    }
}

static bool InternalHasCollider(uint32_t entityID)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    return ecs.HasComponent<Component::ColliderComponent>(entityID);
}


void CrashoutsEngine::Bindings::RegisterColliderBindings()
{
    mono_add_internal_call("CrashoutsEngine.Collider::InternalUpdateRotation",reinterpret_cast<const void*>(InternalUpdateRotation));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasCollider",reinterpret_cast<const void*>(InternalHasCollider));
}