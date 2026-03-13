/*!
@file       createprefabcommand.cpp
@author     Afiq
@date       13/03/2026
@brief      Implementation of CreatePrefabCommand.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#include "createprefabcommand.h"

#include "prefab.h"
#include "entitymanager.h"
#include "transformcomponent.h"

namespace CrashoutsEngine {

    /*!
    @brief Construct a command to create a prefab using its default transform.
    @param prefabName Name/identifier of the prefab to instantiate.
    */
    CreatePrefabCommand::CreatePrefabCommand(const std::string& prefabName)
        : m_prefabName(prefabName) {
    }

    /*!
    @brief Construct a command to create a prefab at a given world position.
    @param prefabName Name/identifier of the prefab to instantiate.
    @param x X position.
    @param y Y position.
    @param z Z position.
    */
    CreatePrefabCommand::CreatePrefabCommand(const std::string& prefabName, float x, float y, float z)
        : m_prefabName(prefabName), m_usePosition(true), m_x(x), m_y(y), m_z(z) {
    }

    /*!
    @brief Instantiate the prefab and optionally place it at the requested position.
    */
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

    /*!
    @brief Destroy the instantiated prefab root entity if it exists.
    */
    void CreatePrefabCommand::Undo()
    {
        if (m_createdRoot == 0)
            return;

        ECS& ecs = *ECS::getInstance();
        ecs.DestroyEntity(m_createdRoot);
        m_createdRoot = 0;
    }

} // namespace CrashoutsEngine
