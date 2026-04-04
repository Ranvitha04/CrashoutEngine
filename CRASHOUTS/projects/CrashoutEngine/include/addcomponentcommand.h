/*!
@file       addcomponentcommand.h
@author     Afiq
@date       13/03/2026
@brief      Editor command for adding a component to an entity with undo support.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once

#include "undomanager.h"
#include "entitymanager.h"

namespace CrashoutsEngine {

    /*!
    @class AddComponentCommand
    @brief Adds a component of type T to an entity (no-op if already present).
    @tparam T Component type to add.
    */
    template <typename T>
    /** @brief AddComponentCommand */
    class AddComponentCommand : public EditorCommand {
    public:
        /*!
        @brief Construct an add-component command for an entity.
        @param entity Target entity to add the component to.
        */
        explicit AddComponentCommand(Entity entity)
            : m_entity(entity) {
        }

        /*!
        @brief Execute the command by adding the component if needed.
        */
        void Execute() override
        {
            ECS& ecs = *ECS::getInstance();

            m_alreadyHad = ecs.HasComponent<T>(m_entity);
            if (m_alreadyHad)
                return;

            ecs.AddComponent<T>(m_entity);
        }

        /*!
        @brief Undo the command by removing the component if it was added.
        */
        void Undo() override
        {
            if (m_alreadyHad)
                return;

            ECS& ecs = *ECS::getInstance();
            if (ecs.HasComponent<T>(m_entity))
                ecs.RemoveComponent<T>(m_entity);
        }

    private:
        Entity m_entity = 0;
        bool m_alreadyHad = false;
    };

} // namespace CrashoutsEngine
