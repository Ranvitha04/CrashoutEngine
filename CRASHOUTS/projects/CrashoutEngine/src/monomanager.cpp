#include "monomanager.h"
#include <filesystem>
#include <iostream>
#include <mono/metadata/mono-config.h>

MonoDomain* MonoManager::s_Domain = nullptr;
MonoAssembly* MonoManager::s_Assembly = nullptr;

void MonoManager::Init(const std::string& domainName)
{
    mono_set_dirs("third_party/Mono/lib", "third_party/Mono/etc");
    mono_config_parse(nullptr);

    s_Domain = mono_jit_init(domainName.c_str());

    if (!s_Domain)
        std::cerr << "[Mono] Failed to initialize JIT.\n";
    else
        std::cout << "[Mono] JIT initialized successfully.\n";
}

void MonoManager::LoadAssembly(const std::string& assemblyPath)
{
    std::filesystem::path abs = std::filesystem::absolute(assemblyPath);
    std::cout << "[Mono] >>> LoadAssembly called for " << abs << '\n';

    if (!std::filesystem::exists(abs))
    {
        std::cerr << "[Mono] Assembly not found: " << abs << '\n';
        return;
    }

    if (!s_Domain)
    {
        std::cerr << "[Mono] Domain not initialized!\n";
        return;
    }

    s_Assembly = mono_domain_assembly_open(s_Domain, abs.string().c_str());
    if (!s_Assembly)
    {
        std::cerr << "[Mono] Failed to load assembly: " << abs << '\n';
        return;
    }

    std::cout << "[Mono] Assembly opened successfully.\n";
}

void MonoManager::Shutdown()
{
    if (s_Domain)
    {
        return;
    }

	mono_jit_cleanup(s_Domain);
	s_Domain = nullptr;
	s_Assembly = nullptr;
	std::cout << "[Mono] Domain cleaned up.\n";
}
