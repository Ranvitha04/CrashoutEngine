#pragma once
#include "undomanager.h"
#include "entitymanager.h"

namespace CrashoutsEngine {

    // Works when ecs.AddComponent<T>(e) exists (no required args)
    template <typename T>
    class AddComponentCommand : public EditorCommand {
    public:
        explicit AddComponentCommand(Entity e) : m_entity(e) {}

        void Execute() override {
            ECS& ecs = *ECS::getInstance();
            m_alreadyHad = ecs.HasComponent<T>(m_entity);
            if (!m_alreadyHad) ecs.AddComponent<T>(m_entity);
        }

        void Undo() override {
            if (m_alreadyHad) return;
            ECS& ecs = *ECS::getInstance();
            if (ecs.HasComponent<T>(m_entity)) ecs.RemoveComponent<T>(m_entity);
        }

    private:
        Entity m_entity{};
        bool m_alreadyHad{};
    };

    // Works if component is copy-assignable AND ecs.AddComponent<T>(e) exists
    template <typename T>
    class RemoveComponentCommand : public EditorCommand {
    public:
        explicit RemoveComponentCommand(Entity e) : m_entity(e) {}

        void Execute() override {
            ECS& ecs = *ECS::getInstance();
            m_had = ecs.HasComponent<T>(m_entity);
            if (!m_had) return;

            m_snapshot = *ecs.GetComponent<T>(m_entity);
            ecs.RemoveComponent<T>(m_entity);
        }

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