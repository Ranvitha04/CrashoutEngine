// hierarchy.cpp
#include <algorithm>                 // std::find, std::remove
#include "hierarchy.h"
#include "hierarchy_components.h"
#include "entitymanager.h"
#include "shapes.h"                  // for TransformComponent if you use it

namespace CrashoutsEngine {

    using namespace Component;

    static inline void AddChild(ECS& ecs, Entity parent, Entity child) {
        if (!ecs.HasComponent<ChildrenComponent>(parent))
            ecs.AddComponent<ChildrenComponent>(parent, ChildrenComponent{});

        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        // avoid duplicates
        if (std::find(kids->children.begin(), kids->children.end(), child) == kids->children.end())
            kids->children.push_back(child);

        if (!ecs.HasComponent<ParentComponent>(child))
            ecs.AddComponent<ParentComponent>(child, ParentComponent{ parent });
        else
            ecs.GetComponent<ParentComponent>(child)->parent = parent;
    }

    static inline void RemoveChildRef(ECS& ecs, Entity parent, Entity child) {
        if (!ecs.HasComponent<ChildrenComponent>(parent)) return;
        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        kids->children.erase(
            std::remove(kids->children.begin(), kids->children.end(), child),
            kids->children.end()
        );
    }

    void AttachChild(ECS& ecs, Entity parent, Entity child) {
        if (parent == 0 || child == 0 || parent == child) return;

        // if child already had a parent, detach first
        if (ecs.HasComponent<ParentComponent>(child)) {
            Entity old = ecs.GetComponent<ParentComponent>(child)->parent;
            if (old) RemoveChildRef(ecs, old, child);
        }
        AddChild(ecs, parent, child);
    }

    void DetachChild(ECS& ecs, Entity child) {
        if (!ecs.HasComponent<ParentComponent>(child)) return;

        Entity parent = ecs.GetComponent<ParentComponent>(child)->parent;
        if (parent) RemoveChildRef(ecs, parent, child);
        ecs.GetComponent<ParentComponent>(child)->parent = 0;
    }

    void Reparent(ECS& ecs, Entity child, Entity newParent) {
        DetachChild(ecs, child);
        if (newParent) AttachChild(ecs, newParent, child);
    }

    // Very simple propagation example
    static void PropagateFrom(ECS& ecs, Entity parent) {
        if (!ecs.HasComponent<TransformComponent>(parent)) return;

        auto* pt = ecs.GetComponent<TransformComponent>(parent);

        if (!ecs.HasComponent<ChildrenComponent>(parent)) return;
        const auto& kids = ecs.GetComponent<ChildrenComponent>(parent)->children;

        for (Entity c : kids) {
            if (!ecs.HasComponent<TransformComponent>(c)) continue;
            auto* ct = ecs.GetComponent<TransformComponent>(c);

            ct->position.x += pt->position.x;
            ct->position.y += pt->position.y;
            ct->position.z += pt->position.z;
            ct->rotation += pt->rotation;

            PropagateFrom(ecs, c);
        }
    }

    void UpdateHierarchy(ECS& ecs) {
        // roots = entities that have ChildrenComponent but no (or zero) ParentComponent
        const auto roots = ecs.GetComponentArray<ChildrenComponent>().indexToEntity;
        for (Entity e : roots) {
            if (!ecs.HasComponent<ParentComponent>(e) ||
                ecs.GetComponent<ParentComponent>(e)->parent == 0) {
                PropagateFrom(ecs, e);
            }
        }
    }

} // namespace CrashoutsEngine
