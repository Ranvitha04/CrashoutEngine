#include "debugreflection.h"
#include "managers/entitymanager.h"
#include <imgui.h>

// current components:
#include "components/namecomponent.h"
#include "components/transformcomponent.h"
#include "components/shapescomponent.h"
#include "managers/physicsmanager.h"
#include "managers/collisionmanager.h"

#include "shapes.h"

using namespace CrashoutsEngine;

namespace {

    struct NameAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "NameComponent"; }
        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<Component::NameComponent>(e);
        }
        void add(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) ecs.AddComponent<Component::NameComponent>(e, "Unnamed Entity");
        }
        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<Component::NameComponent>(e);
        }
        void drawInspector(ECS& ecs, Entity e) override {
            if (auto* c = ecs.GetComponent<Component::NameComponent>(e)) {
                char buf[256]{};
                if (c->name.size() < sizeof(buf)) std::memcpy(buf, c->name.data(), c->name.size());
                if (ImGui::InputText("Name", buf, sizeof(buf))) c->name = buf;
            }
        }
    };

    struct TransformAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "TransformComponent"; }
        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<Component::TransformComponent>(e);
        }
        void add(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) ecs.AddComponent<Component::TransformComponent>(e);
        }
        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<Component::TransformComponent>(e);
        }
        void drawInspector(ECS& ecs, Entity e) override {
            if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) {
                float pos[3] = { t->position.x, t->position.y, t->position.z };
                float size[2] = { t->size.x, t->size.y };
                ImGui::DragFloat3("Position", pos, 0.1f);
                ImGui::DragFloat2("Scale", size, 0.05f);
                ImGui::DragFloat("Rotation", &t->rotation, 1.0f);
                t->position.x = pos[0]; t->position.y = pos[1]; t->position.z = pos[2];
                t->size.x = size[0]; t->size.y = size[1];
            }
        }
    };

    struct RigidBodyAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "RigidBody"; }

        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<RigidBodyComponent>(e);
        }

        void add(ECS& ecs, Entity e) override {
            // Defaults: max vel (10,10,0), mass 1.0, restitution 0.2  (tweak as you like)
            ecs.AddComponent<RigidBodyComponent>(e, CEVec3{ 10.f,10.f,0.f }, 1.f, 0.2f);
        }

        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<RigidBodyComponent>(e);
        }

        void drawInspector(ECS& ecs, Entity e) override {
            if (auto* rb = ecs.GetComponent<RigidBodyComponent>(e)) {
                // Velocity (editable)
                CEVec3 v = rb->getVelocity();
                float vel[3]{ v.x, v.y, v.z };
                if (ImGui::DragFloat3("Velocity", vel, 0.1f)) {
                    rb->setVelocity({ vel[0], vel[1], vel[2] });
                }

                // Mass & restitution (read-only with current API)
                ImGui::Text("Mass: %.3f", rb->getMass());
                ImGui::Text("Restitution: %.3f", rb->getCoeff());

                // One-shot force
                static float f[3]{ 0.f,0.f,0.f };
                ImGui::InputFloat3("Force to apply", f);
                ImGui::SameLine();
                if (ImGui::Button("Apply Force##rb")) {
                    rb->addForce({ f[0], f[1], f[2] });
                }
                if (ImGui::Button("Clear Force Accumulator##rb")) {
                    rb->clearForceAccumulator();
                }
            }
        }
    };

    struct ColliderAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "Collider"; }

        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<ColliderComponent>(e);
        }

        void add(ECS& ecs, Entity e) override {
            // Default to a 1.0-radius sphere at the entity's transform position (if present)
            CEVec3 center{ 0.f,0.f,0.f };
            if (auto* t = ecs.GetComponent<TransformComponent>(e)) center = t->position;

            auto col = std::make_unique<Collision::Sphere>(1.f, center);
            ecs.AddComponent<ColliderComponent>(e, ColliderComponent{ std::move(col) });
        }

        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<ColliderComponent>(e);
        }

        void drawInspector(ECS& ecs, Entity e) override {
            auto* cc = ecs.GetComponent<ColliderComponent>(e);
            if (!cc || !cc->collider) return;

            // Type selector
            int type = static_cast<int>(cc->collider->type);
            const char* items[] = { "Sphere","AABB" };
            ImGui::Combo("Type", &type, items, IM_ARRAYSIZE(items));

            // If type changed, rebuild collider (keep a sane default size)
            if (type != static_cast<int>(cc->collider->type)) {
                if (type == Collision::SPHERE) {
                    CEVec3 center{ 0.f,0.f,0.f };
                    if (auto* t = ecs.GetComponent<TransformComponent>(e)) center = t->position;
                    cc->collider = std::make_unique<Collision::Sphere>(1.f, center);
                }
                else {
                    // AABB centered on transform with ±0.5 extents
                    CEVec3 c{ 0.f,0.f,0.f };
                    if (auto* t = ecs.GetComponent<TransformComponent>(e)) c = t->position;
                    cc->collider = std::make_unique<Collision::AABBRect>(
                        CEVec3{ c.x - 0.5f, c.y - 0.5f, c.z - 0.5f },
                        CEVec3{ c.x + 0.5f, c.y + 0.5f, c.z + 0.5f }
                    );
                }
            }

            // Draw fields per type
            if (cc->collider->type == Collision::SPHERE) {
                auto* s = static_cast<Collision::Sphere*>(cc->collider.get());
                float r = s->radius;
                if (ImGui::DragFloat("Radius", &r, 0.05f, 0.0f, 1e6f)) s->radius = r;

                // Optionally sync sphere center to Transform each frame:
                if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
                    ImGui::TextDisabled("Center follows Transform (pos: %.2f, %.2f, %.2f)",
                        t->position.x, t->position.y, t->position.z);
                    s->center = t->position; // keep collider centered on entity
                }
                else {
                    float c[3]{ s->center.x, s->center.y, s->center.z };
                    if (ImGui::DragFloat3("Center", c, 0.1f)) s->center = { c[0], c[1], c[2] };
                }
            }
            else { // AABB
                auto* a = static_cast<Collision::AABBRect*>(cc->collider.get());
                float mn[3]{ a->min.x, a->min.y, a->min.z };
                float mx[3]{ a->max.x, a->max.y, a->max.z };
                bool changed = false;
                changed |= ImGui::DragFloat3("Min", mn, 0.05f);
                changed |= ImGui::DragFloat3("Max", mx, 0.05f);
                if (changed) {
                    // Ensure min <= max
                    for (int i = 0; i < 3; ++i) if (mn[i] > mx[i]) std::swap(mn[i], mx[i]);
                    a->min = { mn[0], mn[1], mn[2] };
                    a->max = { mx[0], mx[1], mx[2] };
                }
                // Optional helper: snap AABB around Transform
                if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
                    if (ImGui::Button("Fit around Transform##aabb")) {
                        const float hx = 0.5f * t->size.x;
                        const float hy = 0.5f * t->size.y;
                        a->min = { t->position.x - hx, t->position.y - hy, t->position.z - 0.5f };
                        a->max = { t->position.x + hx, t->position.y + hy, t->position.z + 0.5f };
                    }
                }
            }
        }
    };

    struct ShapeAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "Shape"; }
        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<Component::ShapeComponent>(e);
        }
        void add(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) ecs.AddComponent<Component::ShapeComponent>(e);
        }
        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<Component::ShapeComponent>(e);
        }
        void drawInspector(ECS& ecs, Entity e) override {
            using namespace Component;
            auto* sc = ecs.GetComponent<ShapeComponent>(e);
            if (!sc) return;

            // type
            int t = (int)sc->type;
            const char* items[] = { "Square","Rectangle","Line","Triangle" };
            if (ImGui::Combo("Type", &t, items, IM_ARRAYSIZE(items))) {
                sc->type = (ShapeType)t;
            }

            // shared
            ImGui::DragFloat3("Color", &sc->color.x, 0.01f, 0.f, 1.f);
            ImGui::DragFloat("Rotation", &sc->rotation, 1.f);
            ImGui::DragFloat("Scale", &sc->scale, 0.01f, 0.01f, 100.f);

            // per-type
            switch (sc->type) {
            case ShapeType::Square:
                ImGui::DragFloat("Size", &sc->size, 0.5f, 0.0f, 1e6f);
                break;
            case ShapeType::Rectangle:
                ImGui::DragFloat("Width", &sc->width, 0.5f, 0.0f, 1e6f);
                ImGui::DragFloat("Height", &sc->height, 0.5f, 0.0f, 1e6f);
                break;
            case ShapeType::Line:
                ImGui::DragFloat3("P1", &sc->p1.x, 0.5f);
                ImGui::DragFloat3("P2", &sc->p2.x, 0.5f);
                ImGui::DragFloat("Thickness", &sc->thickness, 0.1f, 0.0f, 1e6f);
                break;
            case ShapeType::Triangle:
                ImGui::DragFloat3("T1", &sc->t1.x, 0.5f);
                ImGui::DragFloat3("T2", &sc->t2.x, 0.5f);
                ImGui::DragFloat3("T3", &sc->t3.x, 0.5f);
                break;
            }

            if (ImGui::Button(sc->preview ? "Stop Preview##shape" : "Start Preview##shape")) {
                sc->preview = !sc->preview;
            }
        }
    };

    static NameAdapter gNameAdapter;
    static TransformAdapter gTransformAdapter;
    static RigidBodyAdapter gRigidBodyAdapter;
    static ColliderAdapter gColliderAdapter;
    static ShapeAdapter gShapeAdapter;

} // anon

// Call this once at startup (after ECS is ready)
extern "C" void RegisterBuiltinComponentAdapters() {
    using namespace DebugReflection;
    ComponentRegistry::instance().registerAdapter(&gNameAdapter);
    ComponentRegistry::instance().registerAdapter(&gTransformAdapter);
    ComponentRegistry::instance().registerAdapter(&gRigidBodyAdapter);
    ComponentRegistry::instance().registerAdapter(&gColliderAdapter);
    ComponentRegistry::instance().registerAdapter(&gShapeAdapter);
}