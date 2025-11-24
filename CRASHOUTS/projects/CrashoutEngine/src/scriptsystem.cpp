/*!
@file		scriptsystem.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of script system functions
*/
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

namespace CrashoutsEngine
{
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

        for (auto& scriptComp : scripts.dense)
        {
            scriptComp.Start();
            scriptComp.Update();
        }
    }

    void ScriptSystem::FixedUpdateAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& scriptComp : scripts.dense)
        {
            scriptComp.FixedUpdate();
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
