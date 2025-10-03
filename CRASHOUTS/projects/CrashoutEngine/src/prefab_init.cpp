/*!
@file        prefab_init.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Sets up prefab templates for creating game objects.

*/
#include "prefab_init.h"
#include "prefab.h"
#include "managers/entitymanager.h"
#include "managers/collisionmanager.h"
#include <memory>

using namespace CrashoutsEngine;
using namespace Component;

// This helper function makes a collision box centered around a position
static std::unique_ptr<Collision::AABBRect> MakeCenteredAABB3(const CEVec3& center, float w, float h)
{
    const float hw = 0.5f * w, hh = 0.5f * h;
    CEVec3 mn{ center.x - hw, center.y - hh, center.z };
    CEVec3 mx{ center.x + hw, center.y + hh, center.z };
    return std::make_unique<Collision::AABBRect>(mn, mx);
}
//
void InitPrefabs()
{
    //Defaul prefab
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(ent))e.AddComponent<TransformComponent>(ent, TransformComponent{}); });
    Prefab::Register("Box", [](ECS& e) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ "Box" });
        TransformComponent t{ 0.f, CEVec2{}, CEVec3{ -2.5f, 60.f, 0.f } };
        e.AddComponent<TransformComponent>(ent, t);
        ShapeComponent sh{};
        sh.type = ShapeType::Rectangle;
        sh.width = 32.f;
        sh.height = 32.f;
        //Add collider,shape component and rigidbody
        e.AddComponent<ShapeComponent>(ent, sh);
        e.AddComponent<ColliderComponent>(ent, MakeCenteredAABB3(t.position, sh.width, sh.height));
        e.AddComponent<RigidBodyComponent>(ent, CEVec3{ 0.f, 0.f, 0.f }, 1.0f, 0.0f);
        });
}
