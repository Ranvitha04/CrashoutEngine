#pragma once
#include "entitymanager.h"
#include "scriptcomponent.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

namespace CrashoutsEngine
{
    class ScriptSystem
    {
    public:
        static void InitializeAll(MonoDomain* domain, MonoAssembly* assembly);
        static void UpdateAll();
        static void RegisterAllBindings();

        static void DestroyAll();
        static void ReloadAll();
    };
}