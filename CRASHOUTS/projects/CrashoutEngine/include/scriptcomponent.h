/*!
@file		scriptcomponent.h
@author     Shahrin Idzuan
@author     Mohamad Amir Akmal
@author     m.biniswandi
@date       06/11/2025
@brief		Declaration of script component struct

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
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
        // MonoObject* Instance = nullptr; // Removed: Use GCHandle
        MonoClass* Class = nullptr;
        MonoMethod* MethodStart = nullptr;
        MonoMethod* MethodUpdate = nullptr;
        MonoMethod* MethodLateUpdate = nullptr;
        MonoMethod* MethodFixedUpdate = nullptr;
        MonoMethod* MethodOnDrawGizmos = nullptr;
		MonoMethod* MethodOnDestroy = nullptr;
        bool Started = false;
        CE::u32 GCHandle = 0;

        ScriptInstance() = default;
        ScriptInstance(const std::string& name) : ClassName(name) {}
    };

}

namespace Component {

    struct ScriptComponent
    {
        uint32_t EntityID = 0;
        std::vector<CrashoutsEngine::ScriptInstance> Scripts;

        ScriptComponent() = default;

        ScriptComponent(const std::string& name, uint32_t ID) : EntityID(ID)
        {
            Scripts.emplace_back(name);
        }

        // Manage scripts
        CrashoutsEngine::ScriptInstance& AddScript(const std::string& className);
        void RemoveScript(size_t index);

        bool Initialize(MonoDomain* domain, MonoAssembly* assembly);
        static void ResetThunks();
        void Start();
        void Update();
        void LateUpdate();
        void FixedUpdate();
        void DrawGizmos();
        void Destroy();
    };

}
