#include "ScriptSystem.h"
#include "entitymanager.h"
#include "ScriptComponent.h"

namespace CrashoutsEngine
{
    void ScriptSystem::InitializeAll(MonoDomain* domain, MonoAssembly* assembly)
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& script : scripts.dense)
        {
            if (!script.Instance)
                script.Initialize(domain, assembly);
        }
    }

    void ScriptSystem::UpdateAll(float dt)
    {
        auto& ecs = *ECS::getInstance();
        auto& scripts = ecs.GetComponentArray<ScriptComponent>();

        for (auto& script : scripts.dense)
        {
            if (!script.Started)
                script.Start();

            script.Update(dt);
        }
    }
}
