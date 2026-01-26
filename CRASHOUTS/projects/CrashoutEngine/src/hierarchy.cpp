/*!
@file        hierarchy.cpp
@author      b.mohamadafiq@digipen.edu
@date        7/11/2025
@brief       Parent–child hierarchy: linking, reparenting, and world transform and color.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include <algorithm>                 
#include <utility>                  
#include "hierarchy.h"
#include "hierarchycomponent.h"
#include "entitymanager.h"
#include "namecomponent.h"
#include "transformcomponent.h"
#include "activecomponent.h"
#include "shapescomponent.h"
#include <cmath> 
#include "shapes.h"              
#include <glm/gtc/matrix_transform.hpp>

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
    /*
    static CEColor GetAncestorColor(ECS& ecs, Entity e) {
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
        return CEColor{ 1.f, 1.f, 1.f, 1.f };
    }
    // Force a child to take its parent's (ancestor) color every Update.
    static void ForceInheritColorFromParent(ECS& ecs, Entity child) {
        using namespace Component;

        if (!ecs.HasComponent<ParentComponent>(child)) return;
        if (!ecs.HasComponent<ShapeComponent>(child))  return;

        auto* parentComp = ecs.GetComponent<ParentComponent>(child);
        auto* shapeComp = ecs.GetComponent<ShapeComponent>(child);

        // Always inherit 
        const CEColor parentCol = GetAncestorColor(ecs, child);
        
        shapeComp->color.r = parentCol.r * parentComp->colorMul.r;
        shapeComp->color.g = parentCol.g * parentComp->colorMul.g;
        shapeComp->color.b = parentCol.b * parentComp->colorMul.b;
        shapeComp->color.a = parentCol.a * parentComp->colorMul.a;
        
    }
    */

    // Check if child is already in parent’s list
    bool IsAlreadyChildOf(ECS& ecs, Entity parent, Entity child) {
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
        pointerParentComp->colorMul = { 1.f, 1.f, 1.f, 1.f };

        if (ecs.HasComponent<TransformComponent>(parent) &&
            ecs.HasComponent<TransformComponent>(child)) {

            auto* parentTransform = ecs.GetComponent<TransformComponent>(parent);
            auto* childTransform = ecs.GetComponent<TransformComponent>(child);

            
            pointerParentComp->localRot = childTransform->rotation - parentTransform->rotation;

            
            // Safety check for zero scale
            float pX = (parentTransform->size.x == 0.f) ? 1.f : parentTransform->size.x;
            float pY = (parentTransform->size.y == 0.f) ? 1.f : parentTransform->size.y;
            pointerParentComp->localScale = { childTransform->size.x / pX, childTransform->size.y / pY };

            
            // We need the vector from Parent->Child, un-rotated and un-scaled by the parent.

            
            float dx = childTransform->position.x - parentTransform->position.x;
            float dy = childTransform->position.y - parentTransform->position.y;

            
            // Convert degrees to radians
            const float DEG_TO_RAD = 0.0174532925f;
            float angleRad = -parentTransform->rotation * DEG_TO_RAD; 
            float s = std::sin(angleRad);
            float c = std::cos(angleRad);

            // Un-rotated vector
            float unRotX = (c * dx) - (s * dy);
            float unRotY = (s * dx) + (c * dy);

            // C. Inverse Scale (Divide by Parent Scale)
            pointerParentComp->localPos = {
                unRotX / pX,
                unRotY / pY,
                childTransform->position.z - parentTransform->position.z 
            };
        }
        else {
            pointerParentComp->localPos = { 0,0,0.1f }; 
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
    /*
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

            // Use a loop that allows erasure
            for (auto it = kids->children.begin(); it != kids->children.end(); ) {
                Entity c = *it;

                if (!ecs.HasComponent<NameComponent>(c)) {
                    it = kids->children.erase(it); // Remove dead ID and Update iterator
                    continue;
                }

                auto* pointerParentComp = GetOrAddParent(ecs, c);
                pointerParentComp->parent = parent;

                ++it; // Only increment if we didn't erase
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
    */
   
    void RebuildHierarchyLinks(ECS& ecs)
    {
       
        {
            auto& childrenArr = ecs.GetComponentArray<Component::ChildrenComponent>();
            for (Entity e : childrenArr.indexToEntity)
            {
                if (!ecs.HasComponent<Component::ChildrenComponent>(e)) continue;
                ecs.GetComponent<Component::ChildrenComponent>(e)->children.clear();
            }
        }

        
        {
            auto& parentArr = ecs.GetComponentArray<Component::ParentComponent>();
            for (Entity child : parentArr.indexToEntity)
            {
                if (!ecs.HasComponent<Component::ParentComponent>(child)) continue;

                auto* pc = ecs.GetComponent<Component::ParentComponent>(child);
                Entity parent = pc->parent;

                // basic validation
                if (parent == 0 || parent == child) {
                    pc->parent = 0;
                    continue;
                }

                // parent might not exist anymore
                if (!ecs.HasComponent<Component::NameComponent>(parent)) {
                    pc->parent = 0;
                    continue;
                }

                AddChildLink(ecs, parent, child);   
            }
        }
    }
   
    /*static void RecomputeLocalFromWorld(
        const Component::TransformComponent& parentW,
        const Component::TransformComponent& childW,
        Component::ParentComponent& pc)
    {
        const float DEG_TO_RAD = 0.0174532925f;

        float pX = (parentW.size.x == 0.f) ? 1.f : parentW.size.x;
        float pY = (parentW.size.y == 0.f) ? 1.f : parentW.size.y;

        
        pc.localScale.x = childW.size.x / pX;
        pc.localScale.y = childW.size.y / pY;

        
        pc.localRot = childW.rotation - parentW.rotation;

        float dx = childW.position.x - parentW.position.x;
        float dy = childW.position.y - parentW.position.y;

        float angleRad = -parentW.rotation * DEG_TO_RAD;
        float s = std::sin(angleRad);
        float c = std::cos(angleRad);

        float unRotX = (c * dx) - (s * dy);
        float unRotY = (s * dx) + (c * dy);

        pc.localPos.x = unRotX / pX;
        pc.localPos.y = unRotY / pY;
        pc.localPos.z = childW.position.z - parentW.position.z;
    }*/

    // Per-frame propagation entry point.
    void UpdateHierarchy(ECS& ecs)
    {
       
        //ValidateHierarchy(ecs);

#if 0
        auto applyWorldFrom = [&](Entity parent, Entity child) {
            auto* parentTransform = ecs.GetComponent<TransformComponent>(parent);
            auto* childTransform = ecs.GetComponent<TransformComponent>(child);
            auto* pointerParentComp = ecs.GetComponent<ParentComponent>(child);

            if (!parentTransform || !childTransform || !pointerParentComp) return;

            // ----------------------------------------------------------
            // 1. SCALE (Multiplicative)
            // ----------------------------------------------------------
            childTransform->size.x = parentTransform->size.x * pointerParentComp->localScale.x;
            childTransform->size.y = parentTransform->size.y * pointerParentComp->localScale.y;

            // ----------------------------------------------------------
            // 2. ROTATION (Additive)
            // ----------------------------------------------------------
            childTransform->rotation = parentTransform->rotation + pointerParentComp->localRot;

            // ----------------------------------------------------------
            // 3. POSITION (Orbit Logic)
            // ----------------------------------------------------------
            // The child's local position must be SCALED by the parent's size,
            // and then ROTATED by the parent's rotation.

            const float DEG_TO_RAD = 0.0174532925f;
            float parentRad = parentTransform->rotation * DEG_TO_RAD;
            float s = std::sin(parentRad);
            float c = std::cos(parentRad);

            // A. Apply Parent Scale to the Offset
            // (If parent grows, child moves further away)
            float scaledOffsetX = pointerParentComp->localPos.x * parentTransform->size.x;
            float scaledOffsetY = pointerParentComp->localPos.y * parentTransform->size.y;

            // B. Rotate the Scaled Offset
            // Standard Rotation Matrix: x' = x*c - y*s, y' = x*s + y*c
            float rotatedX = (c * scaledOffsetX) - (s * scaledOffsetY);
            float rotatedY = (s * scaledOffsetX) + (c * scaledOffsetY);

            // C. Add to Parent World Position
            childTransform->position.x = parentTransform->position.x + rotatedX;
            childTransform->position.y = parentTransform->position.y + rotatedY;

            // D. Z-Axis (Simple addition for 2D layers)
            childTransform->position.z = parentTransform->position.z + pointerParentComp->localPos.z;

            //if (ecs.HasComponent<ColliderComponent>(child)) {
            //    auto* colliderComp = ecs.GetComponent<ColliderComponent>(child);
            //    if (colliderComp && colliderComp->collider) {
            //        // 1. Sync Position
            //        colliderComp->collider->UpdateCollider(childTransform->position);
            //
            //        // 2. Sync Rotation (Add parent's rotation influence)
            //        // If your OBB/Polygon supports rotation, pass the new child rotation
            //        colliderComp->collider->UpdateRotation(childTransform->rotation);
            //    }
            //}

            // ----------------------------------------------------------
            // 4. COLOR
            // ----------------------------------------------------------
            if (ecs.HasComponent<ShapeComponent>(parent)) {
                ForceInheritColorFromParent(ecs, child);
            }
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
#endif
 
    
        std::function<void(Entity, const TransformComponent*)> processChildren =
            [&](Entity parentEntity, const TransformComponent* parentTransform)
            {
                // 1. Get Children Component 
                auto* kids = ecs.GetComponent<ChildrenComponent>(parentEntity);
                if (!kids || kids->children.empty()) return;

                
                const float DEG_TO_RAD = 0.0174532925f;
                float parentRad = parentTransform->rotation * DEG_TO_RAD;
                float s = std::sin(parentRad);
                float c = std::cos(parentRad);

                // 3. Iterate Children
                for (Entity child : kids->children) {

                    // Get Child Components
                    auto* childTransform = ecs.GetComponent<TransformComponent>(child);
                    auto* pointerParentComp = ecs.GetComponent<ParentComponent>(child);

                    // Safety check
                    if (!childTransform || !pointerParentComp) continue;

                    // --- APPLY TRANSFORMS (Inline for speed) ---

                    // A. Scale
                    childTransform->size.x = parentTransform->size.x * pointerParentComp->localScale.x;
                    childTransform->size.y = parentTransform->size.y * pointerParentComp->localScale.y;

                    // B. Rotation
                    childTransform->rotation = parentTransform->rotation + pointerParentComp->localRot;

                    // C. Position (Orbit Logic using pre-calc Sine/Cosine)
                    float scaledOffsetX = pointerParentComp->localPos.x * parentTransform->size.x;
                    float scaledOffsetY = pointerParentComp->localPos.y * parentTransform->size.y;

                    float rotatedX = (c * scaledOffsetX) - (s * scaledOffsetY);
                    float rotatedY = (s * scaledOffsetX) + (c * scaledOffsetY);

                    childTransform->position.x = parentTransform->position.x + rotatedX;
                    childTransform->position.y = parentTransform->position.y + rotatedY;
                    childTransform->position.z = parentTransform->position.z + pointerParentComp->localPos.z;

                 

                    // 4. RECURSE
                    // Pass the CURRENT child as the NEW parent
                    processChildren(child, childTransform);
                }
            };

        // -------------------------------------------------------
        // MAIN LOOP: Only find Roots
        // -------------------------------------------------------
        // Optimization: Iterate NameComponent or TransformComponent if smaller than ChildrenComponent
        auto& childArray = ecs.GetComponentArray<ChildrenComponent>();

        for (Entity e : childArray.indexToEntity) {
            // We only start the recursion on ROOTS (Entities that have children but NO parent)
            if (!ecs.HasComponent<ParentComponent>(e)) {
                // Get the root transform once
                if (auto* rootTransform = ecs.GetComponent<TransformComponent>(e)) {
                    processChildren(e, rootTransform);
                }
            }
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
