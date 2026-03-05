#pragma once

#include "undomanager.h"
#include "entitymanager.h"

namespace CrashoutsEngine {

    template <typename T>
    class RemoveComponentCommand : public EditorCommand {
    public:
        explicit RemoveComponentCommand(Entity entity)
            : m_entity(entity) {
        }

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