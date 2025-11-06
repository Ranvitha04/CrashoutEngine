#include <algorithm>                 
#include <utility>                  
#include "hierarchy.h"
#include "hierarchy_components.h"
#include "entitymanager.h"
#include <cmath> 
#include "shapes.h"              

namespace CrashoutsEngine {

    using namespace Component;


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


    // color
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
            // keep climbing
            e = p;
        }
        // no ancestor with a ShapeComponent -> default white
        return CEVec3{ 1.f, 1.f, 1.f };
    }

    // Force a child to take its parent's (ancestor) color every update.
 
    static void ForceInheritColorFromParent(ECS& ecs, Entity child) {
        using namespace Component;

        if (!ecs.HasComponent<ParentComponent>(child)) return;
        if (!ecs.HasComponent<ShapeComponent>(child))  return;

        auto* pc = ecs.GetComponent<ParentComponent>(child);
        auto* sc = ecs.GetComponent<ShapeComponent>(child);

        // Always inherit (you asked for "always take the parent")
        const CEVec3 parentCol = GetAncestorColor(ecs, child);

        
        sc->color.x = parentCol.x * pc->colorMul.x;
        sc->color.y = parentCol.y * pc->colorMul.y;
        sc->color.z = parentCol.z * pc->colorMul.z;
        
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


    void AttachChild(ECS& ecs, Entity parent, Entity child) {
        if (parent == 0 || child == 0 || parent == child) return;

   
        if (ecs.HasComponent<Component::ParentComponent>(child)) {
            auto* old = ecs.GetComponent<Component::ParentComponent>(child);
            if (old->parent == parent) return;
            if (old->parent != 0) RemoveChildLink(ecs, old->parent, child);
        }


        using Component::TransformComponent;
        auto* ppc = GetOrAddParent(ecs, child);
        ppc->parent = parent;

        if (ecs.HasComponent<TransformComponent>(parent) &&
            ecs.HasComponent<TransformComponent>(child)) {

            auto* pt = ecs.GetComponent<TransformComponent>(parent);
            auto* ct = ecs.GetComponent<TransformComponent>(child);

            ppc->localRot = ct->rotation - pt->rotation;
            ppc->localScale = { ct->size.x / (pt->size.x == 0.f ? 1.f : pt->size.x),
                                ct->size.y / (pt->size.y == 0.f ? 1.f : pt->size.y) };

            
            const float s = std::sin(-pt->rotation);
            const float c = std::cos(-pt->rotation);
            const float dx = ct->position.x - pt->position.x;
            const float dy = ct->position.y - pt->position.y;
            ppc->localPos = { c * dx - s * dy, s * dx + c * dy, ct->position.z - pt->position.z };
        }
        else {
           
            ppc->localPos = { 0,0,0 };
            ppc->localRot = 0.f;
            ppc->localScale = { 1.f,1.f };
        }

        
        AddChildLink(ecs, parent, child);
    }

    void DetachChild(ECS& ecs, Entity child) {
        if (!ecs.HasComponent<ParentComponent>(child)) return;
        auto* ppc = ecs.GetComponent<ParentComponent>(child);
        Entity parent = ppc->parent;
        if (parent != 0) {
            RemoveChildLink(ecs, parent, child);
        }
        
        ecs.RemoveComponent<ParentComponent>(child);
    }

    void Reparent(ECS& ecs, Entity child, Entity newParent) {
        if (child == 0 || newParent == 0 || child == newParent) return;
       
        if (ecs.HasComponent<ParentComponent>(child)) {
            auto* ppc = ecs.GetComponent<ParentComponent>(child);
            if (ppc->parent == newParent) return;
        }
        DetachChild(ecs, child);
        AttachChild(ecs, newParent, child);
    }

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
                auto* ppc = GetOrAddParent(ecs, c);
                ppc->parent = parent;
            }
        }

        
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

    void UpdateHierarchy(ECS& ecs)
    {
       
        ValidateHierarchy(ecs);

        using Component::TransformComponent;
        using Component::ParentComponent;
        using Component::ChildrenComponent;

     
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
