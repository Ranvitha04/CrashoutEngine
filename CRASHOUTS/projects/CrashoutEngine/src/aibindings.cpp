/*!
@file       aibindings.cpp
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      aibindings.cpp file contains the function definitions for registering AI-related bindings in the CrashoutsEngine

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

#include "aimanager.h"
#include "aicomponent.h"
#include "state.h"
#include "entitymanager.h"

#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>



using namespace CrashoutsEngine;

namespace
{
	// ai manager func

    static void InternalAIManagerUpdate()
    {
        AIManager manager;
        manager.Update();
    }

    // fsm func

    static void InternalFSMSetState(uint32_t entity, int32_t stateID)
    {
        ECS& ecs = *ECS::getInstance();
        auto& ai = *ecs.GetComponent<Component::AIComponent>(entity);
        ai.fsm.SetCurrentState(stateID);
    }

    static int32_t InternalFSMGetState(uint32_t entity)
    {
        ECS& ecs = *ECS::getInstance();
        auto& ai = *ecs.GetComponent<Component::AIComponent>(entity);
        return ai.fsm.GetCurrentState().GetID();
    }

	// Helper that convert MonoDelegate to std::function<void()>
    static std::function<void()> MakeManagedCallbackVoid(MonoDelegate* del)
    {
        if (!del)
            return {};

        MonoObject* obj = (MonoObject*)del;
        MonoClass* klass = mono_object_get_class(obj);
        if (!klass)
            return {};

        MonoMethod* invoke = mono_get_delegate_invoke(klass);
        if (!invoke)
            return {};

        // pin the delegate so GC does not collect it
        uint32_t handle = mono_gchandle_new(obj, /* pinned */ true);

        return [handle, invoke]()
            {
                MonoObject* target = mono_gchandle_get_target(handle);
                if (!target)
                    return;

                void* args[1] = { nullptr };
                MonoObject* exc = nullptr;

                mono_runtime_invoke(invoke, target, args, &exc);

                if (exc)
                {
                    MonoString* msg = mono_object_to_string(exc, nullptr);
                    char* cmsg = mono_string_to_utf8(msg);
                    printf("[Mono Exception] %s\n", cmsg);
                    mono_free(cmsg);
                }
            };
    }

    // Assign ENTER callback
    static void InternalFSMSetEnter(uint32_t entity, int32_t stateID, MonoDelegate* del)
    {
        ECS& ecs = *ECS::getInstance();
        auto& ai = *ecs.GetComponent<Component::AIComponent>(entity);

        ai.fsm.GetState(stateID).enter = MakeManagedCallbackVoid(del);
    }

    // Assign UPDATE callback
    static void InternalFSMSetUpdate(uint32_t entity, int32_t stateID, MonoDelegate* del)
    {
        ECS& ecs = *ECS::getInstance();
        auto& ai = *ecs.GetComponent<Component::AIComponent>(entity);

        ai.fsm.GetState(stateID).Update = MakeManagedCallbackVoid(del);
    }

    // Assign EXIT callback
    static void InternalFSMSetExit(uint32_t entity, int32_t stateID, MonoDelegate* del)
    {
        ECS& ecs = *ECS::getInstance();
        auto& ai = *ecs.GetComponent<Component::AIComponent>(entity);

        ai.fsm.GetState(stateID).exit = MakeManagedCallbackVoid(del);
    }

}
namespace CrashoutsEngine::Bindings
{
    void RegisterAIBindings()
    {
        mono_add_internal_call("CrashoutsEngine.AI::Update",reinterpret_cast<const void*>(InternalAIManagerUpdate));
        mono_add_internal_call("CrashoutsEngine.AI::SetState", reinterpret_cast<const void*>(InternalFSMSetState));
        mono_add_internal_call("CrashoutsEngine.AI::GetState", reinterpret_cast<const void*>(InternalFSMGetState));
        mono_add_internal_call("CrashoutsEngine.AI::SetEnter", reinterpret_cast<const void*>(InternalFSMSetEnter));
        mono_add_internal_call("CrashoutsEngine.AI::SetUpdate", reinterpret_cast<const void*>(InternalFSMSetUpdate));
        mono_add_internal_call("CrashoutsEngine.AI::SetExit", reinterpret_cast<const void*>(InternalFSMSetExit));
    }
}

