#include "monomanager.h"
#include <iostream>

MonoDomain* MonoManager::s_Domain = nullptr;

void MonoManager::Init(const std::string& domainName)
{
    // Set runtime and config directories
    mono_set_dirs("third_party/Mono/lib", "third_party/Mono/etc");

    s_Domain = mono_jit_init(domainName.c_str());

    if (!s_Domain)
        std::cerr << "[Mono] Failed to initialize JIT." << std::endl;
    else
        std::cout << "[Mono] JIT initialized successfully." << std::endl;
}

void MonoManager::LoadAssembly(const std::string& assemblyPath)
{
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

    MonoImage* image = mono_assembly_get_image(assembly);
    MonoClass* klass = mono_class_from_name(image, "YourNamespace", "YourClass");
    if (!klass)
    {
        std::cerr << "[Mono] Failed to find class." << std::endl;
        return;
    }

    MonoMethod* method = mono_class_get_method_from_name(klass, "YourFunction", 0);
    if (!method)
    {
        std::cerr << "[Mono] Failed to find method." << std::endl;
        return;
    }

    mono_runtime_invoke(method, nullptr, nullptr, nullptr);
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
