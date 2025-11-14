/*!
@file		collidereventbindings.cpp
@author
@date
@brief		Mono bindings for collision event callbacks (enter/stay/exit)
*/

/*
#include "collidereventbindings.h"
#include "collidereventmanager.h"
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-runtime.h>

using namespace CrashoutsEngine;

static void RegisterCollideEnter(uint32_t entity, MonoDelegate* del)
{
    auto fn = (CrashoutsEngine::CollisionEventManager::CallbackFnCollide)mono_delegate_to_native(del);
    mono_gchandle_new((MonoObject*)del, true);

    CollisionEventManager::GetInstance().registerCollideEnter(entity, fn);
}

static void RegisterCollideStay(uint32_t entity, MonoDelegate* del)
{
    auto fn = (CrashoutsEngine::CollisionEventManager::CallbackFnCollide)mono_delegate_to_ftnptr(del);
    mono_gchandle_new((MonoObject*)del, true);

    CollisionEventManager::GetInstance().registerCollideStay(entity, fn);
}

static void RegisterCollideExit(uint32_t entity, MonoDelegate* del)
{
    auto fn = (CrashoutsEngine::CollisionEventManager::CallbackFnCollide)mono_delegate_to_ftnptr(del);
    mono_gchandle_new((MonoObject*)del, true);

    CollisionEventManager::GetInstance().registerCollideExit(entity, fn);
}

static void RegisterTriggerEnter(uint32_t entity, MonoDelegate* del)
{
    auto fn = (CrashoutsEngine::CollisionEventManager::CallbackTriggerFn)mono_delegate_to_ftnptr(del);
    mono_gchandle_new((MonoObject*)del, true);

    CollisionEventManager::GetInstance().registerTriggerEnter(entity, fn);
}

static void RegisterTriggerStay(uint32_t entity, MonoDelegate* del)
{
    auto fn = (CrashoutsEngine::CollisionEventManager::CallbackTriggerFn)mono_delegate_to_ftnptr(del);
    mono_gchandle_new((MonoObject*)del, true);

    CollisionEventManager::GetInstance().registerTriggerStay(entity, fn);
}

static void RegisterTriggerExit(uint32_t entity, MonoDelegate* del)
{
    auto fn = (CrashoutsEngine::CollisionEventManager::CallbackTriggerFn)mono_delegate_to_ftnptr(del);
    mono_gchandle_new((MonoObject*)del, true);

    CollisionEventManager::GetInstance().registerTriggerExit(entity, fn);
}


namespace CrashoutsEngine::Bindings
{
    void RegisterCollisionEventBindings()
    {
        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideEnter", (const void*)RegisterCollideEnter);
        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideStay", (const void*)RegisterCollideStay);
        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideExit", (const void*)RegisterCollideExit);

        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerEnter", (const void*)RegisterTriggerEnter);
        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerStay", (const void*)RegisterTriggerStay);
        mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerExit", (const void*)RegisterTriggerExit);
    }
}
*/
