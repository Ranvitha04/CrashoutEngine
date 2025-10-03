// prefab_init.cpp
#include "prefab_init.h"
#include "prefab.h"

// Use the same ECS/component header your scenes use:
#include "managers/entitymanager.h"   // defines CrashoutsEngine::ECS and Component::*

using namespace CrashoutsEngine;
using namespace Component; 

// We register factories that receive ECS later, so no ECS parameter is needed here.
void InitPrefabs()
{
    // Fallback: create a plain named entity (and ensure it has a Transform)
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(ent)) {
            e.AddComponent<TransformComponent>(ent, TransformComponent{});
        }
        });

    // --- Example prefabs you can extend any time ---

    // Platform: wide rectangle
    Prefab::Register("Platform", [](ECS& e) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ "Platform" });
        e.AddComponent<TransformComponent>(ent, TransformComponent{});
        e.AddComponent<ShapeComponent>(ent, ShapeComponent{
            .type = ShapeType::Rectangle, .width = 100.f, .height = 10.f
            });
        // add collider/rigidbody/etc. here if desired
        });

    // Box: small rectangle
    Prefab::Register("Box", [](ECS& e) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ "Box" });
        e.AddComponent<TransformComponent>(ent, TransformComponent{});
        e.AddComponent<ShapeComponent>(ent, ShapeComponent{
            .type = ShapeType::Rectangle, .width = 20.f, .height = 20.f
            });
        });

    // Add more: Prefab::Register("Ball", [](ECS& e){ /* ... */ });
}
