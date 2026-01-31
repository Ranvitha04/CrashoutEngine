/*!
@file      collidereventbindings.cpp
@author    m.biniswandi@digipen.edu
@date      29/11/2025
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
	static CollisionEventManager::CallbackFn MakeManagedCollisionCallback(MonoDelegate* del)
	{
		if (!del) return {};

		MonoObject* obj = (MonoObject*)del;
		MonoClass* klass = mono_object_get_class(obj);
		if (!klass) return nullptr;

		MonoMethod* invoke = mono_get_delegate_invoke(klass);
		if (!invoke) return nullptr;

		// Create the handle
		uint32_t rawHandle = mono_gchandle_new(obj, true);

		// Wrap handle in a shared_ptr that calls mono_gchandle_free when the std::function dies
		std::shared_ptr<uint32_t> handlePtr(new uint32_t(rawHandle), [](uint32_t* h) {
			mono_gchandle_free(*h);
			delete h;
			});

		return [handlePtr, invoke](Entity other)
		{
			// Access the handle via the shared_ptr
			MonoObject* target = mono_gchandle_get_target(*handlePtr);

			if (!target) return; // Domain might be dead

			uint32_t id = static_cast<uint32_t>(other);
			void* args[1];
			args[0] = &id;

			MonoObject* exc = nullptr;
			mono_runtime_invoke(invoke, target, args, &exc);

			if (exc) {
				// RETRIEVE AND PRINT THE ERROR MESSAGE
				MonoObject* other_exc = nullptr;
				MonoString* str = mono_object_to_string(exc, &other_exc);
				const char* mess = mono_string_to_utf8(str);

				std::cout << "[Mono Error] Collision Callback Failed: " << mess << std::endl;

				mono_free((void*)mess);
			}
		};
	}


	// Wraps a C# delegate that returns a BOOL (true = collide, false = ignore)
	static std::function<bool(Entity)> MakePreSolveCallback(MonoDelegate* del)
	{
		if (!del) return [](Entity) { return true; }; // Default to true if no delegate

		MonoObject* obj = (MonoObject*)del;
		MonoClass* klass = mono_object_get_class(obj);
		if (!klass) return [](Entity) { return true; };

		MonoMethod* invoke = mono_get_delegate_invoke(klass);
		if (!invoke) return [](Entity) { return true; };

		uint32_t handle = mono_gchandle_new(obj, /*pinned*/ true);

		return [handle, invoke](Entity other) -> bool
			{
				MonoObject* target = mono_gchandle_get_target(handle);
				if (!target) return true;

				// Prepare arguments
				uint32_t id = static_cast<uint32_t>(other);
				void* args[1];
				args[0] = &id;

				MonoObject* exc = nullptr;

				// --- CAPTURE THE RETURN VALUE ---
				MonoObject* resultObj = mono_runtime_invoke(invoke, target, args, &exc);

				if (exc) {
					// Log error if needed: mono_print_unhandled_exception(exc);
					return true; // Fail safe: collide normally
				}

				// Unbox the boolean result from C#
				if (resultObj) {
					return *(bool*)mono_object_unbox(resultObj);
				}

				return true;
			};
	};

	// Internal call for PreSolve
	static void Internal_RegisterPreSolve(uint32_t entity, MonoDelegate* del)
	{
		auto fn = MakePreSolveCallback(del);
		// You need to add this function to your CollisionEventManager!
		CollisionEventManager::GetInstance().RegisterPreSolve((Entity)entity, fn);
	}

	// COLLIDABLE EVENTS

	static void Internal_RegisterCollideEnter(uint32_t entity, MonoDelegate* del)
	{
		CollisionEventManager::CallbackFn fn = MakeManagedCollisionCallback(del);
		CollisionEventManager::GetInstance().RegisterCollideEnter((Entity)entity, fn);
	}

	static void Internal_RegisterCollideStay(uint32_t entity, MonoDelegate* del)
	{
		CollisionEventManager::CallbackFn fn = MakeManagedCollisionCallback(del);
		CollisionEventManager::GetInstance().RegisterCollideStay((Entity)entity, fn);
	}

	static void Internal_RegisterCollideExit(uint32_t entity, MonoDelegate* del)
	{
		CollisionEventManager::CallbackFn fn = MakeManagedCollisionCallback(del);
		CollisionEventManager::GetInstance().RegisterCollideExit((Entity)entity, fn);
	}

	// =============================================================
	// TRIGGER EVENTS
	// =============================================================

	static void Internal_RegisterTriggerEnter(uint32_t entity, MonoDelegate* del)
	{
		CollisionEventManager::CallbackFn fn = MakeManagedCollisionCallback(del);
		CollisionEventManager::GetInstance().RegisterTriggerEnter((Entity)entity, fn);
	}

	static void Internal_RegisterTriggerStay(uint32_t entity, MonoDelegate* del)
	{
		CollisionEventManager::CallbackFn fn = MakeManagedCollisionCallback(del);
		CollisionEventManager::GetInstance().RegisterTriggerStay((Entity)entity, fn);
	}

	static void Internal_RegisterTriggerExit(uint32_t entity, MonoDelegate* del)
	{
		CollisionEventManager::CallbackFn fn = MakeManagedCollisionCallback(del);
		CollisionEventManager::GetInstance().RegisterTriggerExit((Entity)entity, fn);
	}
} // unnamed namespace


// Binding registration

namespace CrashoutsEngine::Bindings
{
	void RegisterCollisionEventBindings()
	{
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterPreSolve", reinterpret_cast<const void*>(Internal_RegisterPreSolve));

		// Physical Collisions
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideEnter", reinterpret_cast<const void*>(Internal_RegisterCollideEnter));
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideStay", reinterpret_cast<const void*>(Internal_RegisterCollideStay));
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterCollideExit", reinterpret_cast<const void*>(Internal_RegisterCollideExit));

		// Trigger Overlaps
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerEnter", reinterpret_cast<const void*>(Internal_RegisterTriggerEnter));
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerStay", reinterpret_cast<const void*>(Internal_RegisterTriggerStay));
		mono_add_internal_call("CrashoutsEngine.CollisionEvents::RegisterTriggerExit", reinterpret_cast<const void*>(Internal_RegisterTriggerExit));
	}
}
