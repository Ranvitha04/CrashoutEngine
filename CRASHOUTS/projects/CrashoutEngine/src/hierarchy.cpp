/*!
@file        hierarchy.cpp
@author      b.mohamadafiq@digipen.edu
@date        7/11/2025
@brief       Parent–child hierarchy: linking, reparenting, and world transform and color.

*/
#include <algorithm>                 
#include <utility>                  
#include "hierarchy.h"
#include "hierarchy_components.h"
#include "entitymanager.h"
#include <cmath> 
#include "shapes.h"              

namespace CrashoutsEngine {

    using namespace Component;

    // Get or create a ChildrenComponent on a parent entity.
    static inline ChildrenComponent* GetOrAddChildren(ECS& ecs, Entity parent) {
        if (ecs.HasComponent<ChildrenComponent>(parent))
            return ecs.GetComponent<ChildrenComponent>(parent);
        return &ecs.AddComponent<ChildrenComponent>(parent, ChildrenComponent{});
    }

    // Get or create a ParentComponent on a child entity.
    static inline ParentComponent* GetOrAddParent(ECS& ecs, Entity child) {
        if (ecs.HasComponent<ParentComponent>(child))
            return ecs.GetComponent<ParentComponent>(child);
        return &ecs.AddComponent<ParentComponent>(child, ParentComponent{});
    }


    // inheritance helpers
    static CEVec3 GetAncestorColor(ECS& ecs, Entity e) {
        using namespace Component;
        // if this direct parent has ShapeComponent, use that;
        while (ecs.HasComponent<ParentComponent>(e)) {
            auto* pc = ecs.GetComponent<ParentComponent>(e);
            const Entity p = pc->parent;
            if (p == 0) break;

            if (ecs.HasComponent<ShapeComponent>(p)) {
                return ecs.GetComponent<ShapeComponent>(p)->color;
            }
           
            e = p;
        }
        // no ancestor with a ShapeComponent 
        return CEVec3{ 1.f, 1.f, 1.f };
    }
    // Force a child to take its parent's (ancestor) color every update.
    static void ForceInheritColorFromParent(ECS& ecs, Entity child) {
        using namespace Component;

        if (!ecs.HasComponent<ParentComponent>(child)) return;
        if (!ecs.HasComponent<ShapeComponent>(child))  return;

        auto* parentComp = ecs.GetComponent<ParentComponent>(child);
        auto* shapeComp = ecs.GetComponent<ShapeComponent>(child);

        // Always inherit 
        const CEVec3 parentCol = GetAncestorColor(ecs, child);

        
        shapeComp->color.x = parentCol.x * parentComp->colorMul.x;
        shapeComp->color.y = parentCol.y * parentComp->colorMul.y;
        shapeComp->color.z = parentCol.z * parentComp->colorMul.z;
        
    }

    // Check if child is already in parent’s list
    static inline bool IsAlreadyChildOf(ECS& ecs, Entity parent, Entity child) {
        if (!ecs.HasComponent<ChildrenComponent>(parent)) return false;
        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        return std::find(kids->children.begin(), kids->children.end(), child) != kids->children.end();
    }
     // Ensure parent’s list includes child.
    static inline void AddChildLink(ECS& ecs, Entity parent, Entity child) {
        auto* kids = GetOrAddChildren(ecs, parent);
        if (!IsAlreadyChildOf(ecs, parent, child))
            kids->children.push_back(child);
    }
    // Remove child id from parent’s list
    static inline void RemoveChildLink(ECS& ecs, Entity parent, Entity child) {
        if (!ecs.HasComponent<ChildrenComponent>(parent)) return;
        auto* kids = ecs.GetComponent<ChildrenComponent>(parent);
        auto it = std::remove(kids->children.begin(), kids->children.end(), child);
        kids->children.erase(it, kids->children.end());
    }

    // Create a parent→child relationship
    void AttachChild(ECS& ecs, Entity parent, Entity child) {
        if (parent == 0 || child == 0 || parent == child) return;

   
        if (ecs.HasComponent<Component::ParentComponent>(child)) {
            auto* old = ecs.GetComponent<Component::ParentComponent>(child);
            if (old->parent == parent) return;
            if (old->parent != 0) RemoveChildLink(ecs, old->parent, child);
        }

        auto* pointerParentComp = GetOrAddParent(ecs, child);
        pointerParentComp->parent = parent;

        if (ecs.HasComponent<TransformComponent>(parent) &&
            ecs.HasComponent<TransformComponent>(child)) {

            auto* parentTransform = ecs.GetComponent<TransformComponent>(parent);
            auto* childTransform = ecs.GetComponent<TransformComponent>(child);

            pointerParentComp->localRot = childTransform->rotation - parentTransform->rotation;
            pointerParentComp->localScale = { childTransform->size.x / (parentTransform->size.x == 0.f ? 1.f : parentTransform->size.x), childTransform->size.y / (parentTransform->size.y == 0.f ? 1.f : parentTransform->size.y) };
            const float s = std::sin(-parentTransform->rotation);
            const float c = std::cos(-parentTransform->rotation);
            const float dx = childTransform->position.x - parentTransform->position.x;
            const float dy = childTransform->position.y - parentTransform->position.y;
            pointerParentComp->localPos = { c * dx - s * dy, s * dx + c * dy, childTransform->position.z - parentTransform->position.z };
        }
        else {
           
            pointerParentComp->localPos = { 0,0,0 };
            pointerParentComp->localRot = 0.f;
            pointerParentComp->localScale = { 1.f,1.f };
        }

        
        AddChildLink(ecs, parent, child);
    }
    //Remove a child’s parent link.

    void DetachChild(ECS& ecs, Entity child) {
        if (!ecs.HasComponent<ParentComponent>(child)) return;
        auto* ppc = ecs.GetComponent<ParentComponent>(child);
        Entity parent = ppc->parent;
        if (parent != 0) {
            RemoveChildLink(ecs, parent, child);
        }
        
        ecs.RemoveComponent<ParentComponent>(child);
    }
    //Move child under a new parent.
    void Reparent(ECS& ecs, Entity child, Entity newParent) {
        if (child == 0 || newParent == 0 || child == newParent) return;
       
        if (ecs.HasComponent<ParentComponent>(child)) {
            auto* pointerParentComp = ecs.GetComponent<ParentComponent>(child);
            if (pointerParentComp->parent == newParent) return;
        }
        DetachChild(ecs, child);
        AttachChild(ecs, newParent, child);
    }
    // Keep Parent/Children structures consistent.
    static void ValidateHierarchy(ECS& ecs) {

        auto& arr = ecs.GetComponentArray<ChildrenComponent>();
        for (Entity parent : arr.indexToEntity) {
            if (!ecs.HasComponent<ChildrenComponent>(parent)) continue;
            auto* kids = ecs.GetComponent<ChildrenComponent>(parent);

            std::vector<Entity> unique;
            unique.reserve(kids->children.size());
            for (Entity c : kids->children) {
                if (c == 0 || c == parent) continue;
                if (std::find(unique.begin(), unique.end(), c) == unique.end())
                    unique.push_back(c);
            }
            kids->children.swap(unique);

         
            for (Entity c : kids->children) {
                auto* pointerParentComp = GetOrAddParent(ecs, c);
                pointerParentComp->parent = parent;
            }
        }

        
        auto& parentArray = ecs.GetComponentArray<ParentComponent>();
        for (Entity child : parentArray.indexToEntity) {
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
    // Per-frame propagation entry point.
    void UpdateHierarchy(ECS& ecs)
    {
       
        ValidateHierarchy(ecs);
     
        auto applyWorldFrom = [&](Entity parent, Entity child) {
            auto* parentTransform = ecs.GetComponent<TransformComponent>(parent);
            auto* childTransform = ecs.GetComponent<TransformComponent>(child);
            auto* pointerParentComp = ecs.GetComponent<ParentComponent>(child);
            if (!parentTransform || !childTransform || !pointerParentComp) return;

            // scale
            childTransform->size.x = parentTransform->size.x * pointerParentComp->localScale.x;
            childTransform->size.y = parentTransform->size.y * pointerParentComp->localScale.y;

            // rotation
            const float rot = parentTransform->rotation + pointerParentComp->localRot;
            childTransform->rotation = rot;

            // position = parentPos + R(parentRot) * localPos
            const float s = std::sin(parentTransform->rotation);
            const float c = std::cos(parentTransform->rotation);
            const float lx = pointerParentComp->localPos.x;
            const float ly = pointerParentComp->localPos.y;
            const float wx = parentTransform->position.x + (c * lx - s * ly);
            const float wy = parentTransform->position.y + (s * lx + c * ly);
            const float wz = parentTransform->position.z + pointerParentComp->localPos.z;
            childTransform->position = { wx, wy, wz };
            ForceInheritColorFromParent(ecs, child);
            };

        
        std::function<void(Entity)> dfs = [&](Entity p) {
            auto* kids = ecs.GetComponent<ChildrenComponent>(p);
            if (!kids) return;
            for (Entity c : kids->children) {
                applyWorldFrom(p, c);
                dfs(c);
            }
            };

   
        for (Entity e : ecs.GetComponentArray<ChildrenComponent>().indexToEntity) {
            if (!ecs.HasComponent<ChildrenComponent>(e)) continue;
            if (ecs.HasComponent<ParentComponent>(e)) continue;
            dfs(e);
        }
    }
    // Find top most ancestor of entity.
    Entity GetRoot(ECS& ecs, Entity e)
    {
        Entity cur = e;
        while (cur && ecs.HasComponent<Component::ParentComponent>(cur))
        {
            auto* pc = ecs.GetComponent<Component::ParentComponent>(cur);
            if (!pc->parent)
                break;
            cur = pc->parent;
        }
        return cur;
    }
    // Collect entity and all its descendants.
    void GatherDescendants(ECS& ecs, Entity e, std::vector<Entity>& out)
    {
        out.push_back(e);

        if (ecs.HasComponent<Component::ChildrenComponent>(e))
        {
            auto* cc = ecs.GetComponent<Component::ChildrenComponent>(e);
            for (Entity c : cc->children)
                GatherDescendants(ecs, c, out);
        }
    }


} 
