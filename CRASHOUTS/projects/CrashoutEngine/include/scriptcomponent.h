#pragma once
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <string>
#include <iostream>

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

        ScriptComponent() = default;
        ScriptComponent(const std::string& name) : ClassName(name) {}

        bool Initialize(MonoDomain* domain, MonoAssembly* assembly);
        void Start();
        void Update(float dt);
        void Destroy();
    };
}
