#pragma once
#include <string>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>

namespace CrashoutsEngine
{
    struct ScriptComponent
    {
        std::string ClassName; // e.g. "PlayerScript" when i get to it lmao
        MonoObject* Instance = nullptr;
        MonoClass* Class = nullptr;
        MonoMethod* MethodStart = nullptr;
        MonoMethod* MethodUpdate = nullptr;
        bool Started = false;

        ScriptComponent() = default;
        ScriptComponent(const std::string& className) : ClassName(className) {}

        bool Initialize(MonoDomain* domain, MonoAssembly* assembly);
        void Start();
        void Update(float deltaTime);
        void Destroy();
    };
}
