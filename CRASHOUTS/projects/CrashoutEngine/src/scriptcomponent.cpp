/*!
@file		scriptcomponent.cpp
@author     Shahrin Idzuan
@author     m.biniswandi
@author     Renfred Lee
@date       06/11/2025
@brief		Definitions of script component functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "ScriptComponent.h"
#include "entitymanager.h"
#include <mono/metadata/mono-config.h>

namespace CrashoutsEngine
{
	namespace
	{
		static MonoMethod* s_RegisterScript = nullptr;
		static MonoMethod* s_UnregisterScript = nullptr;

		typedef void(__stdcall* ScriptRegistrationThunk)(MonoObject* instance, MonoObject** exc);
		static ScriptRegistrationThunk s_RegisterScriptThunk = nullptr;
		static ScriptRegistrationThunk s_UnregisterScriptThunk = nullptr;

		bool InitializeOne(MonoDomain* domain, MonoAssembly* assembly, uint32_t entityID, ScriptInstance& script)
		{
			if (!assembly) {
				std::cerr << "[Mono] Assembly pointer is null!\n";
				return false;
			}

			MonoImage* image = mono_assembly_get_image(assembly);
			if (!image)
			{
				std::cerr << "[Mono] Failed to get image from assembly.\n";
				return false;
			}

			script.Class = mono_class_from_name(image, "CrashoutsEngine", script.ClassName.c_str());
			if (!script.Class)
			{
				std::cerr << "[Mono] Could not find class: " << script.ClassName << '\n';
				return false;
			}

			// Get the MonoClass* for Entity
			MonoClass* entityClass = mono_class_from_name(image, "CrashoutsEngine", "Entity");
			if (!entityClass)
			{
				std::cerr << "[Mono] Could not find class: CrashoutsEngine.Entity\n";
				return false;
			}

			// Create script instance
			MonoObject* instance = mono_object_new(domain, script.Class);
			mono_runtime_object_init(instance);

			// Create GC handle
			script.GCHandle = mono_gchandle_new(instance, false);

			// Construct C# Entity wrapper
			MonoObject* entityObj = mono_object_new(domain, entityClass);
			MonoMethod* entityCtor = mono_class_get_method_from_name(entityClass, ".ctor", 1);
			if (!entityCtor)
			{
				std::cerr << "[Mono] Could not find Entity constructor\n";
				return false;
			}

			uint32_t eid = entityID;
			void* ctorArgs[1] = { &eid };
			mono_runtime_invoke(entityCtor, entityObj, ctorArgs, nullptr);

			// Assign Entity field on script
			MonoClassField* entityField = mono_class_get_field_from_name(script.Class, "Entity");
			if (entityField)
			{
				mono_field_set_value(instance, entityField, entityObj);
			}

			// Cache methods
			script.MethodStart = mono_class_get_method_from_name(script.Class, "Start", 0);
			script.MethodUpdate = mono_class_get_method_from_name(script.Class, "Update", 0);
			// Cache FixedUpdate()
			script.MethodFixedUpdate = mono_class_get_method_from_name(script.Class, "FixedUpdate", 0);
			// Cache OnDrawGizmos()
			script.MethodOnDrawGizmos = mono_class_get_method_from_name(script.Class, "OnDrawGizmos", 0);
			// Cache OnDestroy()
			script.MethodOnDestroy = mono_class_get_method_from_name(script.Class, "OnDestroy", 0);


			if (script.MethodStart)
			{
				std::cout << "[Mono] Found Start() for " << script.ClassName << '\n';
			}
			else
			{
				std::cerr << "[Mono] Missing Start() in " << script.ClassName << '\n';
			}

			if (script.MethodUpdate)
			{
				std::cout << "[Mono] Found Update() for " << script.ClassName << '\n';
			}
			else
			{
				std::cerr << "[Mono] Missing Update() in " << script.ClassName << '\n';
			}

			if (script.MethodFixedUpdate)
			{
				std::cout << "[Mono] Found FixedUpdate() for " << script.ClassName << '\n';
			}
			else
			{
				std::cout << "[Mono] No FixedUpdate() in " << script.ClassName << '\n';
			}

			if (script.MethodOnDestroy)
			{
				std::cout << "[Mono] Found OnDestroy() for " << script.ClassName << '\n';
			}
			else
			{
				std::cout << "[Mono] No OnDestroy() in " << script.ClassName << '\n';
			}

			// Register in ScriptManager
			if (!s_RegisterScript) {
				MonoClass* scriptManagerClass = mono_class_from_name(image, "CrashoutsEngine", "ScriptManager");
				if (scriptManagerClass) {
					s_RegisterScript = mono_class_get_method_from_name(scriptManagerClass, "RegisterScript", 1);
					s_UnregisterScript = mono_class_get_method_from_name(scriptManagerClass, "UnregisterScript", 1);

					if (s_RegisterScript) s_RegisterScriptThunk = (ScriptRegistrationThunk)mono_method_get_unmanaged_thunk(s_RegisterScript);
					if (s_UnregisterScript) s_UnregisterScriptThunk = (ScriptRegistrationThunk)mono_method_get_unmanaged_thunk(s_UnregisterScript);
				}
			}

			if (s_RegisterScriptThunk) {
				MonoObject* exc = nullptr;
				s_RegisterScriptThunk(instance, &exc);
				if (exc) mono_print_unhandled_exception(exc);
			}
			else if (s_RegisterScript) {
				void* args[1] = { instance };
				mono_runtime_invoke(s_RegisterScript, nullptr, args, nullptr);
			}

			std::cout << "[Mono] ScriptInstance for class " << script.ClassName
				<< " initialized successfully\n";

			script.Started = false;
			return true;
		}

		void DestroyOne(ScriptInstance& script)
		{
			if (script.GCHandle)
			{
				MonoObject* instance = mono_gchandle_get_target(script.GCHandle);
				if (instance) {
					if (s_UnregisterScriptThunk) {
						MonoObject* exc = nullptr;
						s_UnregisterScriptThunk(instance, &exc);
						if (exc) mono_print_unhandled_exception(exc);
					}
					else if (s_UnregisterScript) {
						void* args[1] = { instance };
						mono_runtime_invoke(s_UnregisterScript, nullptr, args, nullptr);
					}

					if (script.MethodOnDestroy)
					{
						MonoObject* exc = nullptr;
						mono_runtime_invoke(script.MethodOnDestroy, instance, nullptr, &exc);
						if (exc) mono_print_unhandled_exception(exc);
					}
				}
				mono_gchandle_free(script.GCHandle);
				script.GCHandle = 0;
			}

			script.Class = nullptr;
			script.MethodStart = nullptr;
			script.MethodUpdate = nullptr;
			script.MethodFixedUpdate = nullptr;
			script.MethodOnDestroy = nullptr;
			script.Started = false;
		}
	}
}

namespace Component {
	using namespace CrashoutsEngine;
	ScriptInstance& ScriptComponent::AddScript(const std::string& className)
	{
		Scripts.emplace_back(className);
		return Scripts.back();
	}

	void ScriptComponent::RemoveScript(size_t index)
	{
		if (index >= Scripts.size())
			return;

		DestroyOne(Scripts[index]);
		Scripts.erase(Scripts.begin() + static_cast<std::ptrdiff_t>(index));
	}

	bool ScriptComponent::Initialize(MonoDomain* domain, MonoAssembly* assembly)
	{
		bool ok = true;
		for (auto& script : Scripts)
		{
			if (script.GCHandle) continue;

			// Re-initialize each script
			DestroyOne(script);
			if (!script.ClassName.empty())
			{
				ok &= InitializeOne(domain, assembly, EntityID, script);
			}
			//script.Started = false;
		}
		return ok;
	}

	void ScriptComponent::ResetThunks()
	{
		s_RegisterScript = nullptr;
		s_UnregisterScript = nullptr;
		s_RegisterScriptThunk = nullptr;
		s_UnregisterScriptThunk = nullptr;
	}

	void ScriptComponent::Start()
	{
		// Snapshot class names to iterate safely. 
		// We don't copy instances because they might become stale (e.g. if Entity is destroyed).
		std::vector<std::string> scriptNames;
		scriptNames.reserve(Scripts.size());
		for (const auto& s : Scripts) {
			scriptNames.push_back(s.ClassName);
		}

		uint32_t myEntityID = EntityID;

		for (const auto& name : scriptNames)
		{
			auto* ecs = ECS::getInstance();
			if (!ecs) return;

			// Check if entity/component still exists
			if (!ecs->HasComponent<ScriptComponent>(myEntityID)) {
				return; // Entity or component destroyed
			}

			auto* liveComp = ecs->GetComponent<ScriptComponent>(myEntityID);
			if (!liveComp) return; // Should not happen if HasComponent is true, but safe

			// Find the live script instance
			ScriptInstance* liveScript = nullptr;
			for (size_t j = 0; j < liveComp->Scripts.size(); ++j)
			{
				if (liveComp->Scripts[j].ClassName == name)
				{
					liveScript = &liveComp->Scripts[j];
					break;
				}
			}

			// If script was removed or not found, skip
			if (!liveScript) continue;

			// Only start if not already started
			if (liveScript->Started) continue;

			if (!liveScript->MethodStart) {
				// std::cout << "[Script] No Start() for " << liveScript->ClassName << "\n";
				// Mark as started even if no method, so we don't check again
				liveScript->Started = true;
				continue;
			}
			if (!liveScript->GCHandle) {
				std::cerr << "[Script] GCHandle null for " << liveScript->ClassName << "\n";
				liveScript->Started = true;
				continue;
			}

			// Mark started BEFORE invocation.
			// This prevents infinite recursion if Start() triggers Start(),
			// and avoids using 'liveScript' pointer after invocation (which might be dangling if vector reallocates).
			liveScript->Started = true;

			// Cache necessary data for invocation to avoid accessing liveScript after this point
			MonoMethod* methodStart = liveScript->MethodStart;
			MonoObject* instance = mono_gchandle_get_target(liveScript->GCHandle);

			if (instance) {
				MonoObject* exc = nullptr;
				mono_runtime_invoke(methodStart, instance, nullptr, &exc);
				if (exc) mono_print_unhandled_exception(exc);
			}
		}
	}

	void ScriptComponent::Update()
	{
		// NO-OP: Handled by ScriptManager batch update
	}

	void ScriptComponent::FixedUpdate()
	{
		// NO-OP: Handled by ScriptManager batch update
	}

	void ScriptComponent::DrawGizmos()
	{
		uint32_t myEntityID = this->EntityID;
		std::vector<ScriptInstance> scriptsCopy = Scripts;

		for (auto& script : scriptsCopy)
		{
			auto* ecs = ECS::getInstance();
			if (!ecs) return;

			if (!ecs->HasComponent<ScriptComponent>(myEntityID)) {
				return;
			}

			ScriptComponent* liveComp = ecs->GetComponent<ScriptComponent>(myEntityID);
			if (!liveComp) return;

			bool stillAlive = false;
			for (const auto& liveScript : liveComp->Scripts) {
				if (liveScript.GCHandle == script.GCHandle) {
					stillAlive = true;
					break;
				}
			}
			if (!stillAlive) continue;

			if (!script.MethodOnDrawGizmos || !script.GCHandle)
				continue;

			MonoObject* target = mono_gchandle_get_target(script.GCHandle);
			if (!target) continue;

			MonoObject* exc = nullptr;
			mono_runtime_invoke(script.MethodOnDrawGizmos, target, nullptr, &exc);

			if (exc) mono_print_unhandled_exception(exc);
		}
	}


	void ScriptComponent::Destroy()
	{
		for (auto& script : Scripts)
		{
			DestroyOne(script);
		}
	}
}
