/*!
@file       componentcommands.h
@author     Afiq
@date       13/03/2026
@brief      Template editor commands for adding/removing ECS components with undo support.

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
    class AddComponentCommand : public EditorCommand {
    public:
        /*!
        @brief Construct an add-component command for an entity.
        @param e Target entity to add the component to.
        */
        explicit AddComponentCommand(Entity e) : m_entity(e) {}

        /*!
        @brief Execute the command by adding the component if needed.
        */
        void Execute() override {
            ECS& ecs = *ECS::getInstance();
            m_alreadyHad = ecs.HasComponent<T>(m_entity);
            if (!m_alreadyHad) ecs.AddComponent<T>(m_entity);
        }

        /*!
        @brief Undo the command by removing the component if it was added.
        */
        void Undo() override {
            if (m_alreadyHad) return;
            ECS& ecs = *ECS::getInstance();
            if (ecs.HasComponent<T>(m_entity)) ecs.RemoveComponent<T>(m_entity);
        }

    private:
        Entity m_entity{};
        bool m_alreadyHad{};
    };

    /*!
    @class RemoveComponentCommand
    @brief Removes a component of type T from an entity while snapshotting it for undo.
    @tparam T Component type to remove.
    */
    template <typename T>
    class RemoveComponentCommand : public EditorCommand {
    public:
        /*!
        @brief Construct a remove-component command for an entity.
        @param e Target entity to remove the component from.
        */
        explicit RemoveComponentCommand(Entity e) : m_entity(e) {}

        /*!
        @brief Execute the command by removing the component and capturing its state.
        */
        void Execute() override {
            ECS& ecs = *ECS::getInstance();
            m_had = ecs.HasComponent<T>(m_entity);
            if (!m_had) return;

            m_snapshot = *ecs.GetComponent<T>(m_entity);
            ecs.RemoveComponent<T>(m_entity);
        }

        /*!
        @brief Undo the command by restoring the removed component and its captured state.
        */
        void Undo() override {
            if (!m_had) return;
            ECS& ecs = *ECS::getInstance();

            if (!ecs.HasComponent<T>(m_entity)) ecs.AddComponent<T>(m_entity);
            *ecs.GetComponent<T>(m_entity) = m_snapshot;
        }

    private:
        Entity m_entity{};
        bool m_had{};
        T m_snapshot{};
    };

} // namespace CrashoutsEngine
