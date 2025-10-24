/*!
@file       debugcollision.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025
@brief      Implements collision debug visualization for Crashout Engine.
@details    Renders wireframe overlays for all active colliders (AABBs & spheres),
            highlights entities participating in collisions, and draws contact
            points and normal vectors derived from the collision manifold.
//__________________________________________________________________________*/

#include "debugcollision.h"
#include <algorithm>
#include <cmath>

using namespace CrashoutsEngine;
using namespace Component;

/*                                                       internal helpers
----------------------------------------------------------------------------- */
/*!
 @brief Checks if an entity appears in any of the provided collision events.
 @param e            Entity to query.
 @param events       Optional pointer to a collision event list. If null, returns false.
 @return             True if the entity is part of at least one event; false otherwise.
*/
[[maybe_unused]] static bool IsColliding(Entity e, const std::vector<Collision::CollisionEvent>* events) {
    if (!events) return false;
    for (const auto& ev : *events) {
        if (ev.entity1 == e || ev.entity2 == e) return true;
    }
    return false;
}

/*!
 @brief Draws a 2D axis-aligned bounding box as a wireframe rectangle.
 @param minP         Minimum point (bottom-left).
 @param maxP         Maximum point (top-right).
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawAABBOutline(const CEVec3& minP, const CEVec3& maxP, float thickness, const CEVec3& color) {
    CEVec3 p0(minP.x, minP.y, 0.0f);
    CEVec3 p1(maxP.x, minP.y, 0.0f);
    CEVec3 p2(maxP.x, maxP.y, 0.0f);
    CEVec3 p3(minP.x, maxP.y, 0.0f);
    Shapes::CreateLine(p0, p1, thickness, 0.0f, 1.0f, color);
    Shapes::CreateLine(p1, p2, thickness, 0.0f, 1.0f, color);
    Shapes::CreateLine(p2, p3, thickness, 0.0f, 1.0f, color);
    Shapes::CreateLine(p3, p0, thickness, 0.0f, 1.0f, color);
}

/*!
 @brief Draws a 2D oriented bounding box as a wireframe rectangle.
 @param vertices     A vertex array
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawOBBOutline(const Collision::OBBRect* vertices, float thickness, const CEVec3& color) {
    Shapes::CreateLine(vertices->rotatedVertices[0], vertices->rotatedVertices[1], thickness, 0.0f, 1.0f, color);
    Shapes::CreateLine(vertices->rotatedVertices[1], vertices->rotatedVertices[2], thickness, 0.0f, 1.0f, color);
    Shapes::CreateLine(vertices->rotatedVertices[2], vertices->rotatedVertices[3], thickness, 0.0f, 1.0f, color);
    Shapes::CreateLine(vertices->rotatedVertices[3], vertices->rotatedVertices[0], thickness, 0.0f, 1.0f, color);
}

/*!
 @brief Draws a 2D circle wireframe (used for sphere colliders projected to XY).
 @param c            Center position.
 @param r            Radius.
 @param segments     Number of line segments (clamped to minimum of 8).
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawCircleOutline(const CEVec3& c, float r, int segments, float thickness, const CEVec3& color) {
    if (segments < 8) segments = 8;
    const float twoPi = 6.28318530718f;
    CEVec3 prev(c.x + r, c.y, 0.0f);
    for (int i = 1; i <= segments; ++i) {
        float t = (twoPi * i) / segments;
        CEVec3 next(c.x + r * std::cos(t), c.y + r * std::sin(t), 0.0f);
        Shapes::CreateLine(prev, next, thickness, 0.0f, 1.0f, color);
        prev = next;
    }
}

/*                                                public visualization apis
----------------------------------------------------------------------------- */
namespace CollisionGizmos {

    /*!
     @brief Draws wireframes for all colliders in the ECS; tints colliding ones.
     @param ecs            Reference to ECS.
     @param events         Optional collision events to determine tinting.
     @param wireThickness  Line thickness for the wire overlay.
     @note When @p events is provided, colliders for entities present in any event
           are tinted with a hit color to visually distinguish collisions.
    */
    void DrawAllColliders(ECS& ecs, [[maybe_unused]] std::vector<Collision::CollisionEvent>* events, float wireThickness) {
        auto& collArr = ecs.GetComponentArray<Component::ColliderComponent>();
        const CEVec3 ok(1.0f, 0.3f, 0.3f);    // non-colliding tint
        const CEVec3 hit(0.15f, 0.8f, 0.55f);     // colliding tint

        for (auto e : collArr.indexToEntity) {
            auto* pColliderComp = ecs.GetComponent<Component::ColliderComponent>(e);
            if (!pColliderComp || !pColliderComp->collider) continue;

            const CEVec3 color = IsColliding(e, events) ? ok : hit;
            const Collision::Collider* pBase = pColliderComp->collider.get();

            if (pBase->type == Collision::SPHERE) {
                auto const* pSphere = dynamic_cast<const Collision::Sphere*>(pBase);
                if (!pSphere) continue;
                DrawCircleOutline(pSphere->center, pSphere->radius, 48, wireThickness, color);
            }
            else if (pBase->type == Collision::AABB) {
                auto const* pAabb = dynamic_cast<const Collision::AABBRect*>(pBase);
                if (!pAabb) continue;
                DrawAABBOutline(pAabb->min, pAabb->max, wireThickness, color);
            }
            else if (pBase->type == Collision::OBB) {
                auto const* pAabb = dynamic_cast<const Collision::OBBRect*>(pBase);
                if (!pAabb) continue;
                DrawOBBOutline(pAabb, wireThickness, color);
            }
        }

    }

    /*!
     @brief Visualizes contact points as crosses and draws the manifold normal.
     @param events         List of collision events containing manifolds.
     @param normalLen      Length of the normal vector drawn from the first contact.
     @param wireThickness  Line thickness for points and normal.
     @note For each event, all contact points are marked with a small cross.
           If any contact exists, the manifold�s normal is drawn starting at
           the first contact point with length @p normalLen.
    */
    void DrawContactsAndNormals(const std::vector<Collision::CollisionEvent>& events, float normalLen, float wireThickness) {
        const CEVec3 ptClr(1.0f, 0.78f, 0.0f); // contact point color
        const CEVec3 nClr(1.0f, 1.0f, 0.0f);  // normal color

        for (const auto& ev : events) {
            for (const auto& cp : ev.colManifold.contactPoints) {
                CEVec3 p(cp.x, cp.y, 0.0f);
                const float d = 0.02f;
                Shapes::CreateLine(CEVec3(p.x - d, p.y, 0), CEVec3(p.x + d, p.y, 0), wireThickness, 0.0f, 1.0f, ptClr);
                Shapes::CreateLine(CEVec3(p.x, p.y - d, 0), CEVec3(p.x, p.y + d, 0), wireThickness, 0.0f, 1.0f, ptClr);
            }

            if (!ev.colManifold.contactPoints.empty()) {
                const auto& cp0 = ev.colManifold.contactPoints[0];
                CEVec3 p0(cp0.x, cp0.y, 0.0f);
                CEVec3 p1(cp0.x + ev.colManifold.normal.x * normalLen,
                    cp0.y + ev.colManifold.normal.y * normalLen, 0.0f);
                Shapes::CreateLine(p0, p1, wireThickness, 0.0f, 1.0f, nClr);
            }
        }
    }

} // namespace CollisionGizmos