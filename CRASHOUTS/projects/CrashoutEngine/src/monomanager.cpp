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

    namespace fs = std::filesystem;
    if (fs::exists(s_AssemblyPath))
        s_LastWriteTime = fs::last_write_time(s_AssemblyPath);
    else
        s_LastWriteTime = fs::file_time_type::clock::now(); // placeholder
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

    CrashoutsEngine::ScriptSystem::DestroyAll();

    // Remove the old assembly reference
    s_Assembly = nullptr;

    try
    {
        namespace fs = std::filesystem;

        // Prepare cache folder
        fs::path src = "scripts/ManagedScripts.dll";
        fs::path dstDir = "scripts/_cache";
        fs::create_directories(dstDir);

        // Create a unique filename to avoid file-lock errors
        std::string uniqueName = "ManagedScripts_reload_" + std::to_string(std::time(nullptr)) + ".dll";
        fs::path dst = dstDir / uniqueName;

        // Copy source DLL to a unique cache DLL
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);

        // Load from this unique file
        s_Assembly = mono_domain_assembly_open(s_Domain, dst.string().c_str());
        if (!s_Assembly)
        {
            std::cerr << "[Mono] Reload failed to open assembly!\n";
            return;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "[Mono] File copy failed: " << e.what() << '\n';
        return;
    }

    if (!s_Assembly)
    {
        std::cerr << "[Mono] Reload failed to open assembly!\n";
        return;
    }

    CrashoutsEngine::ScriptSystem::RegisterAllBindings();
    CrashoutsEngine::ScriptSystem::ReloadAll();

    std::cout << "[Mono] Hot reload completed\n";
}

void MonoManager::CheckHotReload()
{
    namespace fs = std::filesystem;

    try
    {
        if (!fs::exists(s_AssemblyPath))
            return; // no DLL yet then just skip bruh

        auto currentTime = fs::last_write_time(s_AssemblyPath);
        if (currentTime != s_LastWriteTime)
        {
            s_LastWriteTime = currentTime;
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // small FS settle delay
            ReloadScripts();
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "[Mono] Hot reload check failed: " << e.what() << '\n';
    }
}


void MonoManager::Shutdown()
{
    if (s_Domain)
    {
        mono_jit_cleanup(s_Domain);
        std::cout << "[Mono] Runtime cleaned up\n";
    }

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
