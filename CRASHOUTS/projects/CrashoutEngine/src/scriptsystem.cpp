/*!
@file		scriptsystem.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of script system functions
*/
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

namespace CrashoutsEngine
{
    void ScriptSystem::RegisterAllBindings()
	{
        Bindings::RegisterTransformBindings();
        Bindings::RegisterInputBindings();
        Bindings::RegisterRigidBodyBindings();
        Bindings::RegisterEntityBindings();
        Bindings::RegisterColliderBindings();
        //Bindings::RegisterCollisionEventBindings();
	}

    void ScriptSystem::InitializeAll(MonoDomain* domain, MonoAssembly* assembly)
    {
        //RegisterAllBindings();
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& script : scripts.dense)
        {
            if (!script.Instance)
                script.Initialize(domain, assembly);
        }

    }

    void ScriptSystem::UpdateAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& script : scripts.dense)
        {
            if (!script.Started)
                script.Start();

            script.Update();
        }

    }
    void ScriptSystem::DestroyAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& script : scripts.dense)
        {
            if (script.Instance)
            {
                script.Destroy();
                script.Instance = nullptr;
                script.MethodStart = nullptr;
                script.MethodUpdate = nullptr;
                script.Class = nullptr;
            }
            script.Started = false;
        }

        std::cout << "[Mono] All script instances destroyed.\n";
    }

    void ScriptSystem::ReloadAll()
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& script : scripts.dense)
        {
            script.Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
        }

        std::cout << "[Mono] All scripts reinitialized after reload.\n";
    }


}
