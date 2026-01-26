/*!
@file		scriptsystem.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of script system functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "aibindings.h"
#include "animatorbindings.h"
#include "colliderbindings.h"
#include "collidereventbindings.h"
#include "ScriptSystem.h"
#include "entitymanager.h"
#include "InputBindings.h"
#include "monomanager.h"
#include "rigidbodybindings.h"
#include "ScriptComponent.h"
#include "TransformBindings.h"
#include "entitybindings.h"
#include "namebindings.h"
#include "audiobindings.h"
#include "scenebindings.h"
#include "uibindings.h"
#include "layerbindings.h"
#include "timebindings.h"
#include "shapebindings.h"
#include "activecomponent.h"

namespace CrashoutsEngine
{
    using namespace Component;
    // Helper to find a specific C# script instance on an entity
    static MonoObject* InternalGetScript(uint32_t entityID, MonoString* scriptName) {
        auto* ecs = ECS::getInstance();

        // 1. Check if entity has scripts
        if (!ecs->HasComponent<ScriptComponent>(entityID)) return nullptr;

        auto* sc = ecs->GetComponent<ScriptComponent>(entityID);
        if(!sc) return nullptr;
        
        // 2. Convert C# string to std::string
        char* cStr = mono_string_to_utf8(scriptName);
        std::string name(cStr);
        mono_free(cStr);

        // 3. Search the vector for the matching class name
        for (auto& instance : sc->Scripts) {
            if (instance.ClassName == name) {
                if (instance.GCHandle) {
                    return mono_gchandle_get_target(instance.GCHandle);
                }
                return nullptr;
            }
        }

        return nullptr;
    }

    void ScriptSystem::RegisterAllBindings()
	{
        Bindings::RegisterTransformBindings();
        Bindings::RegisterInputBindings();
        Bindings::RegisterRigidBodyBindings();
        Bindings::RegisterEntityBindings();
        Bindings::RegisterColliderBindings();
        Bindings::RegisterCollisionEventBindings();
        Bindings::RegisterNameBindings();
        Bindings::RegisterAudioBindings();
        Bindings::RegisterAnimatorBindings();
        Bindings::RegisterSceneBindings();
        Bindings::RegisterUIBindings();
        Bindings::RegisterLayerBindings();
        Bindings::RegisterDebugBindings();
        Bindings::RegisterAIBindings();
        Bindings::RegisterTimeBindings();
        Bindings::RegisterShapeBindings();

        mono_add_internal_call("CrashoutsEngine.InternalCalls::GetScript", (const void*)InternalGetScript);    
    }


    void ScriptSystem::InitializeAll(MonoDomain* domain, MonoAssembly* assembly)
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& scriptComp : scripts.dense)
        {
            scriptComp.Initialize(domain, assembly);
        }
    }

    void ScriptSystem::UpdateAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (size_t i = 0; i < scripts.dense.size(); ++i)
        {
            Entity entityID = scripts.indexToEntity[i];
            
            // --- ACTIVE CHECK ---
            if (ecs.HasComponent<ActiveComponent>(entityID)) {
                if (!IsEntityActive(ecs, entityID)) {
                    continue; // Skip Start/Update if disabled
                }
            }
            // --------------------

            // Re-fetch component safely before usage to handle potential reallocation
            if (ecs.HasComponent<ScriptComponent>(entityID))
            {
                ecs.GetComponent<ScriptComponent>(entityID)->Start();
            }

            // Re-fetch again for Update, as Start() might have triggered reallocation
            if (ecs.HasComponent<ScriptComponent>(entityID))
            {
                ecs.GetComponent<ScriptComponent>(entityID)->Update();
            }
        }
    }

    void ScriptSystem::FixedUpdateAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (size_t i = 0; i < scripts.dense.size(); ++i)
        {
            Entity entityID = scripts.indexToEntity[i];
            
            // --- ACTIVE CHECK ---
            if (ecs.HasComponent<ActiveComponent>(entityID)) {
                if (!IsEntityActive(ecs, entityID)) {
                    continue; // Skip FixedUpdate if disabled
                }
            }
            // --------------------

            // Re-fetch component safely
            if (ecs.HasComponent<ScriptComponent>(entityID))
            {
                ecs.GetComponent<ScriptComponent>(entityID)->FixedUpdate();
            }
        }
    }

    void ScriptSystem::DrawGizmosAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (size_t i = 0; i < scripts.dense.size(); ++i)
        {
            Entity entityID = scripts.indexToEntity[i];

            // --- ACTIVE CHECK ---
            if (ecs.HasComponent<ActiveComponent>(entityID)) {
                if (!IsEntityActive(ecs, entityID)) {
                    continue; 
                }
            }
            // --------------------

            if (ecs.HasComponent<ScriptComponent>(entityID))
            {
                ecs.GetComponent<ScriptComponent>(entityID)->DrawGizmos();
            }
        }
    }


    void ScriptSystem::DestroyAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& scriptComp : scripts.dense)
        {
            scriptComp.Destroy();
        }
        std::cout << "[Mono] All script instances destroyed.\n";
    }

    void ScriptSystem::ReloadAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& scriptComp : scripts.dense)
        {
            scriptComp.Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
        }
        std::cout << "[Mono] All scripts reinitialized after reload.\n";
    }


}
