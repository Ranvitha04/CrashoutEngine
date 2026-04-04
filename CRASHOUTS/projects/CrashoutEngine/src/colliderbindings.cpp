/*!
@file        colliderbindings.cpp
@author     Shahrin Idzuan
@date       06/11/2025
@brief      Definition of collider bindings functions

All content 9 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "colliderbindings.h"
#include "collidercomponent.h"
#include "colliders.h"
#include "transformcomponent.h"
#include "entitymanager.h"
#include "monomanager.h"
#include "collisionmanager.h"

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
        obb.UpdateRotation(rot);
    }
}

static bool InternalGetEnabled(uint32_t entityID)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    if (!ecs.HasComponent<Component::ColliderComponent>(entityID))
    {
        return false;
    }

    auto& collider = ecs.GetComponent<Component::ColliderComponent>(entityID)->collider;
    return collider ? collider->enabled : false;
}

static void InternalSetEnabled(uint32_t entityID, bool enabled)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    if (!ecs.HasComponent<Component::ColliderComponent>(entityID))
    {
        return;
    }

    auto& collider = ecs.GetComponent<Component::ColliderComponent>(entityID)->collider;
    if (collider) collider->enabled = enabled;
}

static void InternalGetAABB(uint32_t entityID, CEVec3& outMin, CEVec3& outMax)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    if (!ecs.HasComponent<Component::ColliderComponent>(entityID))
    {
        outMin = CEVec3{};
        outMax = CEVec3{};
        return;
    }

    auto& collider = ecs.GetComponent<Component::ColliderComponent>(entityID)->collider;
    if (!collider || collider->type != Collision::ColliderType::AABB)
    {
        outMin = CEVec3{};
        outMax = CEVec3{};
        return;
    }

    auto* aabb = dynamic_cast<Collision::AABBRect*>(collider.get());
    if (!aabb)
    {
        outMin = CEVec3{};
        outMax = CEVec3{};
        return;
    }

    outMin = aabb->min;
    outMax = aabb->max;
}

static void InternalSetAABB(uint32_t entityID, CEVec3& min, CEVec3& max)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    if (!ecs.HasComponent<Component::ColliderComponent>(entityID))
    {
        return;
    }

    auto& collider = ecs.GetComponent<Component::ColliderComponent>(entityID)->collider;
    if (!collider || collider->type != Collision::ColliderType::AABB)
    {
        return;
    }

    auto* aabb = dynamic_cast<Collision::AABBRect*>(collider.get());
    if (!aabb) return;

    aabb->min = min;
    aabb->max = max;
    aabb->extents = (max - min) * 0.5f;
    aabb->offset = min + aabb->extents;
    aabb->position = aabb->offset;
    aabb->isAABBCacheDirty = true;

    if (ecs.HasComponent<Component::TransformComponent>(entityID))
    {
        auto* tf = ecs.GetComponent<Component::TransformComponent>(entityID);
        aabb->UpdateCollider(tf->position);
    }
}

static bool InternalHasCollider(uint32_t entityID)
{
    auto& ecs = *CrashoutsEngine::ECS::getInstance();
    return ecs.HasComponent<Component::ColliderComponent>(entityID);
}

static bool InternalRaycast(CEVec3& start, CEVec3& end, uint32_t& outEntityID, uint32_t ignoredEntity)
{
    CrashoutsEngine::Entity entity;
    bool hit = Collision::CollisionHandler::GetInstance().Raycast(start, end, entity, ignoredEntity);
    if (hit)
    {
        outEntityID = entity;
    }
    else
    {
        outEntityID = 0;
    }
    return hit;
}


void CrashoutsEngine::Bindings::RegisterColliderBindings()
{
    mono_add_internal_call("CrashoutsEngine.InternalCalls::Raycast", reinterpret_cast<const void*>(InternalRaycast));
    mono_add_internal_call("CrashoutsEngine.ColliderComponent::InternalUpdateRotation",reinterpret_cast<const void*>(InternalUpdateRotation));
    mono_add_internal_call("CrashoutsEngine.ColliderComponent::InternalGetEnabled",reinterpret_cast<const void*>(InternalGetEnabled));
    mono_add_internal_call("CrashoutsEngine.ColliderComponent::InternalSetEnabled",reinterpret_cast<const void*>(InternalSetEnabled));
    mono_add_internal_call("CrashoutsEngine.ColliderComponent::InternalGetAABB",reinterpret_cast<const void*>(InternalGetAABB));
    mono_add_internal_call("CrashoutsEngine.ColliderComponent::InternalSetAABB",reinterpret_cast<const void*>(InternalSetAABB));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasCollider",reinterpret_cast<const void*>(InternalHasCollider));
}
