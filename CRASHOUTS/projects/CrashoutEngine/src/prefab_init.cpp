// prefab_init.cpp
#include "prefab_init.h"
#include "prefab.h"

// Engine headers used in scenes
#include "managers/entitymanager.h"
#include "managers/collisionmanager.h"

#include <memory>

using namespace CrashoutsEngine;
using namespace Component;

// Build centered AABB in XY using CEVec3 (Z preserved)
static std::unique_ptr<Collision::AABBRect> MakeCenteredAABB3(const CEVec3& center, float w, float h)
{
    const float hw = 0.5f * w, hh = 0.5f * h;
    CEVec3 mn{ center.x - hw, center.y - hh, center.z };
    CEVec3 mx{ center.x + hw, center.y + hh, center.z };
    // AABBRect ctor(min,max) — matches your debug adapters & tristan.cpp
    return std::make_unique<Collision::AABBRect>(mn, mx);
}

void InitPrefabs()
{
    // Fallback: always make a named entity with a Transform
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(ent))
            e.AddComponent<TransformComponent>(ent, TransformComponent{});
        });

    // ---------------- Box (32x32, solid) ----------------
    Prefab::Register("Box", [](ECS& e) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ "Box" });

        // Transform: same aggregate style you use in tristan.cpp
        TransformComponent t{ 0.f, CEVec2{}, CEVec3{ -2.5f, 60.f, 0.f } };
        e.AddComponent<TransformComponent>(ent, t);

        ShapeComponent sh{};
        sh.type = ShapeType::Rectangle;
        sh.width = 32.f;
        sh.height = 32.f;
        sh.color.x = 255.f; sh.color.y = 64.f; sh.color.z = 64.f;
        // color left at default; renderer multiplies by 1/255 in draw path
        e.AddComponent<ShapeComponent>(ent, sh);

        // Collider must exist for your renderer to draw the Shape
        e.AddComponent<ColliderComponent>(ent, MakeCenteredAABB3(t.position, sh.width, sh.height));

        // RigidBody: use the same ctor your debug UI calls (vel, mass, restitution)
        // If your type also supports aggregate, either works:
        e.AddComponent<RigidBodyComponent>(ent, CEVec3{ 0.f, 0.f, 0.f }, 1.0f, 0.0f);
        });

    // --------------- Platform (100x10, solid) ---------------
    Prefab::Register("Platform", [](ECS& e) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ "Platform" });

        TransformComponent t{ 0.f, CEVec2{}, CEVec3{ 0.f, 80.f, 0.f } };
        e.AddComponent<TransformComponent>(ent, t);

        ShapeComponent sh{};
        sh.type = ShapeType::Rectangle;
        sh.width = 100.f;
        sh.height = 10.f;
        e.AddComponent<ShapeComponent>(ent, sh);

        e.AddComponent<ColliderComponent>(ent, MakeCenteredAABB3(t.position, sh.width, sh.height));

        // Static body (mass 0) if you want it not to move
        e.AddComponent<RigidBodyComponent>(ent, CEVec3{ 0.f, 0.f, 0.f }, 0.0f, 0.0f);
        });
}
