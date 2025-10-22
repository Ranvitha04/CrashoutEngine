#include "monomanager.h"

#include <filesystem>
#include <iostream>
#include <mono/metadata/mono-config.h>

MonoDomain* MonoManager::s_Domain = nullptr;

void MonoManager::Init(const std::string& domainName)
{
    // Set runtime and config directories
    mono_set_dirs("third_party/Mono/lib", "third_party/Mono/etc");

    mono_config_parse(nullptr);

    s_Domain = mono_jit_init(domainName.c_str());

    if (!s_Domain)
        std::cerr << "[Mono] Failed to initialize JIT." << std::endl;
    else
        std::cout << "[Mono] JIT initialized successfully." << std::endl;
}

void MonoManager::LoadAssembly(const std::string& assemblyPath)
{
    std::filesystem::path abs = std::filesystem::absolute(assemblyPath);
    std::cout << "[Mono] >>> LoadAssembly called for " << assemblyPath << std::endl;

    if (!std::filesystem::exists(abs))
    {
        std::cerr << "[Mono] Assembly not found at: " << abs << std::endl;
        return;
    }

    if (!std::filesystem::exists(assemblyPath))
    {
        std::cerr << "[Mono] Assembly not found at: " << std::filesystem::absolute(assemblyPath) << std::endl;
        return;
    }

    if (!s_Domain)
    {
        std::cerr << "[Mono] Domain not initialized!" << std::endl;
        return;
    }

    MonoAssembly* assembly = mono_domain_assembly_open(s_Domain, assemblyPath.c_str());
    if (!assembly)
    {
        std::cerr << "[Mono] Failed to load assembly: " << assemblyPath << std::endl;
        return;
    }
    std::cout << "[Mono] Assembly opened successfully." << std::endl;

    MonoImage* image = mono_assembly_get_image(assembly);
    if (!image)
    {
        std::cerr << "[Mono] Failed to get image." << std::endl;
        return;
    }
    std::cout << "[Mono] Got image successfully." << std::endl;

    MonoClass* klass = mono_class_from_name(image, "ExampleNamespace", "ExampleClass");
    if (!klass)
    {
        std::cerr << "[Mono] Failed to find class ExampleNamespace.ExampleClass." << std::endl;
        return;
    }
    std::cout << "[Mono] Found ExampleClass." << std::endl;

    MonoMethod* method = mono_class_get_method_from_name(klass, "Start", 0);
    if (!method)
    {
        std::cerr << "[Mono] Failed to find method Start()." << std::endl;
        return;
    }
    std::cout << "[Mono] Found Start() method." << std::endl;

    MonoObject* exc = nullptr;
    mono_runtime_invoke(method, nullptr, nullptr, &exc);
    if (exc)
    {
        std::cerr << "[Mono] Managed exception occurred:" << std::endl;
        mono_print_unhandled_exception(exc);
    }
    else
    {
        std::cout << "[Mono] Method executed successfully." << std::endl;
    }
}


void MonoManager::Shutdown()
{
    if (s_Domain)
    {
        mono_jit_cleanup(s_Domain);
        s_Domain = nullptr;
        std::cout << "[Mono] Domain cleaned up." << std::endl;
    }
}
