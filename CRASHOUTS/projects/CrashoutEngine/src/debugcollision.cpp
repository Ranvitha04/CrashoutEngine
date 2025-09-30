#include "debugcollision.h"
#include <algorithm>
#include <cmath>

using namespace CrashoutsEngine;
using namespace Component;

// check if entity is colliding
static bool IsColliding(Entity e, const std::vector<Collision::CollisionEvent>* events) {
    if (!events) return false;
    for (const auto& ev : *events) {
        if (ev.entity1 == e || ev.entity2 == e) return true;
    }
    return false;
}

// draw AABB as wireframe
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

// draw circle wireframe (for sphere collider)
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

namespace CollisionGizmos {

    // draw all colliders in ECS
    void DrawAllColliders(ECS& ecs, const std::vector<Collision::CollisionEvent>* events, float wireThickness) {
        auto& collArr = ecs.GetComponentArray<Component::ColliderComponent>();
        const CEVec3 ok(0.2f, 1.0f, 0.6f);
        const CEVec3 hit(1.0f, 0.3f, 0.3f);

        for (auto e : collArr.indexToEntity) {
            auto* cc = ecs.GetComponent<Component::ColliderComponent>(e);
            if (!cc || !cc->collider) continue;

            const CEVec3 color = /*IsColliding(e, events) ? hit : */  /*TEMP FOR NOW*/ ok;
            const Collision::Collider* base = cc->collider.get();

            if (base->type == Collision::SPHERE) {
                auto const* s = dynamic_cast<const Collision::Sphere*>(base);
                if (!s) continue;
                DrawCircleOutline(s->center, s->radius, 48, wireThickness, color);
            }
            else if (base->type == Collision::AABB) {
                auto const* a = dynamic_cast<const Collision::AABBRect*>(base);
                if (!a) continue;
                DrawAABBOutline(a->min, a->max, wireThickness, color);
            }
        }
    }

    // draw contact points + normals
    void DrawContactsAndNormals(const std::vector<Collision::CollisionEvent>& events,
        float normalLen, float wireThickness) {
        const CEVec3 ptClr(1.0f, 0.78f, 0.0f);
        const CEVec3 nClr(1.0f, 1.0f, 0.0f);

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