/*!
@file      collidereventbindings.cpp
@brief     Mono bindings for collision event callbacks (enter/stay/exit)

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "collidereventbindings.h"
#include "collidereventmanager.h"

#include <functional>

#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>
#include "entitymanager.h"
#include <cstdint>


using namespace CrashoutsEngine;

// wrap a C# delegate into std::function
// i hate mono so much

namespace
{
    // Wraps a Mono delegate into a C++ std::function<void(Entity)> so CollisionEventManager can call it just like a native callback?? maybe
    static CrashoutsEngine::CollisionEventManager::CallbackFnCollide MakeManagedCollisionCallback(MonoDelegate* del)
    {
        if (!del)
            return {};

        MonoObject* obj = (MonoObject*)del;
        MonoClass* klass = mono_object_get_class(obj);
        if (!klass)
            return {};

        // Get the delegate's Invoke(...) method
        MonoMethod* invoke = mono_get_delegate_invoke(klass);
        if (!invoke)
            return {};

        // Pin the delegate so it will not be collected
        uint32_t handle = mono_gchandle_new(obj, /*pinned*/ true);

        // Capture handle + invoke in a lambda
        return [handle, invoke](Entity other)
            {
                MonoObject* target = mono_gchandle_get_target(handle);
                if (!target)
                    return;

                // Our C# delegate signature is: void Callback(uint otherEntity)
                uint32_t id = static_cast<uint32_t>(other);
                void* args[1];
                args[0] = &id;

                MonoObject* exc = nullptr;
                mono_runtime_invoke(invoke, target, args, &exc);

                if (exc)
                {

                }
            };
    }

    // COLLIDABLE EVENTS

    static void Internal_RegisterCollideEnter(uint32_t entity, MonoDelegate* del)
    {
        CollisionEventManager::CallbackFnCollide fn = MakeManagedCollisionCallback(del);
        CollisionEventManager::GetInstance().registerCollideEnter((Entity)entity, fn);
    }

    static void Internal_RegisterCollideStay(uint32_t entity, MonoDelegate* del)
    {
        CollisionEventManager::CallbackFnCollide fn = MakeManagedCollisionCallback(del);
        CollisionEventManager::GetInstance().registerCollideStay((Entity)entity, fn);
    }

    static void Internal_RegisterCollideExit(uint32_t entity, MonoDelegate* del)
    {
        CollisionEventManager::CallbackFnCollide fn = MakeManagedCollisionCallback(del);
        CollisionEventManager::GetInstance().registerCollideExit((Entity)entity, fn);
    }

    // TRIGGER EVENTS

    static void Internal_RegisterTriggerEnter(uint32_t entity, MonoDelegate* del)
    {
        CollisionEventManager::CallbackTriggerFn fn = MakeManagedCollisionCallback(del);
        CollisionEventManager::GetInstance().registerTriggerEnter((Entity)entity, fn);
    }

    static void Internal_RegisterTriggerStay(uint32_t entity, MonoDelegate* del)
    {
        CollisionEventManager::CallbackTriggerFn fn = MakeManagedCollisionCallback(del);
        CollisionEventManager::GetInstance().registerTriggerStay((Entity)entity, fn);
    }

    static void Internal_RegisterTriggerExit(uint32_t entity, MonoDelegate* del)
    {
        CollisionEventManager::CallbackTriggerFn fn = MakeManagedCollisionCallback(del);
        CollisionEventManager::GetInstance().registerTriggerExit((Entity)entity, fn);
    }
} // unnamed namespace


// Binding registration

namespace CrashoutsEngine::Bindings
{
    void RegisterCollisionEventBindings()
    {
        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideEnter",reinterpret_cast<const void*>(Internal_RegisterCollideEnter));

        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideStay",reinterpret_cast<const void*>(Internal_RegisterCollideStay));

        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideExit",reinterpret_cast<const void*>(Internal_RegisterCollideExit));

        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerEnter",reinterpret_cast<const void*>(Internal_RegisterTriggerEnter));

        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerStay",reinterpret_cast<const void*>(Internal_RegisterTriggerStay));

        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerExit",reinterpret_cast<const void*>(Internal_RegisterTriggerExit));
    }
}
