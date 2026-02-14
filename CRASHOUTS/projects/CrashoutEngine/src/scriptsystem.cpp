/*!
@file		scriptsystem.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of script system functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "animatorbindings.h"
#include "colliderbindings.h"
#include "collidereventbindings.h"
#include "ScriptSystem.h"
#include "entitymanager.h"
#include "InputBindings.h"
#include "monomanager.h"
#include "rigidbodybindings.h"
#include "ScriptComponent.h"
#include "TransformBindings.h"
#include "entitybindings.h"
#include "namebindings.h"
#include "audiobindings.h"
#include "scenebindings.h"
#include "uibindings.h"
#include "tagbindings.h"
#include "timebindings.h"
#include "shapebindings.h"
#include "activecomponent.h"
#include "videobindings.h"
#include "particleComponent.h"
#include <mono/metadata/attrdefs.h>
#include "collidereventmanager.h"

namespace CrashoutsEngine
{
	using namespace Component;

	static MonoMethod* s_ScriptManagerUpdate = nullptr;
	static MonoMethod* s_ScriptManagerFixedUpdate = nullptr;
	static MonoMethod* s_ScriptManagerOnDrawGizmos = nullptr;
	static MonoMethod* s_ScriptManagerClear = nullptr;

	typedef void(__stdcall* ScriptManagerUpdateThunk)(MonoObject** exc);
	static ScriptManagerUpdateThunk s_ScriptManagerUpdateThunk = nullptr;
	static ScriptManagerUpdateThunk s_ScriptManagerFixedUpdateThunk = nullptr;
	static ScriptManagerUpdateThunk s_ScriptManagerOnDrawGizmosThunk = nullptr;

	// Helper to get field value based on type
	FieldBackup CaptureFieldValue(MonoObject* instance, MonoClassField* field) {
		FieldBackup backup;
		MonoType* type = mono_field_get_type(field);
		int typeEnum = mono_type_get_type(type);
		backup.monoType = typeEnum;

		// FLOAT
		if (typeEnum == MONO_TYPE_R4) {
			float val;
			mono_field_get_value(instance, field, &val);
			backup.value = val;
		}
		// INT
		else if (typeEnum == MONO_TYPE_I4) {
			int val;
			mono_field_get_value(instance, field, &val);
			backup.value = val;
		}
		// BOOLEAN
		else if (typeEnum == MONO_TYPE_BOOLEAN) {
			bool val;
			mono_field_get_value(instance, field, &val);
			backup.value = val;
		}
		// STRING
		else if (typeEnum == MONO_TYPE_STRING) {
			MonoString* strObj = nullptr;
			mono_field_get_value(instance, field, &strObj);
			if (strObj) {
				char* cStr = mono_string_to_utf8(strObj);
				backup.value = std::string(cStr);
				mono_free(cStr);
			}
			else {
				backup.value = std::string("");
			}
		}
		// VALUE TYPE (STRUCTS)
		else if (typeEnum == MONO_TYPE_VALUETYPE) {
			MonoClass* fieldClass = mono_type_get_class(type);
			const char* className = mono_class_get_name(fieldClass);

			// CEVec3
			if (std::string(className) == "CEVec3") {
				CEVec3 val;
				mono_field_get_value(instance, field, &val);
				backup.value = val;
			}
		}

		return backup;
	}

	// Helper to restore value
	void RestoreFieldValue(MonoObject* instance, MonoClassField* field, const FieldBackup& backup) {
		// Safety: Check if types match (in case user changed float to int in code)
		int currentType = mono_type_get_type(mono_field_get_type(field));
		if (currentType != backup.monoType) return;

		// FLOAT
		if (std::holds_alternative<float>(backup.value)) {
			float val = std::get<float>(backup.value);
			mono_field_set_value(instance, field, &val);
		}
		// INT
		else if (std::holds_alternative<int>(backup.value)) {
			int val = std::get<int>(backup.value);
			mono_field_set_value(instance, field, &val);
		}
		// BOOLEAN
		else if (std::holds_alternative<bool>(backup.value)) {
			bool val = std::get<bool>(backup.value);
			mono_field_set_value(instance, field, &val);
		}
		// STRING
		else if (std::holds_alternative<std::string>(backup.value)) {
			std::string val = std::get<std::string>(backup.value);
			MonoString* monoStr = mono_string_new(MonoManager::GetDomain(), val.c_str());
			mono_field_set_value(instance, field, monoStr);
		}
		// VECTORS (CEVec3)
		else if (std::holds_alternative<CEVec3>(backup.value)) {
			// Only restore if the field is actually a CEVec3
			MonoClass* fieldClass = mono_type_get_class(mono_field_get_type(field));
			if (std::string(mono_class_get_name(fieldClass)) == "CEVec3") {
				CEVec3 val = std::get<CEVec3>(backup.value);
				mono_field_set_value(instance, field, &val);
			}
		}
	}

	/*
	@brief  Retrieve all the script serialized data and return backup script
	@param  ecs - Entity ID of entity to reference
	*/
	GlobalScriptBackup BackupScripts(ECS& ecs) {
		GlobalScriptBackup globalData;
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();

		for (size_t i = 0; i < scripts.dense.size(); ++i) {
			Entity entityID = scripts.indexToEntity[i];
			ScriptComponent* sc = scripts.dense[i];

			for (auto& inst : sc->Scripts) {
				if (!inst.GCHandle) continue;
				MonoObject* object = mono_gchandle_get_target(inst.GCHandle);
				if (!object) continue;

				// Iterate fields
				void* iter = nullptr;
				while (MonoClassField* field = mono_class_get_fields(inst.Class, &iter)) {
					uint32_t flags = mono_field_get_flags(field);
					if (flags & MONO_FIELD_ATTR_STATIC) continue;

					// Check [SerializeField] or Public
					bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
					bool hasAttr = false;

					// Note: You might need to cache the Attribute Class pointer to avoid looking it up every loop
					static MonoClass* attrClass = mono_class_from_name(MonoManager::GetImage(), "CrashoutsEngine", "SerializeField");
					if (attrClass) {
						MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(inst.Class, field);
						if (attrInfo) {
							hasAttr = mono_custom_attrs_has_attr(attrInfo, attrClass);
							mono_custom_attrs_free(attrInfo);
						}
					}

					if (isPublic || hasAttr) {
						const char* fieldName = mono_field_get_name(field);
						globalData[entityID][inst.ClassName][fieldName] = CaptureFieldValue(object, field);
					}
				}
			}
		}
		return globalData;
	}

	/*
	@brief  Reference given script backup data and load it back into script
	@param  ecs  - Entity to reload backup data back into
	@param  data - Script backup data to reference
	*/
	void RestoreAllScripts(ECS& ecs, const GlobalScriptBackup& data) {
		for (auto const& [entityID, scriptData] : data) {
			// Ensure entity still exists and has script component
			if (!ecs.HasComponent<ScriptComponent>(entityID)) continue;
			auto* sc = ecs.GetComponent<ScriptComponent>(entityID);

			// Iterate through restored data classes
			for (auto const& [className, fields] : scriptData) {
				// Find the new instance of this class on the entity
				for (auto& inst : sc->Scripts) {
					if (inst.ClassName == className && inst.GCHandle) {
						MonoObject* object = mono_gchandle_get_target(inst.GCHandle);
						if (!object) continue;

						// Restore fields
						for (auto const& [fieldName, value] : fields) {
							MonoClassField* field = mono_class_get_field_from_name(inst.Class, fieldName.c_str());
							if (field) {
								RestoreFieldValue(object, field, value);
							}
						}
						break;
					}
				}
			}
		}
	}

	// Helper to find a specific C# script instance on an entity
	static MonoObject* InternalGetScript(uint32_t entityID, MonoString* scriptName) {
		auto* ecs = ECS::getInstance();

		// 1. Check if entity has scripts
		if (!ecs->HasComponent<ScriptComponent>(entityID)) return nullptr;

		auto* sc = ecs->GetComponent<ScriptComponent>(entityID);
		if (!sc) return nullptr;

		// 2. Convert C# string to std::string
		char* cStr = mono_string_to_utf8(scriptName);
		std::string name(cStr);
		mono_free(cStr);

		// 3. Search the vector for the matching class name
		for (auto& instance : sc->Scripts) {
			if (instance.ClassName == name) {
				if (instance.GCHandle) {
					return mono_gchandle_get_target(instance.GCHandle);
				}
				return nullptr;
			}
		}

		return nullptr;
	}

	static void InternalSetParticleEmitting(uint32_t entityID, bool emitting) {
		auto* ecs = ECS::getInstance();
		if (!ecs) return;
		if (!ecs->HasComponent<Component::ParticleComponent>(entityID)) return;
		auto* comp = ecs->GetComponent<Component::ParticleComponent>(entityID);
		if (!comp) return;
		comp->isEmitting = emitting;
	}

	void ScriptSystem::RegisterAllBindings()
	{
        Bindings::RegisterTransformBindings();
        Bindings::RegisterInputBindings();
        Bindings::RegisterRigidBodyBindings();
        Bindings::RegisterEntityBindings();
        Bindings::RegisterColliderBindings();
        Bindings::RegisterCollisionEventBindings();
        Bindings::RegisterNameBindings();
        Bindings::RegisterAudioBindings();
        Bindings::RegisterAnimatorBindings();
        Bindings::RegisterSceneBindings();
        Bindings::RegisterUIBindings();
        Bindings::RegisterTagBindings();
        Bindings::RegisterDebugBindings();
        //Bindings::RegisterAIBindings();
        Bindings::RegisterTimeBindings();
        Bindings::RegisterShapeBindings();
        Bindings::RegisterVideoBindings();

		mono_add_internal_call("CrashoutsEngine.InternalCalls::GetScript", (const void*)InternalGetScript);
		mono_add_internal_call("CrashoutsEngine.InternalCalls::Particle_SetEmitting", (const void*)InternalSetParticleEmitting);
	}


	void ScriptSystem::InitializeAll(MonoDomain* domain, MonoAssembly* assembly)
	{
		auto& ecs = *ECS::getInstance();
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();

		for (auto* scriptComp : scripts.dense)
		{
			scriptComp->Initialize(domain, assembly);
		}

		// Cache ScriptManager methods
		if (assembly) {
			MonoImage* image = mono_assembly_get_image(assembly);
			MonoClass* scriptManagerClass = mono_class_from_name(image, "CrashoutsEngine", "ScriptManager");
			if (scriptManagerClass) {
				s_ScriptManagerUpdate = mono_class_get_method_from_name(scriptManagerClass, "UpdateAll", 0);
				s_ScriptManagerFixedUpdate = mono_class_get_method_from_name(scriptManagerClass, "FixedUpdateAll", 0);
				s_ScriptManagerOnDrawGizmos = mono_class_get_method_from_name(scriptManagerClass, "OnDrawGizmosAll", 0);
				s_ScriptManagerClear = mono_class_get_method_from_name(scriptManagerClass, "ClearAll", 0);

				if (s_ScriptManagerUpdate) s_ScriptManagerUpdateThunk = (ScriptManagerUpdateThunk)mono_method_get_unmanaged_thunk(s_ScriptManagerUpdate);
				if (s_ScriptManagerFixedUpdate) s_ScriptManagerFixedUpdateThunk = (ScriptManagerUpdateThunk)mono_method_get_unmanaged_thunk(s_ScriptManagerFixedUpdate);
				if (s_ScriptManagerOnDrawGizmos) s_ScriptManagerOnDrawGizmosThunk = (ScriptManagerUpdateThunk)mono_method_get_unmanaged_thunk(s_ScriptManagerOnDrawGizmos);
			}
		}
	}

	void ScriptSystem::UpdateAll()
	{
		auto& ecs = *ECS::getInstance();
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();

		// 1. Process Start() for any new scripts (must stay in C++ loop for now to track Started flag)
		for (size_t i = 0; i < scripts.dense.size(); ++i)
		{
			Entity entityID = scripts.indexToEntity[i];
			if (ecs.HasComponent<ActiveComponent>(entityID)) {
				if (!IsEntityActive(ecs, entityID)) continue;
			}

			if (ecs.HasComponent<ScriptComponent>(entityID))
			{
				ecs.GetComponent<ScriptComponent>(entityID)->Start();
			}
		}

		// 2. Batch Update via ScriptManager
		if (s_ScriptManagerUpdateThunk) {
			MonoObject* exc = nullptr;
			s_ScriptManagerUpdateThunk(&exc);
			if (exc) mono_print_unhandled_exception(exc);
		}
		else if (s_ScriptManagerUpdate) {
			MonoObject* exc = nullptr;
			mono_runtime_invoke(s_ScriptManagerUpdate, nullptr, nullptr, &exc);
			if (exc) mono_print_unhandled_exception(exc);
		}
	}

	void ScriptSystem::FixedUpdateAll()
	{
		if (s_ScriptManagerFixedUpdateThunk) {
			MonoObject* exc = nullptr;
			s_ScriptManagerFixedUpdateThunk(&exc);
			if (exc) mono_print_unhandled_exception(exc);
		}
		else if (s_ScriptManagerFixedUpdate) {
			MonoObject* exc = nullptr;
			mono_runtime_invoke(s_ScriptManagerFixedUpdate, nullptr, nullptr, &exc);
			if (exc) mono_print_unhandled_exception(exc);
		}
	}

	void ScriptSystem::DrawGizmosAll()
	{
		if (s_ScriptManagerOnDrawGizmosThunk) {
			MonoObject* exc = nullptr;
			s_ScriptManagerOnDrawGizmosThunk(&exc);
			if (exc) mono_print_unhandled_exception(exc);
		}
		else if (s_ScriptManagerOnDrawGizmos) {
			MonoObject* exc = nullptr;
			mono_runtime_invoke(s_ScriptManagerOnDrawGizmos, nullptr, nullptr, &exc);
			if (exc) mono_print_unhandled_exception(exc);
		}
	}

	void ScriptSystem::DestroyAll()
	{
		auto& ecs = *ECS::getInstance();
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();

		for (auto* scriptComp : scripts.dense)
		{
			scriptComp->Destroy();
		}

		if (s_ScriptManagerClear) {
			mono_runtime_invoke(s_ScriptManagerClear, nullptr, nullptr, nullptr);
		}

		std::cout << "[Mono] All script instances destroyed.\n";
	}

	void ScriptSystem::ReloadAll()
	{
		auto& ecs = *ECS::getInstance();

		// 1. BACKUP STATE
		std::cout << "[Mono] Backing up script state...\n";
		GlobalScriptBackup backup = BackupScripts(ecs);
		CollisionEventManager::GetInstance().Clear();

		if (s_ScriptManagerClear) {
			mono_runtime_invoke(s_ScriptManagerClear, nullptr, nullptr, nullptr);
		}

		// 2. RELOAD DOMAIN
		// This destroys all GCHandles and MonoObjects
		// MonoManager::ReloadScripts() typically calls mono_domain_unload / mono_domain_create
		MonoManager::ReloadAssembly();

		// 3. RE-INITIALIZE
		// Recreate the ScriptInstances (get new Class pointers, create new Objects)
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();
		for (auto* scriptComp : scripts.dense)
		{
			// This creates NEW 'empty' C# objects with default values
			scriptComp->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
		}

		// 4. RESTORE STATE
		std::cout << "[Mono] Restoring script state...\n";
		RestoreAllScripts(ecs, backup);

		std::cout << "[Mono] Hot Reload Complete.\n";
	}

}
