#pragma once

#include "undomanager.h"
#include "entitymanager.h"

namespace CrashoutsEngine {

    template <typename T>
    class AddComponentCommand : public EditorCommand {
    public:
        explicit AddComponentCommand(Entity entity)
            : m_entity(entity) {
        }

        void Execute() override
        {
            ECS& ecs = *ECS::getInstance();

            m_alreadyHad = ecs.HasComponent<T>(m_entity);
            if (m_alreadyHad)
                return;

            ecs.AddComponent<T>(m_entity);
        }

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