/*!
@file		triggerbindings.cpp
@author     Shahrin Idzuan
@brief		Definition of trigger bindings functions
*/
#include "triggerbindings.h"
#include "triggercomponent.h"
#include "entitymanager.h"
#include "monomanager.h"

namespace CrashoutsEngine::Bindings
{
    static void InternalAddTrigger(uint32_t entityID)
    {
        ECS::getInstance()->AddComponent<Component::TriggerComponent>(entityID);
    }

    static bool InternalEntityHasTrigger(uint32_t entityID)
    {
        return ECS::getInstance()->HasComponent<Component::TriggerComponent>(entityID);
    }

    static void InternalRemoveTrigger(uint32_t entityID)
    {
        ECS::getInstance()->RemoveComponent<Component::TriggerComponent>(entityID);
    }

    static MonoString* InternalTrigger_GetID(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::TriggerComponent>(entityID);
        if (!comp) return mono_string_new(MonoManager::GetDomain(), "");
        return mono_string_new(MonoManager::GetDomain(), comp->triggerID.c_str());
    }

    static void InternalTrigger_SetID(uint32_t entityID, MonoString* id)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::TriggerComponent>(entityID);
        if (comp && id) {
            char* cStr = mono_string_to_utf8(id);
            comp->triggerID = cStr;
            mono_free(cStr);
        }
    }

    static bool InternalTrigger_GetOneTimeUse(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::TriggerComponent>(entityID);
        return comp ? comp->oneTimeUse : false;
    }

    static void InternalTrigger_SetOneTimeUse(uint32_t entityID, bool oneTime)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::TriggerComponent>(entityID);
        if (comp) comp->oneTimeUse = oneTime;
    }

    static bool InternalTrigger_GetTriggered(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::TriggerComponent>(entityID);
        return comp ? comp->triggered : false;
    }

    static void InternalTrigger_SetTriggered(uint32_t entityID, bool triggered)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::TriggerComponent>(entityID);
        if (comp) comp->triggered = triggered;
    }

    void RegisterTriggerBindings()
    {
        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddTrigger", (const void*)InternalAddTrigger);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasTrigger", (const void*)InternalEntityHasTrigger);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveTrigger", (const void*)InternalRemoveTrigger);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Trigger_GetID", (const void*)InternalTrigger_GetID);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Trigger_SetID", (const void*)InternalTrigger_SetID);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Trigger_GetOneTimeUse", (const void*)InternalTrigger_GetOneTimeUse);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Trigger_SetOneTimeUse", (const void*)InternalTrigger_SetOneTimeUse);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Trigger_GetTriggered", (const void*)InternalTrigger_GetTriggered);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Trigger_SetTriggered", (const void*)InternalTrigger_SetTriggered);
    }
}
