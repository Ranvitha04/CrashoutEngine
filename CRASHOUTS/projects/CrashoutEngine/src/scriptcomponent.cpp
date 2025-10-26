#include "ScriptComponent.h"
#include <iostream>

namespace CrashoutsEngine
{
    static MonoClass* GetMonoClass(MonoAssembly* assembly, const std::string& className)
    {
        MonoImage* image = mono_assembly_get_image(assembly);
        MonoClass* klass = mono_class_from_name(image, "ExampleNamespace", className.c_str());

        if (!klass)
        {
            std::cerr << "[Mono] Could not find class: " << className << '\n';
        }

        return klass;
    }

    bool ScriptComponent::Initialize(MonoDomain* domain, MonoAssembly* assembly)
    {
        Class = GetMonoClass(assembly, ClassName);

        if (!Class)
        {
            return false;
        }

        Instance = mono_object_new(domain, Class);

        if (!Instance)
        {
            std::cerr << "[Mono] Failed to create instance of " << ClassName << '\n';
            return false;
        }

        mono_runtime_object_init(Instance);

        MethodStart = mono_class_get_method_from_name(Class, "Start", 0);
        MethodUpdate = mono_class_get_method_from_name(Class, "Update", 1);

        if (!MethodStart)
        {
            std::cerr << "[Mono] Warning: " << ClassName << " has no Start()\n";
        }

        if (!MethodUpdate)
        {
            std::cerr << "[Mono] Warning: " << ClassName << " has no Update(float)\n";
        }

        return true;
    }

    void ScriptComponent::Start()
    {
        if (Started || !MethodStart)
        {
            return;
        }

        MonoObject* exception = nullptr;
        mono_runtime_invoke(MethodStart, Instance, nullptr, &exception);
        if (exception)
        {
            MonoString* msg = mono_object_to_string(exception, nullptr);
            std::cerr << "[Mono Exception] " << mono_string_to_utf8(msg) << std::endl;
        }

        Started = true;
    }

    void ScriptComponent::Update(float deltaTime)
    {
        if (!MethodUpdate) return;

        void* args[1];
        args[0] = &deltaTime;

        MonoObject* exception = nullptr;
        mono_runtime_invoke(MethodUpdate, Instance, args, &exception);

        if (exception)
        {
            MonoString* msg = mono_object_to_string(exception, nullptr);
            std::cerr << "[Mono Exception] " << mono_string_to_utf8(msg) << std::endl;
        }
    }

    void ScriptComponent::Destroy()
    {
        Instance = nullptr;
        Class = nullptr;
        MethodStart = nullptr;
        MethodUpdate = nullptr;
        Started = false;
    }
}
