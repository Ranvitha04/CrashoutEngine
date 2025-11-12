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
#include "camera.h"
#include "collidercomponent.h"

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
    float z = RenderCamera::engineCamera.GetFar();
    CEVec3 p0(minP.x, minP.y, z);
    CEVec3 p1(maxP.x, minP.y, z);
    CEVec3 p2(maxP.x, maxP.y, z);
    CEVec3 p3(minP.x, maxP.y, z);

    CEVec2 size = {thickness, thickness};

    Shapes::CreateLine(p0, p1, thickness, color);
    Shapes::CreateLine(p1, p2, thickness, color);
    Shapes::CreateLine(p2, p3, thickness, color);
    Shapes::CreateLine(p3, p0, thickness, color);
}

/*!
 @brief Draws a 2D oriented bounding box as a wireframe rectangle.
 @param vertices     A vertex array
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawOBBOutline(const Collision::OBBRect* vertices, float thickness, const CEVec3& color) {
    float z = RenderCamera::engineCamera.GetFar();
    // Create new vertices with the modified Z value
    CEVec3 v0 = { vertices->rotatedVertices[0].x, vertices->rotatedVertices[0].y, z };
    CEVec3 v1 = { vertices->rotatedVertices[1].x, vertices->rotatedVertices[1].y, z };
    CEVec3 v2 = { vertices->rotatedVertices[2].x, vertices->rotatedVertices[2].y, z };
    CEVec3 v3 = { vertices->rotatedVertices[3].x, vertices->rotatedVertices[3].y, z };

    // Draw the lines using the new, z-modified vertices
    Shapes::CreateLine(v0, v1, thickness, color);
    Shapes::CreateLine(v1, v2, thickness, color);
    Shapes::CreateLine(v2, v3, thickness, color);
    Shapes::CreateLine(v3, v0, thickness, color);
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
    float z = RenderCamera::engineCamera.GetFar();

    for (int i = 1; i <= segments; ++i) {
        float t = (twoPi * i) / segments;
        CEVec3 next(c.x + r * std::cos(t), c.y + r * std::sin(t), z);
        Shapes::CreateLine(prev, next, thickness, color);
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
        const CEVec3 ok(0.15f, 0.8f, 0.55f);    // non-colliding tint
        const CEVec3 hit(1.0f, 0.3f, 0.3f);     // colliding tint

        for (auto e : collArr.indexToEntity) {
            Component::ColliderComponent const & pColliderComp = *ecs.GetComponent<Component::ColliderComponent>(e);

            if (!pColliderComp.collider) continue;

            const CEVec3 color = IsColliding(e, events) ? hit : ok;

            if (pColliderComp.collider->type == Collision::SPHERE) {
                Collision::Sphere const& sphereTemp = dynamic_cast<Collision::Sphere const&>(*pColliderComp.collider);
                DrawCircleOutline(sphereTemp.center, sphereTemp.radius, 48, wireThickness, color);
            }
            else if (pColliderComp.collider->type == Collision::AABB) {
                Collision::AABBRect const& aabbTemp = dynamic_cast<Collision::AABBRect const&>(*pColliderComp.collider);
                DrawAABBOutline(aabbTemp.min, aabbTemp.max, wireThickness, color);
            }
            else if (pColliderComp.collider->type == Collision::OBB) {
                Collision::OBBRect const& obbTemp = dynamic_cast<Collision::OBBRect const&>(*pColliderComp.collider);
                DrawOBBOutline(&obbTemp, wireThickness, color);
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
                CEVec2 size = { 1.f, 1.f };
                Shapes::CreateLine(CEVec3(p.x - d, p.y, 0), CEVec3(p.x + d, p.y, 0), wireThickness, ptClr);
                Shapes::CreateLine(CEVec3(p.x, p.y - d, 0), CEVec3(p.x, p.y + d, 0), wireThickness, ptClr);
            }

            if (!ev.colManifold.contactPoints.empty()) {
                const auto& cp0 = ev.colManifold.contactPoints[0];
                CEVec3 p0(cp0.x, cp0.y, 0.0f);
                CEVec3 p1(cp0.x + ev.colManifold.normal.x * normalLen,
                    cp0.y + ev.colManifold.normal.y * normalLen, 0.0f);
                CEVec2 size = { 1.f, 1.f };
                Shapes::CreateLine(p0, p1, wireThickness, nClr);
            }
        }
    }

} // namespace CollisionGizmos