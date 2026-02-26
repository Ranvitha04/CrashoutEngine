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
#include "sceneasset.h"
#include "uibindings.h"
#include "tagbindings.h"
#include "timebindings.h"
#include "shapebindings.h"
#include "activecomponent.h"
#include "videobindings.h"
#include "checkpointbindings.h"
#include "triggerbindings.h"
#include "particlecomponent.h"
#include <mono/metadata/attrdefs.h>
#include "collidereventmanager.h"
#include "performanceSystem.h"
#include "rapidjson/document.h"

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
		backup.monoType = MONO_TYPE_END;

		if (!instance || !field) return backup;

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
			// CEVec2
			else if (std::string(className) == "CEVec2") {
				CEVec2 val;
				mono_field_get_value(instance, field, &val);
				backup.value = val;
			}
			// CEColor
			else if (std::string(className) == "CEColor") {
				CEColor val;
				mono_field_get_value(instance, field, &val);
				backup.value = val;
			}
		}

		return backup;
	}

	// Helper to restore value
	void RestoreFieldValue(MonoObject* instance, MonoClassField* field, const FieldBackup& backup) {
		if (!instance || !field) return;

		// Safety: Check if types match (in case user changed float to int in code)
		int currentType = mono_type_get_type(mono_field_get_type(field));
		if (currentType != backup.monoType) {
			// Don't log spam for type mismatches (they happen if user edits code)
			return;
		}

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
			MonoClass* fieldClass = mono_type_get_class(mono_field_get_type(field));
			if (std::string(mono_class_get_name(fieldClass)) == "CEVec3") {
				CEVec3 val = std::get<CEVec3>(backup.value);
				mono_field_set_value(instance, field, &val);
			}
		}
		// VECTORS (CEVec2)
		else if (std::holds_alternative<CEVec2>(backup.value)) {
			MonoClass* fieldClass = mono_type_get_class(mono_field_get_type(field));
			if (std::string(mono_class_get_name(fieldClass)) == "CEVec2") {
				CEVec2 val = std::get<CEVec2>(backup.value);
				mono_field_set_value(instance, field, &val);
			}
		}
		// COLOR
		else if (std::holds_alternative<CEColor>(backup.value)) {
			MonoClass* fieldClass = mono_type_get_class(mono_field_get_type(field));
			if (std::string(mono_class_get_name(fieldClass)) == "CEColor") {
				CEColor val = std::get<CEColor>(backup.value);
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

		MonoImage* image = MonoManager::GetImage();
		if (!image) {
			std::cerr << "[Backup] Failed to get Mono image!\n";
			return globalData;
		}
		MonoClass* attrClass = mono_class_from_name(image, "CrashoutsEngine", "SerializeField");

		int entityCount = 0;
		int fieldCount = 0;

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
						// std::cout << "[Backup] Captured field: " << inst.ClassName << "." << fieldName << "\n";
						fieldCount++;
					}
				}
			}
			entityCount++;
		}
		std::cout << "[Backup] Captured " << fieldCount << " fields from " << entityCount << " script components.\n";
		return globalData;
	}

	/*
	@brief  Reference given script backup data and load it back into script
	@param  ecs  - Entity to reload backup data back into
	@param  data - Script backup data to reference
	*/
	void RestoreAllScripts(ECS& ecs, const GlobalScriptBackup& data) {
		int restoredCount = 0;
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

						// Refresh Class and Method pointers for the inspector
						MonoImage* image = MonoManager::GetImage();
						inst.Class = mono_class_from_name(image, "CrashoutsEngine", inst.ClassName.c_str());
						
						// Try fallback to no namespace if not found
						if (!inst.Class) {
							inst.Class = mono_class_from_name(image, "", inst.ClassName.c_str());
						}

						if (inst.Class) {
							inst.MethodStart = mono_class_get_method_from_name(inst.Class, "Start", 0);
							inst.MethodUpdate = mono_class_get_method_from_name(inst.Class, "Update", 0);
							inst.MethodFixedUpdate = mono_class_get_method_from_name(inst.Class, "FixedUpdate", 0);
							inst.MethodOnDrawGizmos = mono_class_get_method_from_name(inst.Class, "OnDrawGizmos", 0);
							inst.MethodOnDestroy = mono_class_get_method_from_name(inst.Class, "OnDestroy", 0);
						}

						// Restore fields
						for (auto const& [fieldName, value] : fields) {
							if (!inst.Class) break;
							MonoClassField* field = mono_class_get_field_from_name(inst.Class, fieldName.c_str());
							if (field) {
								RestoreFieldValue(object, field, value);
								// std::cout << "[Restore] Applied field: " << className << "." << fieldName << "\n";
								restoredCount++;
							}
						}
						break;
					}
				}
			}
		}
		std::cout << "[Restore] Applied " << restoredCount << " fields back to scripts.\n";
	}

	void ScriptSystem::DeserializeFields(const rapidjson::Value& scriptsArray, Component::ScriptComponent& scriptComp) {
		if (!scriptsArray.IsArray()) return;

		for (rapidjson::SizeType k = 0; k < scriptsArray.Size(); ++k) {
			if (k >= scriptComp.Scripts.size()) break;
			auto& inst = scriptComp.Scripts[k];
			const auto& item = scriptsArray[k];

			MonoObject* instance = inst.GCHandle ? mono_gchandle_get_target(inst.GCHandle) : nullptr;
			if (item.IsObject() && item.HasMember("fields") && instance && inst.Class) {
				const auto& fields = item["fields"];
				for (auto it = fields.MemberBegin(); it != fields.MemberEnd(); ++it) {
					MonoClassField* field = mono_class_get_field_from_name(inst.Class, it->name.GetString());
					if (field) {
						MonoType* type = mono_field_get_type(field);
						int typeEnum = mono_type_get_type(type);

						MonoClass* fieldClass = mono_class_from_mono_type(type);
						const char* fieldClassName = fieldClass ? mono_class_get_name(fieldClass) : "";


						if (typeEnum == MONO_TYPE_I4 && it->value.IsInt()) {
							int val = it->value.GetInt();
							mono_field_set_value(instance, field, &val);
						}
						else if (typeEnum == MONO_TYPE_R4 && it->value.IsNumber()) {
							float val = it->value.GetFloat();
							mono_field_set_value(instance, field, &val);
						}
						else if (typeEnum == MONO_TYPE_BOOLEAN && it->value.IsBool()) {
							bool val = it->value.GetBool();
							mono_field_set_value(instance, field, &val);
						}
						else if (typeEnum == MONO_TYPE_STRING && it->value.IsString()) {
							MonoString* str = mono_string_new(MonoManager::GetDomain(), it->value.GetString());
							mono_field_set_value(instance, field, str);
						}
						else if (strcmp(fieldClassName, "Entity") == 0) {
							if (it->value.IsUint() || it->value.IsInt()) {
								uint32_t loadedID = it->value.GetUint();

								MonoClassField* idField = mono_class_get_field_from_name(fieldClass, "ID");
								if (!idField) idField = mono_class_get_field_from_name(fieldClass, "<ID>k__BackingField");
								if (!idField) idField = mono_class_get_field_from_name(fieldClass, "id");

								if (mono_class_is_valuetype(fieldClass)) {
									// Struct handling
									MonoObject* boxed = mono_object_new(MonoManager::GetDomain(), fieldClass);
									if (idField) mono_field_set_value(boxed, idField, &loadedID);

									void* rawData = mono_object_unbox(boxed);
									mono_field_set_value(instance, field, rawData);
								}
								else {
									// Class handling
									if (loadedID == 0) {
										// FIX: If the ID is 0, safely set the C# object to null!
										void* nullPtr = nullptr;
										mono_field_set_value(instance, field, nullPtr);
									}
									else {
										MonoObject* newEntObj = mono_object_new(MonoManager::GetDomain(), fieldClass);
										if (idField) mono_field_set_value(newEntObj, idField, &loadedID);
										mono_field_set_value(instance, field, newEntObj);
									}
								}
							}
						}
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
        Bindings::RegisterCheckpointBindings();
        Bindings::RegisterTriggerBindings();

		mono_add_internal_call("CrashoutsEngine.InternalCalls::GetScript", (const void*)InternalGetScript);
		mono_add_internal_call("CrashoutsEngine.InternalCalls::Particle_SetEmitting", (const void*)InternalSetParticleEmitting);
	}


	void ScriptSystem::InitializeAll(MonoDomain* domain, MonoAssembly* assembly)
	{
		auto& ecs = *ECS::getInstance();
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();

		for (auto* scriptComp : scripts.dense)
		{
			// Reset GCHandle to 0 so Initialize actually does work after a hot reload
			//for (auto& inst : scriptComp->Scripts) inst.GCHandle = 0;
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
		FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "MonoUpdate");
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
		FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "MonoFixedUpdate");
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

		s_ScriptManagerUpdate = nullptr;
		s_ScriptManagerFixedUpdate = nullptr;
		s_ScriptManagerOnDrawGizmos = nullptr;
		s_ScriptManagerClear = nullptr;

		s_ScriptManagerUpdateThunk = nullptr;
		s_ScriptManagerFixedUpdateThunk = nullptr;
		s_ScriptManagerOnDrawGizmosThunk = nullptr;

		Component::ScriptComponent::ResetThunks();

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

		// Reset static pointers as they belong to the old domain
		s_ScriptManagerUpdate = nullptr;
		s_ScriptManagerFixedUpdate = nullptr;
		s_ScriptManagerOnDrawGizmos = nullptr;
		s_ScriptManagerClear = nullptr;
		s_ScriptManagerUpdateThunk = nullptr;
		s_ScriptManagerFixedUpdateThunk = nullptr;
		s_ScriptManagerOnDrawGizmosThunk = nullptr;

		Component::ScriptComponent::ResetThunks();

		// 2. RELOAD DOMAIN
		// This destroys all GCHandles and MonoObjects
		// MonoManager::ReloadAssembly() typically calls mono_domain_unload / mono_domain_create
		MonoManager::ReloadAssembly();

		// Wipe old handles so the hot-reload creates fresh objects
		auto& scripts = ecs.GetComponentArray<ScriptComponent>();
		for (auto* scriptComp : scripts.dense) {
			for (auto& inst : scriptComp->Scripts) inst.GCHandle = 0;
		}

		// 3. RE-INITIALIZE
		// Recreate the ScriptInstances (get new Class pointers, create new Objects)
		InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

		// 4. RESTORE STATE
		std::cout << "[Mono] Restoring script state...\n";
		RestoreAllScripts(ecs, backup);

		// 5. PERSIST TO DISK
		// This prevents "STOP" from reloading old values from disk
		if (!Serialization::currSceneName.empty()) {
			Serialization::SaveScenes(Serialization::currSceneName);
			std::cout << "[Mono] Persisted restored values to scene: " << Serialization::currSceneName << "\n";
		}

		std::cout << "[Mono] Hot Reload Complete.\n";
	}

}
