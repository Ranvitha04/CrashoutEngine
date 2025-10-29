#include "TransformBindings.h"
#include "entitymanager.h"
#include "transformcomponent.h"
#include <mono/metadata/object.h>

using namespace CrashoutsEngine;
using namespace Component;

// Position CEVec3
static void TransformGetPosition(uint32_t entityID, CEVec3* outPos)
{
    ECS* ecs = ECS::getInstance();
    if (!ecs->HasComponent<TransformComponent>(entityID))
        return;

    auto* transform = ecs->GetComponent<TransformComponent>(entityID);
    *outPos = transform->position;
}

static void TransformSetPosition(uint32_t entityID, CEVec3* inPos)
{
    ECS* ecs = ECS::getInstance();
    if (!ecs->HasComponent<TransformComponent>(entityID))
        return;

    auto* transform = ecs->GetComponent<TransformComponent>(entityID);
    transform->position = *inPos;
}

// Rotation
static void TransformGetRotation(uint32_t entityID, float* outRot)
{
    ECS* ecs = ECS::getInstance();
    if (!ecs->HasComponent<TransformComponent>(entityID))
        return;

    auto* transform = ecs->GetComponent<TransformComponent>(entityID);
    *outRot = transform->rotation;
}

static void TransformSetRotation(uint32_t entityID, float* inRot)
{
    ECS* ecs = ECS::getInstance();
    if (!ecs->HasComponent<TransformComponent>(entityID))
        return;

    auto* transform = ecs->GetComponent<TransformComponent>(entityID);
    transform->rotation = *inRot;
}

// Size / Scale (CEVec2)
static void TransformGetSize(uint32_t entityID, CEVec2* outSize)
{
    ECS* ecs = ECS::getInstance();
    if (!ecs->HasComponent<TransformComponent>(entityID))
        return;

    auto* transform = ecs->GetComponent<TransformComponent>(entityID);
    *outSize = transform->size;
}

static void TransformSetSize(uint32_t entityID, CEVec2* inSize)
{
    ECS* ecs = ECS::getInstance();
    if (!ecs->HasComponent<TransformComponent>(entityID))
        return;

    auto* transform = ecs->GetComponent<TransformComponent>(entityID);
    transform->size = *inSize;
}

// Check if entity has transform
static bool EntityHasTransform(uint32_t entityID)
{
    ECS* ecs = ECS::getInstance();
    return ecs->HasComponent<TransformComponent>(entityID);
}

// Register Internal Calls
void CrashoutsEngine::Bindings::RegisterTransform()
{
    // Position
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TransformGetPosition", (const void*)TransformGetPosition);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TransformSetPosition", (const void*)TransformSetPosition);

    // Rotation
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TransformGetRotation", (const void*)TransformGetRotation);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TransformSetRotation", (const void*)TransformSetRotation);

    // Size / Scale
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TransformGetSize", (const void*)TransformGetSize);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::TransformSetSize", (const void*)TransformSetSize);

    // Component check
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasTransform", (const void*)EntityHasTransform);
}
