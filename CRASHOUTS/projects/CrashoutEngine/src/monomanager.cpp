#include "monomanager.h"

#include <filesystem>
#include <iostream>
#include <mono/metadata/mono-config.h>

#include "frameratecontroller.h"

MonoDomain* MonoManager::s_Domain = nullptr;
MonoAssembly* MonoManager::s_Assembly = nullptr;

void MonoManager::Init(const std::string& domainName)
{
    // Set runtime and config directories
    mono_set_dirs("third_party/Mono/lib", "third_party/Mono/etc");

    mono_config_parse(nullptr);

    s_Domain = mono_jit_init(domainName.c_str());

    if (!s_Domain)
    {
        std::cerr << "[Mono] Failed to initialize JIT." << '\n';
	}
    else
    {
        std::cout << "[Mono] JIT initialized successfully." << '\n';
    }
}

void MonoManager::LoadAssembly(const std::string& assemblyPath)
{
    std::filesystem::path abs = std::filesystem::absolute(assemblyPath);
    std::cout << "[Mono] >>> LoadAssembly called for " << assemblyPath << '\n';

    if (!std::filesystem::exists(abs))
    {
        std::cerr << "[Mono] Assembly not found at: " << abs << '\n';
        return;
    }

    if (!std::filesystem::exists(assemblyPath))
    {
        std::cerr << "[Mono] Assembly not found at: " << std::filesystem::absolute(assemblyPath) << '\n';
        return;
    }

    if (!s_Domain)
    {
        std::cerr << "[Mono] Domain not initialized!" << '\n';
        return;
    }

    MonoAssembly* assembly = mono_domain_assembly_open(s_Domain, assemblyPath.c_str());
    if (!assembly)
    {
        std::cerr << "[Mono] Failed to load assembly: " << assemblyPath << '\n';
        return;
    }

    s_Assembly = mono_domain_assembly_open(s_Domain, assemblyPath.c_str());
    if (!s_Assembly)
    {
        std::cerr << "[Mono] Failed to load assembly: " << assemblyPath << '\n';
        return;
    }

    std::cout << "[Mono] Assembly opened successfully." << '\n';

    MonoImage* image = mono_assembly_get_image(assembly);
    if (!image)
    {
        std::cerr << "[Mono] Failed to get image." << '\n';
        return;
    }
    std::cout << "[Mono] Got image successfully." << '\n';
	/*
    MonoClass* klass = mono_class_from_name(image, "ExampleNamespace", "ExampleClass");
    MonoMethod* startMethod = mono_class_get_method_from_name(klass, "Start", 0);
	MonoMethod* runMethod = mono_class_get_method_from_name(klass, "Run", 0);
	*/
    MonoClass* klass = mono_class_from_name(image, "ExampleNamespace", "PlayerScript");

    s_Assembly = mono_domain_assembly_open(s_Domain, assemblyPath.c_str());
    if (!s_Assembly)
    {
        std::cerr << "[Mono] Failed to load assembly: " << assemblyPath << '\n';
        return;
    }

    std::cout << "[Mono] Assembly opened successfully." << '\n';


    // Create an instance first
    MonoObject* instance = mono_object_new(s_Domain, klass);
    mono_runtime_object_init(instance);

    // Find methods
    MonoMethod* startMethod = mono_class_get_method_from_name(klass, "Start", 0);
    MonoMethod* updateMethod = mono_class_get_method_from_name(klass, "Update", 1);

    // Call Start()
    mono_runtime_invoke(startMethod, instance, nullptr, nullptr);

    // Simulate an Update call with a float argument
    double dt = FrameRateController::GetInstance().FixedDT();
    void* args[1] ={&dt};

    MonoObject* exc = nullptr;
    mono_runtime_invoke(updateMethod, instance, args, &exc);


    if (exc)
        mono_print_unhandled_exception(exc);

    std::cout << "[Mono] Finished executing PlayerScript.\n";

}


void MonoManager::Shutdown()
{
    if (s_Domain)
    {
        mono_jit_cleanup(s_Domain);
        s_Domain = nullptr;
        std::cout << "[Mono] Domain cleaned up." << '\n';
    }
}
