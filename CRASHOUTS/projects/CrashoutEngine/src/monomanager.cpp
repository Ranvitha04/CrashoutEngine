#include "monomanager.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <mono/metadata/mono-config.h>

#include "ScriptSystem.h"

MonoDomain* MonoManager::s_Domain = nullptr;
MonoAssembly* MonoManager::s_Assembly = nullptr;
std::filesystem::file_time_type MonoManager::s_LastWriteTime{};
std::string MonoManager::s_AssemblyPath = "scripts/ManagedScripts.dll";

void MonoManager::Init(const std::string& domainName)
{
    mono_set_dirs("third_party/Mono/lib", "third_party/Mono/etc");
    mono_config_parse(nullptr);

    s_Domain = mono_jit_init(domainName.c_str());

    if (!s_Domain)
    {
        std::cerr << "[Mono] Failed to initialize JIT.\n";
    }
    else
    {
        std::cout << "[Mono] JIT initialized successfully.\n";
    }

    // Track DLL timestamp for hot reload
    s_LastWriteTime = std::filesystem::last_write_time(s_AssemblyPath);
}

void MonoManager::LoadAssembly(const std::string& assemblyPath)
{
    std::string TempDllPath = CopyAssemblyToTemp(assemblyPath);


    if (!std::filesystem::exists(TempDllPath))
    {
        std::cerr << "[Mono] Assembly not found: " << TempDllPath << '\n';
        return;
    }

    if (!s_Domain)
    {
        std::cerr << "[Mono] Domain not initialized!\n";
        return;
    }

    s_Assembly = mono_domain_assembly_open(s_Domain, TempDllPath.c_str());
    if (!s_Assembly)
    {
        std::cerr << "[Mono] Failed to load assembly: " << TempDllPath << '\n';
        return;
    }

    std::cout << "[Mono] Assembly opened successfully.\n";

    CrashoutsEngine::ScriptSystem::RegisterAllBindings();

    // Track last modified time for hot reload
    s_LastWriteTime = std::filesystem::last_write_time(assemblyPath);
}

void MonoManager::ReloadScripts()
{
    std::cout << "[Mono] Reloading scripts..." << std::endl;

    //Clean up old script instances
    CrashoutsEngine::ScriptSystem::DestroyAll();

    //Unload old assembly
    if (s_Assembly)
    {
        mono_assembly_close(s_Assembly);
        s_Assembly = nullptr;
    }

    //Reload new assembly into same domain
    LoadAssembly(s_AssemblyPath);

    CrashoutsEngine::ScriptSystem::ReloadAll();

    std::cout << "[Mono] Hot reload completed\n";
}


void MonoManager::CheckHotReload()
{
    auto currentTime = std::filesystem::last_write_time(s_AssemblyPath);
    if (currentTime != s_LastWriteTime)
    {
        s_LastWriteTime = currentTime;
        ReloadScripts();
    }
}


void MonoManager::Shutdown()
{
    if (!s_Domain)
    {
        return;
    }

	mono_jit_cleanup(s_Domain);
	s_Domain = nullptr;
	s_Assembly = nullptr;
	std::cout << "[Mono] Domain cleaned up\n";
}

MonoImage* MonoManager::GetImage()
{
    if (!s_Assembly)
        return nullptr;
    return mono_assembly_get_image(s_Assembly);
}

std::string MonoManager::CopyAssemblyToTemp(const std::string& originalPath)
{
    namespace fs = std::filesystem;

    fs::path src = originalPath;
    fs::path dst = "scripts/_cache";
    fs::create_directories(dst);

    fs::path tempFile = dst / src.filename();
    fs::copy_file(src, tempFile, fs::copy_options::overwrite_existing);

    return tempFile.string();
}
