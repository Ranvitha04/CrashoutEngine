#pragma once
#include <string>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
//#include <mono/metadata/appdomain.h>

class MonoManager
{
public:
    static void Init(const std::string& domainName = "CrashoutDomain");
    static void Shutdown();
    static void LoadAssembly(const std::string& assemblyPath);

    static MonoDomain* GetDomain() { return s_Domain; }
    static MonoAssembly* GetAssembly() { return s_Assembly; }
    static MonoImage* GetImage();


private:
    static MonoDomain* s_Domain;
    static MonoAssembly* s_Assembly; // store last loaded assembly
};
