/*!
@file		scriptsystem.h
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Declaration of script system class

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <vector>
#include <map>
#include <string>
#include <variant>

// A container to hold the value of a single field
struct FieldBackup {
    int monoType; // To ensure we don't restore mismatching types
    std::variant<int, float, bool, std::string, CEVec3> value;
    // Add more types to variant as needed (CEVec2, CEColor, etc.)
};

// Map: ClassName -> FieldName -> Data
using ScriptBackupData = std::map<std::string, std::map<std::string, FieldBackup>>;

// Map: EntityID -> ScriptBackupData
using GlobalScriptBackup = std::map<uint32_t, ScriptBackupData>;

namespace CrashoutsEngine
{
    class ScriptSystem
    {
    public:
        static void InitializeAll(MonoDomain* domain, MonoAssembly* assembly);
        static void UpdateAll();
        static void FixedUpdateAll();
        static void DrawGizmosAll();
        static void RegisterAllBindings();

        static void DestroyAll();
        static void ReloadAll();
    };
}