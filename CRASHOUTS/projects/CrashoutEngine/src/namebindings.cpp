/*!
@file       namebindings.cpp
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      bindings for NameComponent

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

#include "namebindings.h"
#include "entitymanager.h"
#include "namecomponent.h"
#include "monomanager.h"

#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>
#include <iostream>

namespace CrashoutsEngine::Bindings
{
    // InternalCall Implementations

    static bool InternalHasName(uint32_t entityID)
    {
        return ECS::getInstance()->HasComponent<Component::NameComponent>(entityID);
    }

    static void InternalAddName(uint32_t entityID)
    {
        ECS::getInstance()->AddComponent<Component::NameComponent>(entityID, Component::NameComponent{});
    }

    static void InternalRemoveName(uint32_t entityID)
    {
        ECS::getInstance()->RemoveComponent<Component::NameComponent>(entityID);
    }

    static MonoString* InternalGetName(uint32_t entityID)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::NameComponent>(entityID);
        if (!comp)
            return nullptr;

        return mono_string_new(MonoManager::GetDomain(), comp->name.c_str());
    }

    static void InternalSetName(uint32_t entityID, MonoString* newName)
    {
        auto* comp = ECS::getInstance()->GetComponent<Component::NameComponent>(entityID);
        if (!comp)
            return;

        char* utf8 = mono_string_to_utf8(newName);
        comp->name = utf8;
        //fwee
        mono_free(utf8);
    }

	// register ze bindings
    void RegisterNameBindings()
    {
        mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasName",(const void*)InternalHasName);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::AddName",(const void*)InternalAddName);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveName",(const void*)InternalRemoveName);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::GetName",(const void*)InternalGetName);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::SetName",(const void*)InternalSetName);
        //std::cout << "[Mono] Registered: NameComponent bindings\n";
    }
}
