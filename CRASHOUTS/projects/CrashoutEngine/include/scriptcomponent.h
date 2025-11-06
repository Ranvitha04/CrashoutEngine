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
#include "CETypes.h"

namespace CrashoutsEngine
{
    struct ScriptComponent
    {
		std::string ClassName; // e.g. "PlayerScript" which does basically nothing important right now
        MonoObject* Instance = nullptr;
        MonoClass* Class = nullptr;
        MonoMethod* MethodStart = nullptr;
        MonoMethod* MethodUpdate = nullptr;
        bool Started = false;

        uint32_t EntityID = 0;
        CE::u32 GCHandle=0;

        ScriptComponent() = default;
        ScriptComponent(const std::string& name, uint32_t ID) : ClassName(name), EntityID(ID) {}

        bool Initialize(MonoDomain* domain, MonoAssembly* assembly);
        void Start();
        void Update();
        void Destroy();
    };
}
