#include "createprefabcommand.h"

#include "prefab.h"
#include "entitymanager.h"
#include "transformcomponent.h"

namespace CrashoutsEngine {

    CreatePrefabCommand::CreatePrefabCommand(const std::string& prefabName)
        : m_prefabName(prefabName) {
    }

    CreatePrefabCommand::CreatePrefabCommand(const std::string& prefabName, float x, float y, float z)
        : m_prefabName(prefabName), m_usePosition(true), m_x(x), m_y(y), m_z(z) {
    }

    void CreatePrefabCommand::Execute()
    {
        ECS& ecs = *ECS::getInstance();

        m_createdRoot = Prefab::Create(ecs, m_prefabName);
        if (m_createdRoot == 0)
            return;

        // If dropped into viewport, place the created entity directly.
        if (m_usePosition) {
            if (auto* t = ecs.GetComponent<Component::TransformComponent>(m_createdRoot)) {
                t->position.x = m_x;
                t->position.y = m_y;
                t->position.z = m_z;
            }
        }
    }

    void CreatePrefabCommand::Undo()
    {
        if (m_createdRoot == 0)
            return;

        ECS& ecs = *ECS::getInstance();
        ecs.DestroyEntity(m_createdRoot);
        m_createdRoot = 0;
    }

} // namespace CrashoutsEngine