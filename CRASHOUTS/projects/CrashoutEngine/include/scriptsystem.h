/*!
@file		scriptsystem.h
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Declaration of script system class

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

namespace CrashoutsEngine
{
    class ScriptSystem
    {
    public:
        static void InitializeAll(MonoDomain* domain, MonoAssembly* assembly);
        static void UpdateAll();
        static void FixedUpdateAll();
        static void RegisterAllBindings();

        static void DestroyAll();
        static void ReloadAll();
    };
}