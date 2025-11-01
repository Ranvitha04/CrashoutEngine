#include "hierarchy.h"         
#include "entitymanager.h"
#include "shapes.h"             
namespace CrashoutsEngine {

    // ---------- Helpers to manage the relationship ----------
    static inline void AddChild(ECS& ecs, Entity parent, Entity child) {
        using namespace Component;
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
        using namespace Component;
        if (!ecs.HasComponent<ChildrenComponent>(parent)) return;
        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        kids->children.erase(std::remove(kids->children.begin(), kids->children.end(), child),
            kids->children.end());
    }

    void AttachChild(ECS& ecs, Entity parent, Entity child) {
        using namespace Component;
        if (parent == 0 || child == 0 || parent == child) return;

        // if child already had a parent, detach first
        if (ecs.HasComponent<ParentComponent>(child)) {
            Entity old = ecs.GetComponent<ParentComponent>(child)->parent;
            if (old) RemoveChildRef(ecs, old, child);
        }
        AddChild(ecs, parent, child);
    }

    void DetachChild(ECS& ecs, Entity child) {
        using namespace Component;
        if (!ecs.HasComponent<ParentComponent>(child)) return;

        Entity parent = ecs.GetComponent<ParentComponent>(child)->parent;
        if (parent) RemoveChildRef(ecs, parent, child);
        ecs.GetComponent<ParentComponent>(child)->parent = 0;
    }

    void Reparent(ECS& ecs, Entity child, Entity newParent) {
        DetachChild(ecs, child);
        if (newParent) AttachChild(ecs, newParent, child);
    }

    // ---------- Transform propagation (Unity-style) ----------
    static void PropagateFrom(ECS& ecs, Entity parent) {
        using namespace Component;
        if (!ecs.HasComponent<TransformComponent>(parent)) return;

        auto* pt = ecs.GetComponent<TransformComponent>(parent);

        if (!ecs.HasComponent<ChildrenComponent>(parent)) return;
        const auto& kids = ecs.GetComponent<ChildrenComponent>(parent)->children;

        for (Entity c : kids) {
            if (!ecs.HasComponent<TransformComponent>(c)) continue;
            auto* ct = ecs.GetComponent<TransformComponent>(c);

            // very simple “local = already stored in ct, parent adds to it” model
            ct->position.x += pt->position.x;
            ct->position.y += pt->position.y;
            ct->position.z += pt->position.z;
            ct->rotation += pt->rotation;               // simplistic; replace with your matrix math

            // recurse
            PropagateFrom(ecs, c);
        }
    }

    // Call this once per frame (before physics/render)
    void UpdateHierarchy(ECS& ecs) {
        using namespace Component;
        // find roots = entities with ChildrenComponent but no ParentComponent
        auto roots = ecs.GetComponentArray<ChildrenComponent>().indexToEntity;
        for (Entity e : roots) {
            if (!ecs.HasComponent<ParentComponent>(e) ||
                ecs.GetComponent<ParentComponent>(e)->parent == 0) {
                PropagateFrom(ecs, e);
            }
        }
    }

} // namespace CrashoutsEngine
