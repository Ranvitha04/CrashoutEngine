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

        Class = mono_class_from_name(image, "ExampleNamespace", ClassName.c_str());
        if (!Class)
        {
            std::cerr << "[Mono] Could not find class: " << ClassName << '\n';
            return false;
        }

        // Create C# instance
        Instance = mono_object_new(domain, Class);
        mono_runtime_object_init(Instance);

        // Cache methods
        MethodStart = mono_class_get_method_from_name(Class, "Start", 0);
        MethodUpdate = mono_class_get_method_from_name(Class, "Update", 1);

        if (MethodStart)
            std::cout << "[Mono] Found Start() for " << ClassName << '\n';
        else
            std::cerr << "[Mono] Missing Start() in " << ClassName << '\n';

        if (MethodUpdate)
            std::cout << "[Mono] Found Update(float) for " << ClassName << '\n';
        else
            std::cerr << "[Mono] Missing Update(float) in " << ClassName << '\n';


        std::cout << "[Mono] ScriptComponent for class " << ClassName << " initialized successfully.\n";


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

    void ScriptComponent::Update(float dt)
    {
        if (!MethodUpdate) return;

        void* args[1] = { &dt };
        MonoObject* exc = nullptr;
        mono_runtime_invoke(MethodUpdate, Instance, args, &exc);
        if (exc) mono_print_unhandled_exception(exc);
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
