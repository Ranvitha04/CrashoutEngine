// hierarchy.cpp
// Parent–child relationships for entities.
// This version is defensive: it never overwrites an existing ChildrenComponent,
// prevents self/duplicate parenting, and offers simple helpers.

#include <algorithm>                 // std::find, std::remove
#include <utility>                   // std::move
#include "hierarchy.h"
#include "hierarchy_components.h"
#include "entitymanager.h"
#include <cmath> 
// If you want transform propagation, define HIERARCHY_ENABLE_TRANSFORM_PROPAGATION
// and implement the section at the bottom that matches your TransformComponent.

namespace CrashoutsEngine {

    using namespace Component;

    // ---------------------------------------------------------------------
    // Small helpers (internal)
    // ---------------------------------------------------------------------

    static inline ChildrenComponent* GetOrAddChildren(ECS& ecs, Entity parent) {
        if (ecs.HasComponent<ChildrenComponent>(parent))
            return ecs.GetComponent<ChildrenComponent>(parent);
        return &ecs.AddComponent<ChildrenComponent>(parent, ChildrenComponent{});
    }

    static inline ParentComponent* GetOrAddParent(ECS& ecs, Entity child) {
        if (ecs.HasComponent<ParentComponent>(child))
            return ecs.GetComponent<ParentComponent>(child);
        return &ecs.AddComponent<ParentComponent>(child, ParentComponent{});
    }

    static inline bool IsAlreadyChildOf(ECS& ecs, Entity parent, Entity child) {
        if (!ecs.HasComponent<ChildrenComponent>(parent)) return false;
        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        return std::find(kids->children.begin(), kids->children.end(), child) != kids->children.end();
    }

    static inline void AddChildLink(ECS& ecs, Entity parent, Entity child) {
        auto* kids = GetOrAddChildren(ecs, parent);
        if (!IsAlreadyChildOf(ecs, parent, child))
            kids->children.push_back(child);
    }

    static inline void RemoveChildLink(ECS& ecs, Entity parent, Entity child) {
        if (!ecs.HasComponent<ChildrenComponent>(parent)) return;
        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        auto it = std::remove(kids->children.begin(), kids->children.end(), child);
        kids->children.erase(it, kids->children.end());
    }

    // ---------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------
    void AttachChild(ECS& ecs, Entity parent, Entity child) {
        if (parent == 0 || child == 0 || parent == child) return;

        // If already parented, clean up first
        if (ecs.HasComponent<Component::ParentComponent>(child)) {
            auto* old = ecs.GetComponent<Component::ParentComponent>(child);
            if (old->parent == parent) return;
            if (old->parent != 0) RemoveChildLink(ecs, old->parent, child);
        }

        // --- Capture local offsets -------------------------------------------
        using Component::TransformComponent;
        auto* ppc = GetOrAddParent(ecs, child);
        ppc->parent = parent;

        if (ecs.HasComponent<TransformComponent>(parent) &&
            ecs.HasComponent<TransformComponent>(child)) {

            auto* pt = ecs.GetComponent<TransformComponent>(parent);
            auto* ct = ecs.GetComponent<TransformComponent>(child);

            // Local rotation/scale relative to parent
            ppc->localRot = ct->rotation - pt->rotation;
            ppc->localScale = { ct->size.x / (pt->size.x == 0.f ? 1.f : pt->size.x),
                                ct->size.y / (pt->size.y == 0.f ? 1.f : pt->size.y) };

            // Local position = inverse(parent rotation) * (childPos - parentPos)
            const float s = std::sin(-pt->rotation);
            const float c = std::cos(-pt->rotation);
            const float dx = ct->position.x - pt->position.x;
            const float dy = ct->position.y - pt->position.y;
            ppc->localPos = { c * dx - s * dy, s * dx + c * dy, ct->position.z - pt->position.z };
        }
        else {
            // Fallback if no transforms: zero offsets
            ppc->localPos = { 0,0,0 };
            ppc->localRot = 0.f;
            ppc->localScale = { 1.f,1.f };
        }

        // link child to parent's list (no re-adding/overwriting children component)
        AddChildLink(ecs, parent, child);
    }

    void DetachChild(ECS& ecs, Entity child) {
        if (!ecs.HasComponent<ParentComponent>(child)) return;
        auto* ppc = ecs.GetComponent<ParentComponent>(child);
        Entity parent = ppc->parent;
        if (parent != 0) {
            RemoveChildLink(ecs, parent, child);
        }
        // Either clear or remove ParentComponent. Removing keeps things tidy.
        ecs.RemoveComponent<ParentComponent>(child);
    }

    void Reparent(ECS& ecs, Entity child, Entity newParent) {
        if (child == 0 || newParent == 0 || child == newParent) return;
        // No-op if already child of newParent
        if (ecs.HasComponent<ParentComponent>(child)) {
            auto* ppc = ecs.GetComponent<ParentComponent>(child);
            if (ppc->parent == newParent) return;
        }
        DetachChild(ecs, child);
        AttachChild(ecs, newParent, child);
    }

    // Optional: Remove a parent from an entity, but keep its existing children.
    // This is useful when turning a former child into a new root.
    void ClearParentButKeepChildren(ECS& ecs, Entity entity) {
        if (ecs.HasComponent<ParentComponent>(entity)) {
            ecs.RemoveComponent<ParentComponent>(entity);
        }
    }

    // ---------------------------------------------------------------------
    // Update / validation
    // ---------------------------------------------------------------------

    // Ensures the hierarchy remains consistent (no dangling links, no duplicates).
    static void ValidateHierarchy(ECS& ecs) {
        // For each parent with ChildrenComponent: remove duplicates and broken links
        auto& arr = ecs.GetComponentArray<ChildrenComponent>();
        for (Entity parent : arr.indexToEntity) {
            if (!ecs.HasComponent<ChildrenComponent>(parent)) continue;
            auto* kids = ecs.GetComponent<ChildrenComponent>(parent);

            // Remove duplicates
            std::vector<Entity> unique;
            unique.reserve(kids->children.size());
            for (Entity c : kids->children) {
                if (c == 0 || c == parent) continue;
                if (std::find(unique.begin(), unique.end(), c) == unique.end())
                    unique.push_back(c);
            }
            kids->children.swap(unique);

            // Align each child's ParentComponent to this parent
            for (Entity c : kids->children) {
                auto* ppc = GetOrAddParent(ecs, c);
                ppc->parent = parent;
            }
        }

        // For each child that claims a parent: make sure parent links back
        auto& parr = ecs.GetComponentArray<ParentComponent>();
        for (Entity child : parr.indexToEntity) {
            if (!ecs.HasComponent<ParentComponent>(child)) continue;
            auto* ppc = ecs.GetComponent<ParentComponent>(child);
            Entity parent = ppc->parent;
            if (parent == 0 || parent == child) {
                ecs.RemoveComponent<ParentComponent>(child);
                continue;
            }
            AddChildLink(ecs, parent, child);
        }
    }

    // ---------------------------------------------------------------------
    // Frame update hook
    // ---------------------------------------------------------------------


    void UpdateHierarchy(ECS& ecs)
    {
        // (If you have a ValidateHierarchy(...) call, keep it first)
        ValidateHierarchy(ecs);

        using Component::TransformComponent;
        using Component::ParentComponent;
        using Component::ChildrenComponent;

        // --- compose child's world from parent ⊕ child's local offset -----------
        auto applyWorldFrom = [&](Entity parent, Entity child) {
            auto* pt = ecs.GetComponent<TransformComponent>(parent);
            auto* ct = ecs.GetComponent<TransformComponent>(child);
            auto* ppc = ecs.GetComponent<ParentComponent>(child);
            if (!pt || !ct || !ppc) return;

            // scale
            ct->size.x = pt->size.x * ppc->localScale.x;
            ct->size.y = pt->size.y * ppc->localScale.y;

            // rotation
            const float rot = pt->rotation + ppc->localRot;
            ct->rotation = rot;

            // position = parentPos + R(parentRot) * localPos
            const float s = std::sin(pt->rotation);
            const float c = std::cos(pt->rotation);
            const float lx = ppc->localPos.x;
            const float ly = ppc->localPos.y;
            const float wx = pt->position.x + (c * lx - s * ly);
            const float wy = pt->position.y + (s * lx + c * ly);
            const float wz = pt->position.z + ppc->localPos.z;
            ct->position = { wx, wy, wz };
            };

        // DFS from roots (entities that have children but no parent)
        std::function<void(Entity)> dfs = [&](Entity p) {
            auto* kids = ecs.GetComponent<ChildrenComponent>(p);
            if (!kids) return;
            for (Entity c : kids->children) {
                applyWorldFrom(p, c);
                dfs(c);
            }
            };

        // start from roots
        for (Entity e : ecs.GetComponentArray<ChildrenComponent>().indexToEntity) {
            if (!ecs.HasComponent<ChildrenComponent>(e)) continue;
            if (ecs.HasComponent<ParentComponent>(e)) continue;
            dfs(e);
        }
    }


} // namespace CrashoutsEngine
