/*!
@file        scriptsystem.cpp
@author     Shahrin Idzuan
@author     Renfred Lee
@author     Renfred (RTIS)
@date       06/11/2025
@brief       Definition of script system functions

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
#include "lightbindings.h"
#include "activecomponent.h"
#include "videobindings.h"
#include "checkpointbindings.h"
#include "triggerbindings.h"
#include "replaybindings.h"
#include "leaderboardbindings.h"
#include "particlecomponent.h"
#include "ppcomponent.h"
#include <mono/metadata/attrdefs.h>
#include "collidereventmanager.h"
#include "performanceSystem.h"
#include "rapidjson/document.h"

// Bring in Logger for Engine Console output
#include "crashlogger.h" 

namespace {
    static Component::LightComponent* GetLight(uint32_t entityID) {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (ecs && ecs->HasComponent<Component::LightComponent>(entityID)) {
            return ecs->GetComponent<Component::LightComponent>(entityID);
        }
        return nullptr;
    }

    static void InternalAddLight(uint32_t id) {
        CrashoutsEngine::ECS::getInstance()->AddComponent<Component::LightComponent>(id);
    }

    static void InternalRemoveLight(uint32_t id) {
        CrashoutsEngine::ECS::getInstance()->RemoveComponent<Component::LightComponent>(id);
    }

    static bool InternalEntityHasLight(uint32_t id) {
        return CrashoutsEngine::ECS::getInstance()->HasComponent<Component::LightComponent>(id);
    }

    static float InternalGetLightIntensity(uint32_t id) {
        if (auto* l = GetLight(id)) return l->intensity;
        return 0.0f;
    }

    static void InternalSetLightIntensity(uint32_t id, float intensity) {
        if (auto* l = GetLight(id)) l->intensity = intensity;
    }

    static void InternalGetLightColor(uint32_t id, CEVec3* outColor) {
        if (!outColor) return;
        if (auto* l = GetLight(id)) *outColor = l->color;
        else *outColor = CEVec3{ 1.0f, 1.0f, 1.0f };
    }

    static void InternalSetLightColor(uint32_t id, CEVec3* inColor) {
        if (!inColor) return;
        if (auto* l = GetLight(id)) l->color = *inColor;
    }

    static bool InternalGetLightActive(uint32_t id) {
        if (auto* l = GetLight(id)) return l->active;
        return false;
    }

    static void InternalSetLightActive(uint32_t id, bool active) {
        if (auto* l = GetLight(id)) l->active = active;
    }
}

void CrashoutsEngine::Bindings::RegisterLightBindings()
{
    mono_add_internal_call("CrashoutsEngine.LightComponent::InternalGetIntensity", (const void*)InternalGetLightIntensity);
    mono_add_internal_call("CrashoutsEngine.LightComponent::InternalSetIntensity", (const void*)InternalSetLightIntensity);
    mono_add_internal_call("CrashoutsEngine.LightComponent::InternalGetColor", (const void*)InternalGetLightColor);
    mono_add_internal_call("CrashoutsEngine.LightComponent::InternalSetColor", (const void*)InternalSetLightColor);
    mono_add_internal_call("CrashoutsEngine.LightComponent::InternalGetActive", (const void*)InternalGetLightActive);
    mono_add_internal_call("CrashoutsEngine.LightComponent::InternalSetActive", (const void*)InternalSetLightActive);

    mono_add_internal_call("CrashoutsEngine.InternalCalls::AddLight", (const void*)InternalAddLight);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::RemoveLight", (const void*)InternalRemoveLight);
    mono_add_internal_call("CrashoutsEngine.InternalCalls::EntityHasLight", (const void*)InternalEntityHasLight);
}

namespace CrashoutsEngine
{
    using namespace Component;

    static MonoMethod* s_ScriptManagerUpdate = nullptr;
    static MonoMethod* s_ScriptManagerLateUpdate = nullptr;
    static MonoMethod* s_ScriptManagerFixedUpdate = nullptr;
    static MonoMethod* s_ScriptManagerOnDrawGizmos = nullptr;
    static MonoMethod* s_ScriptManagerClear = nullptr;

    typedef void(__stdcall* ScriptManagerUpdateThunk)(MonoObject** exc);
    static ScriptManagerUpdateThunk s_ScriptManagerUpdateThunk = nullptr;
    static ScriptManagerUpdateThunk s_ScriptManagerLateUpdateThunk = nullptr;
    static ScriptManagerUpdateThunk s_ScriptManagerFixedUpdateThunk = nullptr;
    static ScriptManagerUpdateThunk s_ScriptManagerOnDrawGizmosThunk = nullptr;

    // Helper to get field value based on type
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
        // CLASSES & VALUE TYPES (STRUCTS)
        else if (typeEnum == MONO_TYPE_VALUETYPE || typeEnum == MONO_TYPE_CLASS) {
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
            // --- NEW: ENTITY REFERENCE BACKUP ---
            else if (std::string(className) == "Entity") {
                MonoObject* targetObj = nullptr;

                // Extract the object memory differently if it is a struct vs a class
                if (typeEnum == MONO_TYPE_VALUETYPE) {
                    targetObj = mono_field_get_value_object(MonoManager::GetDomain(), field, instance);
                }
                else {
                    mono_field_get_value(instance, field, &targetObj);
                }

                if (targetObj) {
                    // Look for the ID field (handling hidden compiler properties)
                    MonoClassField* idField = mono_class_get_field_from_name(fieldClass, "ID");
                    if (!idField) idField = mono_class_get_field_from_name(fieldClass, "<ID>k__BackingField");
                    if (!idField) idField = mono_class_get_field_from_name(fieldClass, "id");

                    if (idField) {
                        uint32_t entID = 0;
                        mono_field_get_value(targetObj, idField, &entID);
                        // We safely store the ID as an integer in our variant
                        backup.value = (int)entID;
                    }
                }
            }
        }

        return backup;
    }

    // Helper to restore value
    void RestoreFieldValue(MonoObject* instance, MonoClassField* field, const FieldBackup& backup) {
        if (!instance || !field) return;

        int currentType = mono_type_get_type(mono_field_get_type(field));
        if (currentType != backup.monoType) {
            return; // Types diverged during recompile (e.g user changed float to int), safely ignore
        }

        MonoClass* fieldClass = mono_type_get_class(mono_field_get_type(field));
        const char* className = fieldClass ? mono_class_get_name(fieldClass) : "";

        // --- NEW: ENTITY REFERENCE RESTORE ---
        if (std::string(className) == "Entity") {
            if (std::holds_alternative<int>(backup.value)) {
                uint32_t loadedID = (uint32_t)std::get<int>(backup.value);

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
            return; // We handled the entity, skip the rest!
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
            if (std::string(className) == "CEVec3") {
                CEVec3 val = std::get<CEVec3>(backup.value);
                mono_field_set_value(instance, field, &val);
            }
        }
        // VECTORS (CEVec2)
        else if (std::holds_alternative<CEVec2>(backup.value)) {
            if (std::string(className) == "CEVec2") {
                CEVec2 val = std::get<CEVec2>(backup.value);
                mono_field_set_value(instance, field, &val);
            }
        }
        // COLOR
        else if (std::holds_alternative<CEColor>(backup.value)) {
            if (std::string(className) == "CEColor") {
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
            LOGE(Core, "[Mono] Failed to get Mono image during backup!");
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

                    // Check [SerializeField] or Public Access Mask
                    bool isPublic = ((flags & MONO_FIELD_ATTR_FIELD_ACCESS_MASK) == MONO_FIELD_ATTR_PUBLIC);
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
                        std::string cleanName = fieldName;

                        // NEW: Clean up compiler-generated backing fields so they restore properly
                        if (cleanName.find("<") == 0 && cleanName.find(">k__BackingField") != std::string::npos) {
                            cleanName = cleanName.substr(1, cleanName.find(">") - 1);
                        }

                        globalData[entityID][inst.ClassName][cleanName] = CaptureFieldValue(object, field);
                        fieldCount++;
                    }
                }
            }
            entityCount++;
        }

        LOGI(Core, ("[Mono] Captured " + std::to_string(fieldCount) + " fields from " + std::to_string(entityCount) + " scripts.").c_str());
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

                        // Refresh Class and Method pointers directly from the living object
                        inst.Class = mono_object_get_class(object);

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

                            // Check standard field name first
                            MonoClassField* field = mono_class_get_field_from_name(inst.Class, fieldName.c_str());

                            // If missing, check if it was a property backing field!
                            if (!field) {
                                std::string backingField = "<" + fieldName + ">k__BackingField";
                                field = mono_class_get_field_from_name(inst.Class, backingField.c_str());
                            }

                            if (field) {
                                RestoreFieldValue(object, field, value);
                                restoredCount++;
                            }
                        }
                        break;
                    }
                }
            }
        }
        LOGS(Core, ("[Mono] Restored " + std::to_string(restoredCount) + " fields back to scripts.").c_str());
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

                    // Fallback check for backing fields during deserialization!
                    if (!field) {
                        std::string backingName = "<" + std::string(it->name.GetString()) + ">k__BackingField";
                        field = mono_class_get_field_from_name(inst.Class, backingName.c_str());
                    }

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
                        else if (typeEnum == MONO_TYPE_VALUETYPE && strcmp(fieldClassName, "CEVec2") == 0) {
                            if (it->value.IsObject() && it->value.HasMember("x") && it->value.HasMember("y") &&
                                it->value["x"].IsNumber() && it->value["y"].IsNumber())
                            {
                                CEVec2 val{};
                                val.x = it->value["x"].GetFloat();
                                val.y = it->value["y"].GetFloat();
                                mono_field_set_value(instance, field, &val);
                            }
                        }
                        else if (typeEnum == MONO_TYPE_VALUETYPE && strcmp(fieldClassName, "CEVec3") == 0) {
                            if (it->value.IsObject() && it->value.HasMember("x") && it->value.HasMember("y") && it->value.HasMember("z") &&
                                it->value["x"].IsNumber() && it->value["y"].IsNumber() && it->value["z"].IsNumber())
                            {
                                CEVec3 val{};
                                val.x = it->value["x"].GetFloat();
                                val.y = it->value["y"].GetFloat();
                                val.z = it->value["z"].GetFloat();
                                mono_field_set_value(instance, field, &val);
                            }
                        }
                        else if (typeEnum == MONO_TYPE_VALUETYPE && strcmp(fieldClassName, "CEColor") == 0) {
                            if (it->value.IsObject() && it->value.HasMember("x") && it->value.HasMember("y") && it->value.HasMember("z") && it->value.HasMember("w") &&
                                it->value["x"].IsNumber() && it->value["y"].IsNumber() && it->value["z"].IsNumber() && it->value["w"].IsNumber())
                            {
                                CEColor val{};
                                val.r = it->value["x"].GetFloat();
                                val.g = it->value["y"].GetFloat();
                                val.b = it->value["z"].GetFloat();
                                val.a = it->value["w"].GetFloat();
                                mono_field_set_value(instance, field, &val);
                            }
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

    static void InternalParticleTriggerBurst(uint32_t entityID) {
        auto* ecs = ECS::getInstance();
        if (!ecs) return;
        if (!ecs->HasComponent<Component::ParticleComponent>(entityID)) return;
        auto* comp = ecs->GetComponent<Component::ParticleComponent>(entityID);
        if (!comp) return;

        comp->data.activeCount = 0;
        comp->emitAccumulator = 0.0f;
        comp->burstRequested = true;
        comp->isEmitting = true;
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
        Bindings::RegisterLightBindings();
        Bindings::RegisterVideoBindings();
        Bindings::RegisterCheckpointBindings();
        Bindings::RegisterTriggerBindings();
        Bindings::RegisterReplayBindings();
        Bindings::RegisterLeaderboardBindings();

        mono_add_internal_call("CrashoutsEngine.InternalCalls::GetScript", (const void*)InternalGetScript);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Particle_SetEmitting", (const void*)InternalSetParticleEmitting);
        mono_add_internal_call("CrashoutsEngine.InternalCalls::Particle_TriggerBurst", (const void*)InternalParticleTriggerBurst);
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
                s_ScriptManagerLateUpdate = mono_class_get_method_from_name(scriptManagerClass, "LateUpdateAll", 0);
                s_ScriptManagerFixedUpdate = mono_class_get_method_from_name(scriptManagerClass, "FixedUpdateAll", 0);
                s_ScriptManagerOnDrawGizmos = mono_class_get_method_from_name(scriptManagerClass, "OnDrawGizmosAll", 0);
                s_ScriptManagerClear = mono_class_get_method_from_name(scriptManagerClass, "ClearAll", 0);

                if (s_ScriptManagerUpdate) s_ScriptManagerUpdateThunk = (ScriptManagerUpdateThunk)mono_method_get_unmanaged_thunk(s_ScriptManagerUpdate);
                if (s_ScriptManagerLateUpdate) s_ScriptManagerLateUpdateThunk = (ScriptManagerUpdateThunk)mono_method_get_unmanaged_thunk(s_ScriptManagerLateUpdate);
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

    void ScriptSystem::LateUpdateAll()
    {
        FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "MonoLateUpdate");
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
        if (s_ScriptManagerLateUpdateThunk) {
            MonoObject* exc = nullptr;
            s_ScriptManagerLateUpdateThunk(&exc);
            if (exc) mono_print_unhandled_exception(exc);
        }
        else if (s_ScriptManagerLateUpdateThunk) {
            MonoObject* exc = nullptr;
            mono_runtime_invoke(s_ScriptManagerLateUpdate, nullptr, nullptr, &exc);
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
        s_ScriptManagerLateUpdateThunk = nullptr;
        s_ScriptManagerFixedUpdate = nullptr;
        s_ScriptManagerOnDrawGizmos = nullptr;
        s_ScriptManagerClear = nullptr;

        s_ScriptManagerUpdateThunk = nullptr;
        s_ScriptManagerLateUpdateThunk = nullptr;
        s_ScriptManagerFixedUpdateThunk = nullptr;
        s_ScriptManagerOnDrawGizmosThunk = nullptr;

        Component::ScriptComponent::ResetThunks();

        LOGI(Core, "[Mono] All script instances destroyed.");
    }

    void ScriptSystem::ReloadAll()
    {
        auto& ecs = *ECS::getInstance();

        // 1. BACKUP STATE
        LOGI(Core, "[Mono] Backing up script state...");
        GlobalScriptBackup backup = BackupScripts(ecs);
        CollisionEventManager::GetInstance().Clear();

        if (s_ScriptManagerClear) {
            mono_runtime_invoke(s_ScriptManagerClear, nullptr, nullptr, nullptr);
        }

        // Reset static pointers as they belong to the old domain
        s_ScriptManagerUpdate = nullptr;
        s_ScriptManagerLateUpdate = nullptr;
        s_ScriptManagerFixedUpdate = nullptr;
        s_ScriptManagerOnDrawGizmos = nullptr;
        s_ScriptManagerClear = nullptr;
        s_ScriptManagerUpdateThunk = nullptr;
        s_ScriptManagerLateUpdateThunk = nullptr;
        s_ScriptManagerFixedUpdateThunk = nullptr;
        s_ScriptManagerOnDrawGizmosThunk = nullptr;

        Component::ScriptComponent::ResetThunks();

        // 2. RELOAD DOMAIN
        MonoManager::ReloadAssembly();

        // Wipe old handles so the hot-reload creates fresh objects
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();
        for (auto* scriptComp : scripts.dense) {
            for (auto& inst : scriptComp->Scripts) inst.GCHandle = 0;
        }

        // 3. RE-INITIALIZE
        InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

        // 4. RESTORE STATE
        LOGI(Core, "[Mono] Restoring script state...");
        RestoreAllScripts(ecs, backup);

        // 5. PERSIST TO DISK
        if (!Serialization::currSceneName.empty()) {
            Serialization::SaveScenes(Serialization::currSceneName);
            LOGS(Core, ("[Mono] Persisted restored values to scene: " + Serialization::currSceneName).c_str());
        }

        LOGS(Core, "[Mono] Hot Reload Complete.");
    }
}