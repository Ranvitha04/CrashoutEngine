/*!
@file		scriptcomponent.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definitions of script component functions
*/
#include "ScriptComponent.h"
#include <mono/metadata/mono-config.h>

namespace CrashoutsEngine
{
    bool ScriptComponent::Initialize(MonoDomain* domain, MonoAssembly* assembly)
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

        Class = mono_class_from_name(image, "CrashoutsEngine", ClassName.c_str());
        if (!Class)
        {
            std::cerr << "[Mono] Could not find class: " << ClassName << '\n';
            return false;
        }

        // Create C# instance
        //Instance = mono_object_new(domain, Class);
        //mono_runtime_object_init(Instance);

        // Get the MonoClass*
        MonoClass* entityClass = mono_class_from_name(image, "CrashoutsEngine", "Entity");

        //GCHandle = 0;

        Instance = mono_object_new(domain, Class);
        mono_runtime_object_init(Instance);

        GCHandle = mono_gchandle_new(Instance,false);

        MonoObject* entityObj = mono_object_new(domain, entityClass);
        MonoMethod* entityCtor = mono_class_get_method_from_name(entityClass, ".ctor", 1);
        uint32_t eid = EntityID;
        void* ctorArgs[1] = { &eid };
        mono_runtime_invoke(entityCtor, entityObj, ctorArgs, nullptr);
        MonoClassField* entityField = mono_class_get_field_from_name(Class, "Entity");
        if (entityField)
        {
            mono_field_set_value(Instance, entityField, entityObj);
        }

        // Cache methods
        MethodStart = mono_class_get_method_from_name(Class, "Start", 0);
        MethodUpdate = mono_class_get_method_from_name(Class, "Update", 0);

        if (MethodStart)
        {
            std::cout << "[Mono] Found Start() for " << ClassName << '\n';
        }
        else
        {
            std::cerr << "[Mono] Missing Start() in " << ClassName << '\n';
        }

        if (MethodUpdate)
        {
            std::cout << "[Mono] Found Update(float) for " << ClassName << '\n';
        }
        else
        {
            std::cerr << "[Mono] Missing Update(float) in " << ClassName << '\n';
        }

        std::cout << "[Mono] ScriptComponent for class " << ClassName << " initialized successfully\n";


        return true;
    }

    void ScriptComponent::Start()
    {
        if (Started || !MethodStart) return;

        MonoObject* exc = nullptr;
        mono_runtime_invoke(MethodStart, Instance, nullptr, &exc);
        if (exc) mono_print_unhandled_exception(exc);

        Started = true;
    }

    void ScriptComponent::Update()
    {
        if (!MethodUpdate) return;
        MonoObject* target = mono_gchandle_get_target(GCHandle);
        //void* args[1] = { &dt };
        MonoObject* exc = nullptr;
        mono_runtime_invoke(MethodUpdate, target, nullptr, &exc);
        if (exc) mono_print_unhandled_exception(exc);

    }

    void ScriptComponent::Destroy()
    {
        Instance = nullptr;
        Class = nullptr;
        MethodStart = nullptr;
        MethodUpdate = nullptr;
        Started = false;
        if (GCHandle) { mono_gchandle_free(GCHandle); GCHandle = 0; }
    }
}
