/*!
@file		scriptcomponent.h
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Declaration of script component struct
*/
#pragma once
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <string>
#include <iostream>
#include <vector>
#include "CETypes.h"

namespace CrashoutsEngine
{
    struct ScriptInstance
    {
        std::string ClassName; // e.g. "PlayerScript" that does something for once.
        MonoObject* Instance = nullptr;
        MonoClass* Class = nullptr;
        MonoMethod* MethodStart = nullptr;
        MonoMethod* MethodUpdate = nullptr;
        bool Started = false;
        CE::u32 GCHandle = 0;

        ScriptInstance() = default;
        ScriptInstance(const std::string& name) : ClassName(name) {}
    };

    struct ScriptComponent
    {
        uint32_t EntityID = 0;
        std::vector<ScriptInstance> Scripts;

        ScriptComponent() = default;

        ScriptComponent(const std::string& name, uint32_t ID) : EntityID(ID)
        {
            Scripts.emplace_back(name);
        }

        // Manage scripts
        ScriptInstance& AddScript(const std::string& className);
        void RemoveScript(size_t index);

        bool Initialize(MonoDomain* domain, MonoAssembly* assembly);
        void Start();
        void Update();
        void Destroy();
    };
}
