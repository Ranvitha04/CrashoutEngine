/*!
@file		checkpointbindings.cpp
@author     m.biniswandi@digipen.edu
@brief		Definition of checkpoint bindings functions
*/
#include "checkpointbindings.h"
#include "checkpointcomponent.h"
#include "entitymanager.h"
#include "monomanager.h"

namespace CrashoutsEngine::Bindings
{
    static void InternalAddCheckpoint(uint32_t entityID)
    {
        ECS::getInstance()->AddComponent<Component::CheckpointComponent>(entityID);
    }

    static bool InternalEntityHasCheckpoint(uint32_t entityID)
    {
        return ECS::getInstance()->HasComponent<Component::CheckpointComponent>(entityID);
    }

    static void InternalRemoveCheckpoint(uint32_t entityID)
    {
        ECS::getInstance()->RemoveComponent<Component::CheckpointComponent>(entityID);
    }

    static void InternalCheckpoint_GetRespawnPosition(uint32_t entityID, CEVec3& outPos)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        if (comp) outPos = comp->respawnPosition;
    }

    static void InternalCheckpoint_SetRespawnPosition(uint32_t entityID, CEVec3& pos)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        if (comp) comp->respawnPosition = pos;
    }

    static uint32_t InternalCheckpoint_GetTargetEntity(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        return comp ? comp->targetRespawnEntity : 0;
    }

    static void InternalCheckpoint_SetTargetEntity(uint32_t entityID, uint32_t targetID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        if (comp) comp->targetRespawnEntity = targetID;
    }

    static MonoString* InternalCheckpoint_GetTargetName(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        if (!comp) return mono_string_new(MonoManager::GetDomain(), "");
        return mono_string_new(MonoManager::GetDomain(), comp->targetEntityName.c_str());
    }

    static void InternalCheckpoint_SetTargetName(uint32_t entityID, MonoString* name)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        if (comp && name) {
            char* cStr = mono_string_to_utf8(name);
            comp->targetEntityName = cStr;
            mono_free(cStr);
        }
    }

    static bool InternalCheckpoint_GetActivated(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        return comp ? comp->activated : false;
    }

    static void InternalCheckpoint_SetActivated(uint32_t entityID, bool activated)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::CheckpointComponent>(entityID);
        if (comp) comp->activated = activated;
    }

    void RegisterCheckpointBindings()
    {
        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddCheckpoint", (const void*)InternalAddCheckpoint);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasCheckpoint", (const void*)InternalEntityHasCheckpoint);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveCheckpoint", (const void*)InternalRemoveCheckpoint);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_GetRespawnPosition", (const void*)InternalCheckpoint_GetRespawnPosition);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_SetRespawnPosition", (const void*)InternalCheckpoint_SetRespawnPosition);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_GetTargetEntity", (const void*)InternalCheckpoint_GetTargetEntity);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_SetTargetEntity", (const void*)InternalCheckpoint_SetTargetEntity);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_GetTargetName", (const void*)InternalCheckpoint_GetTargetName);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_SetTargetName", (const void*)InternalCheckpoint_SetTargetName);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_GetActivated", (const void*)InternalCheckpoint_GetActivated);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Checkpoint_SetActivated", (const void*)InternalCheckpoint_SetActivated);
    }
}
