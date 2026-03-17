/*!
@file       removecomponentcommand.h
@author     Afiq
@date       13/03/2026
@brief      Editor command for removing a component from an entity with undo support.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once

#include "undomanager.h"
#include "entitymanager.h"

namespace CrashoutsEngine {

    /*!
    @class RemoveComponentCommand
    @brief Removes a component of type T from an entity and snapshots it for undo.
    @tparam T Component type to remove.
    */
    template <typename T>
    class RemoveComponentCommand : public EditorCommand {
    public:
        /*!
        @brief Construct a remove-component command for an entity.
        @param entity Target entity to remove the component from.
        */
        explicit RemoveComponentCommand(Entity entity)
            : m_entity(entity) {
        }

        /*!
        @brief Execute the command by removing the component and capturing its state.
        */
        void Execute() override
        {
            ECS& ecs = *ECS::getInstance();

            m_hadComponent = ecs.HasComponent<T>(m_entity);
            if (!m_hadComponent)
                return;

            // Save exact component data before removing
            m_snapshot = *ecs.GetComponent<T>(m_entity);
            ecs.RemoveComponent<T>(m_entity);
        }

        /*!
        @brief Undo the command by restoring the removed component and its captured state.
        */
        void Undo() override
        {
            if (!m_hadComponent)
                return;

            ECS& ecs = *ECS::getInstance();

            if (!ecs.HasComponent<T>(m_entity))
                ecs.AddComponent<T>(m_entity);

            *ecs.GetComponent<T>(m_entity) = m_snapshot;
        }

    private:
        Entity m_entity = 0;
        bool m_hadComponent = false;
        T m_snapshot{};
    };

} // namespace CrashoutsEngine
