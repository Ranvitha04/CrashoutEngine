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

		CE::u64 EntityID = 0;

        ScriptComponent() = default;
        ScriptComponent(const std::string& name, CE::u64 ID) : ClassName(name), EntityID(ID) {}

        bool Initialize(MonoDomain* domain, MonoAssembly* assembly);
        void Start();
        void Update(float dt);
        void Destroy();
    };
}
