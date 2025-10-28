/*!
@file       debugadapter.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

Declares and registers built-in Debug Menu adapters & providers:
- ECS component adapters: Name, Transform, RigidBody, Collider, MeshComponent
- GameStateProvider: bridges GameStateManager to the UI
- BuiltinPhysicsProvider: gravity & damping controls via reflection

These adapters make the ECS inspector and physics/game-state panels
runtime-extensible and project-agnostic.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "debugreflection.h"
#include "entitymanager.h"
#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <numeric>
#include "debugdefaults.h"

/*                                                         component includes
----------------------------------------------------------------------------- */
#include "namecomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "physicsmanager.h"
#include "collisionmanager.h"
#include "gamestatemanager.h"
#include "assetmanager.h"
#include "shapes.h"
#include "textcomponent.h"
#include "reflectimgui.h"
#include "audiocomponent.h"
#include "audioSystem.h"
#include "scriptcomponent.h"
#include "monomanager.h"

using namespace CrashoutsEngine;

//----------------------------------------------------------------------------- file-local state & helpers

namespace {
    f32 max(f32 x1, f32 y2, f32 z3) {
        return std::max(std::max(x1, y2), z3);
    }
}

/*!
 @brief File-local adapters/providers and registration.

 All adapters/providers live in an anonymous namespace and are registered
 once via RegisterBuiltinComponentAdapters(), which the app calls at startup.
*/
namespace {

    /*!
    @brief Adapter for editing Component::NameComponent (text name).
    @details Shows a single InputText and syncs the value back to the ECS component.
    */
    struct NameAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "NameComponent"; }
        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<Component::NameComponent>(e);
        }
        void add(ECS& ecs, Entity e) override {
            auto& D = GetDebugDefaults();
            if (!has(ecs, e)) ecs.AddComponent<Component::NameComponent>(e, D.name_default);
        }
        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<Component::NameComponent>(e);
        }
        void drawInspector(ECS& ecs, Entity e) override {
            if (auto* c = ecs.GetComponent<Component::NameComponent>(e)) {
                draw_reflected(*c, "Name");
            }
        }
    };

    /*!
    @brief Adapter for editing Component::TransformComponent (position/scale/rotation).
    @details Uses ImGui drag controls and writes values back to the ECS each frame.
    */
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
                draw_reflected(*t, "Transform");
            }
        }
    };

    /*!
    @brief Adapter for editing Component::RigidBodyComponent (velocity / forces).
    @details Displays editable velocity, read-only mass & restitution (with current API), and provides one-shot force utilities for quick testing.
    */
    struct RigidBodyAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "RigidBody"; }

        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<RigidBodyComponent>(e);
        }

        void add(ECS& ecs, Entity e) override {
            auto& D = GetDebugDefaults();
            ecs.AddComponent<RigidBodyComponent>(e, D.rb_init_velocity, D.rb_mass, D.rb_restitution);
        }

        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<RigidBodyComponent>(e);
        }

        void drawInspector(ECS& ecs, Entity e) override {
            if (auto* rb = ecs.GetComponent<RigidBodyComponent>(e)) {

                auto ref = rb->reflection();
                if (draw_reflected(ref, "RigidBody (data)")) {
                    *rb = RigidBodyComponent(ref);
                }

                ImGui::SeparatorText("Forces");
                static float f[3]{ 0.f, 0.f, 0.f };
                ImGui::InputFloat3("Force", f);
                ImGui::SameLine();
                if (ImGui::Button("Apply Force##rb")) rb->addForce({ f[0], f[1], f[2] });
                if (ImGui::Button("Clear Force Accumulator##rb")) rb->clearForceAccumulator();
            }
        }
    };

    /*!
    @brief Adapter for editing Component::ColliderComponent (Sphere / AABB).
    @details Lets users switch collider type and tweak radius/min/max; can snap to Transform.
    */
    struct ColliderAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "Collider"; }

        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<ColliderComponent>(e);
        }

        void add(ECS& ecs, Entity e) override {
            auto& D = GetDebugDefaults();
            // Default to a 1.0-radius sphere at the entity's transform position (if present)
            CEVec3 center{ 0.f,0.f,0.f };
            if (auto* t = ecs.GetComponent<TransformComponent>(e)) center = t->position;

            auto col = std::make_unique<Collision::Sphere>(D.sphere_radius, center);
            ecs.AddComponent<ColliderComponent>(e, std::move(col));

            if (!ecs.HasComponent<RigidBodyComponent>(e)) {
                auto& D2 = GetDebugDefaults();
                ecs.AddComponent<RigidBodyComponent>(e, CEVec3{ 0.f,0.f,0.f }, 0.0f, D2.rb_restitution);
            }
        }

        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<ColliderComponent>(e);
        }

        void drawInspector(ECS& ecs, Entity e) override {
            auto* cc = ecs.GetComponent<ColliderComponent>(e);
            auto& D = GetDebugDefaults();
            if (!cc || !cc->collider) return;

            if (!ecs.HasComponent<RigidBodyComponent>(e)) {
                if (ImGui::Button("Add Static RigidBody (mass=0)")) {
                    auto& D3 = GetDebugDefaults();
                    ecs.AddComponent<RigidBodyComponent>(e, CEVec3{ 0.f,0.f,0.f }, 0.0f, D3.rb_restitution);
                }
            }

            // Type selector
            int type = static_cast<int>(cc->collider->type);
            const char* items[] = { "Sphere","AABB" };
            ImGui::Combo("Type", &type, items, IM_ARRAYSIZE(items));

            // If type changed, rebuild collider (keep a sane default size)
            if (type != static_cast<int>(cc->collider->type)) {
                if (type == Collision::SPHERE) {
                    CEVec3 center{ 0.f,0.f,0.f };
                    if (auto* t = ecs.GetComponent<TransformComponent>(e)) center = t->position;
                    cc->collider = std::make_unique<Collision::Sphere>(D.sphere_radius, center);
                }
                else {
                    // AABB centered on transform with �0.5 extents
                    CEVec3 c{ 0.f,0.f,0.f };
                    if (auto* t = ecs.GetComponent<TransformComponent>(e)) c = t->position;
                    cc->collider = std::make_unique<Collision::AABBRect>(
                        CEVec3{ c.x - D.aabb_half, c.y - D.aabb_half, c.z - D.aabb_half },
                        CEVec3{ c.x + D.aabb_half, c.y + D.aabb_half, c.z + D.aabb_half }
                    );
                }
            }

            // Draw fields per type
            if (cc->collider->type == Collision::SPHERE) {
                // Sphere
                auto* s = static_cast<Collision::Sphere*>(cc->collider.get());

                ImGui::SeparatorText("Sphere");

                // Radius
                float r = s->radius;
                ImGui::DragFloat("Radius", &r, 0.05f, 0.0f, 1e6f);
                if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
                    if (r < 0.0f) r = 0.0f; // clamping
                    s->radius = r;
                }

                // Center
                float c[3]{ s->center.x, s->center.y, s->center.z };
                ImGui::DragFloat3("Center", c, 0.1f);
                if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
                    s->center = { c[0], c[1], c[2] };
                }
            }
            else {
                // AABB
                auto* a = static_cast<Collision::AABBRect*>(cc->collider.get());

                ImGui::SeparatorText("AABB");

                float mn[3]{ a->min.x, a->min.y, a->min.z };
                float mx[3]{ a->max.x, a->max.y, a->max.z };

                bool edited = false;
                ImGui::DragFloat3("Min", mn, 0.05f);  edited |= ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::DragFloat3("Max", mx, 0.05f);  edited |= ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit();

                if (edited) {
                    for (int i = 0; i < 3; ++i) if (mn[i] > mx[i]) std::swap(mn[i], mx[i]); // clamp
                    a->min = { mn[0], mn[1], mn[2] };
                    a->max = { mx[0], mx[1], mx[2] };
                    a->extents = (a->max - a->min) * 0.5f;
                }

                // optional: snap AABB around Transform
                if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
                    if (ImGui::Button("Fit around Transform##aabb")) {
                        if (ecs.HasComponent<Component::ShapeComponent>(e)) {
                            auto* shape{ ecs.GetComponent<Component::ShapeComponent>(e) };
                            CEVec3 extents{};
                            

                            switch (shape->type) {
                                case ShapeType::Rectangle: {
                                    extents.x = shape->width * 0.5f;
                                    extents.y = shape->height * 0.5f;

                                }
                                break;
                                case ShapeType::Square: {
                                    extents.x = shape->size * 0.5f;
                                    extents.y = shape->size * 0.5f;
                                }
                                break;
                                case ShapeType::Triangle: {
                                    CEVec3 diffP1P2{ shape->t1 - shape->t2 };
                                    CEVec3 diffP1P3{ shape->t1 - shape->t3 };
                                    CEVec3 diffP2P3{ shape->t2 - shape->t3 };


                                    extents.x = max(std::fabs(diffP1P2.x), std::fabsf(diffP1P3.x), std::fabsf(diffP2P3.x)) * 0.5f;
                                    extents.y = max(std::fabs(diffP1P2.y), std::fabsf(diffP1P3.y), std::fabsf(diffP2P3.y)) * 0.5f;
                                }
                                break;
                            }

                            a->min = t->position - extents;
                            a->max = t->position + extents;
                            a->extents = (a->max - a->min) * 0.5f;
                        }
                       

                        
                    }
                }
            }
        }
    };

    /*!
    @brief Adapter for editing Component::ShapeComponent (debug primitives).
    @details Edits type-specific parameters and supports a per-entity preview toggle.
    */
    struct ShapeAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "ShapeComponent"; }
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
            auto& D = GetDebugDefaults();
            if (!sc) return;

            // type
            int t = (int)sc->type;
            const char* items[] = { "Square","Rectangle","Line","Triangle" };
            if (ImGui::Combo("Type", &t, items, IM_ARRAYSIZE(items))) {
                sc->type = (ShapeType)t;
            }

            //draw_reflected(*sc, "Shape");
            
            // shared
            ImGui::Checkbox("Use TextureAsset", &sc->enableTexture);

            char buf[256]{};
            if (sc->textureName.size() < sizeof(buf)) std::memcpy(buf, sc->textureName.data(), sc->textureName.size());

            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 18.0f);
            ImGui::InputText("##tex_name", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

            ImGui::SameLine();
            ImGui::TextDisabled("TextureAsset");

            ImGui::SameLine();
            if (ImGui::SmallButton("Choose...")) {
                ImGui::OpenPopup("Pick Texture");
            }

            // Modal popup with a simple file browser
            if (ImGui::BeginPopupModal("Pick Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                namespace fs = std::filesystem;
                static std::vector<std::string> s_assets;
                static bool s_scanned = false;

                // Refresh List button
                if (ImGui::Button("Refresh List")) {
                    s_scanned = false;
                }
                ImGui::SameLine();

                if (!s_scanned) {
                    s_scanned = true;
                    s_assets.clear();

                    const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "images"; // base folder
                    for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
                        if (!p.is_regular_file()) continue;

                        const std::string ext = p.path().extension().string();
                        std::string lower = ext;
                        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                        if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
                            lower == ".tga" || lower == ".bmp") {

                            // Store relative path for cleaner display
                            std::string rel = fs::relative(p.path(), assetRoot).string();
                            s_assets.push_back(rel);
                        }
                    }

                    std::sort(s_assets.begin(), s_assets.end());
                }

                // Left: list; Right: preview name
                ImGui::BeginChild("asset_list", ImVec2(420, 300), true);
                for (auto const& f : s_assets) {
                    if (ImGui::Selectable(f.c_str())) {
                        sc->textureName = f;
                        std::strncpy(buf, f.c_str(), sizeof(buf) - 1);
                        buf[sizeof(buf) - 1] = '\0';
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                }
                ImGui::EndChild();

                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            // If using texture, enable the texture options
            if (sc->enableTexture) {

                const char* wrapModes[] = { "Clamp To Edge","Repeat","Mirror" };
                const int numWrapModes = IM_ARRAYSIZE(wrapModes);

                // U (HORIZONTAL) WRAP MODE
                if (ImGui::Combo("U Wrap Mode", &sc->wrapModeU, wrapModes, numWrapModes)) {
                }

                // V (HORIZONTAL) WRAP MODE
                if (ImGui::Combo("V Wrap Mode", &sc->wrapModeV, wrapModes, numWrapModes)) {
                }

                // UV size
                float size[2] = { sc->uvSize.x, sc->uvSize.y };
                ImGui::DragFloat2("UV", size, D.drag_small);
                if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
                    sc->uvSize = { size[0], size[1] };
                }

            }

            ImGui::DragFloat3("Color", &sc->color.x, D.drag_small, 0.f, 255.f);
            //ImGui::DragFloat("Rotation", &sc->rotation, D.drag_rotation);
            //ImGui::DragFloat("Scale", &sc->scale, D.drag_small, 0.01f, 100.f);

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

            /*CEVec3 pos{ 0,0,0 };
            if (auto* tf = ecs.GetComponent<Component::TransformComponent>(e)) pos = tf->position;
            switch (sc->type) {
            case Component::ShapeType::Square:    Component::Shapes::CreateSquare(pos, sc->size, sc->rotation, sc->scale, sc->color); break;
            case Component::ShapeType::Rectangle: Component::Shapes::CreateRectangle(pos, sc->width, sc->height, sc->rotation, sc->scale, sc->color); break;
            case Component::ShapeType::Line:      Component::Shapes::CreateLine(sc->p1, sc->p2, sc->thickness, sc->rotation, sc->scale, sc->color); break;
            case Component::ShapeType::Triangle:  Component::Shapes::CreateTriangle(sc->t1, sc->t2, sc->t3, sc->rotation, sc->scale, sc->color); break;
            }*/
            // reminder to ask how/where is the rendering of the shapes done in general, i don't wanna hunt for functions
        }
    };

    /*!
    @brief Adapter for editing Component::TextComponent.
    @details Edits type-specific parameters and supports a per-entity preview toggle.
    */
    struct TextAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "TextComponent"; }
        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<Component::TextComponent>(e);
        }
        void add(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) ecs.AddComponent<Component::TextComponent>(e);
        }
        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) ecs.RemoveComponent<Component::TextComponent>(e);
        }
        void drawInspector(ECS& ecs, Entity e) override {
            using namespace Component;
            auto* fc = ecs.GetComponent<TextComponent>(e);
            //auto& D = GetDebugDefaults();
            if (!fc) return;

            char buf[256]{};
            if (fc->font.size() < sizeof(buf)) std::memcpy(buf, fc->font.data(), fc->font.size());

            ImGui::InputText("Font Name", buf, sizeof(buf));
            if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
                fc->font = buf;
            }
			
            char buf2[512]{};
            if (fc->text.size() < sizeof(buf2)) std::memcpy(buf2, fc->text.data(), fc->text.size());
            
            ImGui::InputText("Display Text", buf2, sizeof(buf2));
            if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
                fc->text = buf2;
            }

            ImGui::Checkbox("Center Text", &fc->centered);

			ImGui::DragFloat("Scale", &fc->scale);

            ImGui::DragFloat2("offset", &fc->offset.x);

            ImGui::DragFloat3("Color", &fc->color.x, 1.f, 0.f, 255.f);
        }
    };

    /*!
    @brief Adapter for editing Component::AudioComponent.
    @details Edits type-specific parameters and supports a per-entity preview toggle.
    */
    struct AudioAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "Audio"; }
        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<Component::AudioEmitterComponent>(e);
        }
        void add(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) ecs.AddComponent<Component::AudioEmitterComponent>(e);
        }
        void remove(ECS& ecs, Entity e) override {
            if (has(ecs, e)) {
                if (auto* c = ecs.GetComponent<Component::AudioEmitterComponent>(e);
                    c && c->inst) {
                    c->inst->stop(FMOD_STUDIO_STOP_IMMEDIATE); c->inst->release(); c->inst = nullptr;
                }
                ecs.RemoveComponent<Component::AudioEmitterComponent>(e);
            }
        }
        void drawInspector(ECS& ecs, Entity e) override {
            using Component::AudioEmitterComponent;
            auto* c = ecs.GetComponent<AudioEmitterComponent>(e);
            if (!c) return;

            // Event path text
            ImGui::SeparatorText("Event");

            char buf[256]{};
            if (c->eventPath.size() < sizeof(buf)) std::memcpy(buf, c->eventPath.data(), c->eventPath.size());
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 18.0f);
            ImGui::InputTextWithHint("##event_path", "event:/…", buf, sizeof(buf));
            if (ImGui::IsItemEdited() || ImGui::IsItemDeactivatedAfterEdit()) {
                c->eventPath = buf;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Event");

            ImGui::SameLine();
            if (ImGui::SmallButton("Choose...")) {
                ImGui::OpenPopup("Pick Audio Event");
            }

            if (ImGui::BeginPopupModal("Pick Audio Event", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                namespace fs = std::filesystem;
                static std::vector<std::string> s_audioDisplay;  // shown to user
                static std::vector<std::string> s_audioEvent;    // actual event:/… we’ll write
                static bool s_scanned = false;

                // one-time (per session) scan
                if (!s_scanned) {
                    s_scanned = true;
                    s_audioDisplay.clear();
                    s_audioEvent.clear();

                    const fs::path assetRoot = "assets/audio";
                    auto to_event_like = [&](const fs::path& p) -> std::string {
                        // turn assets/audio/SFX/Beep.wav -> event:/SFX/Beep
                        fs::path rel = fs::relative(p, assetRoot);
                        fs::path withoutExt = rel;
                        if (withoutExt.has_extension()) withoutExt.replace_extension();
                        std::string e = withoutExt.generic_string();

                        // Capitalization/punctuation are preserved; prepend event:/ and normalize slashes.
                        return "event:/" + e;
                        };

                    try {
                        for (auto& entry : fs::recursive_directory_iterator(assetRoot)) {
                            if (!entry.is_regular_file()) continue;
                            std::string ext = entry.path().extension().string();
                            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                            // Include common authoring/artifacts; refine as needed for your pipeline
                            if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".fsb" || ext == ".bank") {
                                std::string disp = fs::relative(entry.path(), assetRoot).generic_string();
                                std::string ev = to_event_like(entry.path());

                                s_audioDisplay.push_back(disp);
                                s_audioEvent.push_back(ev);
                            }
                        }

                        // Sort by display name
                        std::vector<size_t> idx(s_audioDisplay.size());
                        std::iota(idx.begin(), idx.end(), 0);
                        std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
                            return s_audioDisplay[a] < s_audioDisplay[b];
                           });

                        std::vector<std::string> disp2, ev2;
                        disp2.reserve(idx.size()); ev2.reserve(idx.size());
                        for (size_t i : idx) { disp2.push_back(std::move(s_audioDisplay[i])); ev2.push_back(std::move(s_audioEvent[i])); }
                        s_audioDisplay.swap(disp2); s_audioEvent.swap(ev2);
                    }
                    catch (...) {
                        // swallow errors; empty list will be shown
                    }
                }

                // Left: list
                ImGui::BeginChild("audio_list", ImVec2(420, 300), true);
                for (size_t i = 0; i < s_audioDisplay.size(); ++i) {
                    if (ImGui::Selectable(s_audioDisplay[i].c_str())) {
                        c->eventPath = s_audioEvent[i];

                        // reflect selection back to the read-only text box
                        std::strncpy(buf, c->eventPath.c_str(), sizeof(buf) - 1);
                        buf[sizeof(buf) - 1] = '\0';

                        ImGui::CloseCurrentPopup();
                        break;
                    }
                }
                ImGui::EndChild();

                if (ImGui::Button("Rescan")) {
                    // allow rescanning if you add new audio during runtime
                    s_scanned = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

                ImGui::EndPopup();
            }

            ImGui::DragFloat("Volume", &c->volume, 0.01f, 0.f, 5.f);
            ImGui::DragFloat("Pitch", &c->pitch, 0.01f, 0.25f, 4.f);
            ImGui::Checkbox("3D Spatial", &c->spatial3D);

            // Create/Play/Stop lifecycle
            if (ImGui::Button(c->inst ? "Restart" : "Play")) {
                if (!c->eventPath.empty()) {
                    if (c->inst) { c->inst->stop(FMOD_STUDIO_STOP_IMMEDIATE); c->inst->release(); c->inst = nullptr; }
                    FMOD::Studio::EventInstance* inst = Audio::AudioSystem::instance().createInstance(c->eventPath.c_str());
                    if (inst) {
                        inst->setVolume(c->volume);
                        inst->setPitch(c->pitch);
                        // Optional: attach 3D to Transform
                        if (c->spatial3D) {
                            if (auto* t = ecs.GetComponent<Component::TransformComponent>(e)) {
                                FMOD_3D_ATTRIBUTES a{};
                                a.position = { t->position.x, t->position.y, t->position.z };
                                a.forward = { 0,0,1 }; a.up = { 0,1,0 }; a.velocity = { 0,0,0 };
                                inst->set3DAttributes(&a);
                            }
                        }
                        inst->start();
                        c->inst = inst;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop") && c->inst) {
                c->inst->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
            }

            // Live controls for a running instance
            if (c->inst) {
                c->inst->setVolume(c->volume);
                c->inst->setPitch(c->pitch);

                // Expose event parameters dynamically
                FMOD::Studio::EventDescription* desc = nullptr;
                c->inst->getDescription(&desc);
                if (desc) {
                    int count = 0; desc->getParameterDescriptionCount(&count);
                    for (int i = 0; i < count; ++i) {
                        FMOD_STUDIO_PARAMETER_DESCRIPTION pd{};
                        if (desc->getParameterDescriptionByIndex(i, &pd) == FMOD_OK) {
                            float v = 0.f;
                            c->inst->getParameterByID(pd.id, &v, nullptr);
                            if (ImGui::DragFloat(pd.name, &v, 0.01f, pd.minimum, pd.maximum, "%.3f")) {
                                c->inst->setParameterByID(pd.id, v);
                            }
                        }
                    }
                }
            }
        }
    };

    /*!
    @brief Adapter for editing Component::ScriptComponent.
    @details Edits type-specific parameters and supports a per-entity preview toggle.
    */
    struct ScriptAdapter : DebugReflection::IComponentAdapter {
        const char* name() const override { return "ScriptComponent"; }

        bool has(ECS& ecs, Entity e) const override {
            return ecs.HasComponent<CrashoutsEngine::ScriptComponent>(e);
        }

        void add(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) {
                ecs.AddComponent<CrashoutsEngine::ScriptComponent>(e, "PlayerScript");
            }
        }

        void remove(ECS& ecs, Entity e) override {
            if (!has(ecs, e)) {
                ecs.RemoveComponent<CrashoutsEngine::ScriptComponent>(e);
            }
        }

        void drawInspector(ECS& ecs, Entity e) override {
            auto* sc = ecs.GetComponent<ScriptComponent>(e);
            if (!sc) return;

            char buf[128]{};
            std::snprintf(buf, sizeof(buf), "%s", sc->ClassName.c_str());
            if (ImGui::InputText("Class Name", buf, sizeof(buf)))
                sc->ClassName = buf;

            bool monoReady = (MonoManager::GetDomain() != nullptr && MonoManager::GetAssembly() != nullptr);
            ImGui::TextDisabled("Mono: %s", monoReady ? "Ready" : "Not Ready");

            if (!monoReady) ImGui::BeginDisabled();

            if (ImGui::Button(sc->Instance ? "Reinitialize" : "Initialize")) {
                sc->Destroy();
                sc->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
            }

            ImGui::SameLine();
            if (ImGui::Button("Start")) sc->Start();

            ImGui::SameLine();
            static float dt = 1.0f / 60.0f; ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("dt", &dt, 0.001f, 0, 1, "%.3f");

            ImGui::SameLine();
            if (ImGui::Button("Update")) sc->Update(dt)
                ;
            ImGui::SameLine();
            if (ImGui::Button("Destroy")) sc->Destroy();

            if (!monoReady) ImGui::EndDisabled();
        }
    };

    /*!
    @brief Game-state provider installed at runtime (no hardcoded state list).
    @details Bridges GameStateManager to the Debug Menu; exposes current/previous/next and issues ChangeGameState() transitions from a reflected state list.
    */
    struct GameStateProvider final : DebugReflection::IGameStateProvider {
        int  current()  const override { return (int)CurrentGameState; }
        int  previous() const override { return (int)PreviousGameState; }
        int  next()     const override { return (int)NextGameState; }
        void changeTo(int id) override { ChangeGameState((GameState)id); }

        const std::vector<std::pair<const char*, int>>& states() const override {
            static std::vector<std::pair<const char*, int>> cache;
            cache.clear();
            for (auto const& [st, desc] : GetStateRegistry()) {
                const char* label = desc.Label ? desc.Label : "(unnamed)";
                cache.emplace_back(label, (int)st);
            }
            return cache;
        }
    };

    /*!
    @brief Physics debug provider (global forces / integrators).
    @details Exposes gravity vector and linear damping via PhysicsHandler getters/setters.
    */
    struct BuiltinPhysicsProvider final : DebugReflection::IPhysicsProvider {
        const char* name() const override { return "Global Forces"; }
        void draw(CrashoutsEngine::ECS&) override {
            // Gravity
            CEVec3 g = Physics::PhysicsHandler::GetGravity();
            float gv[3]{ g.x,g.y,g.z };
            if (ImGui::DragFloat3("Gravity (m/s^2)", gv, 0.1f)) {
                Physics::PhysicsHandler::SetGravity({ gv[0],gv[1],gv[2] });
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset g##phys")) {
                Physics::PhysicsHandler::SetGravity({ 0.f,-2.f,0.f });
            }

            // Linear damping
            float d = Physics::PhysicsHandler::GetLinearDamping();
            if (ImGui::DragFloat("Linear damping", &d, 0.05f, 0.f, 100.f)) {
                Physics::PhysicsHandler::SetLinearDamping(d);
            }
        }
    };

    //----------------------------------------------------------------------------- registration singletons

    /*!
    @brief File-local instances of adapters and debug providers registered with reflection.
    @details These live for the lifetime of the process. The registries store raw pointers to
             these singletons; keep them in an anonymous namespace to avoid ODR/linkage issues.
             Adapters drive the ECS inspector; providers extend the Physics/Game State panels.
    */
    static NameAdapter gNameAdapter;
    static TransformAdapter gTransformAdapter;
    static RigidBodyAdapter gRigidBodyAdapter;
    static ColliderAdapter gColliderAdapter;
    static ShapeAdapter gShapeAdapter;
    static TextAdapter gTextAdapter;
    static AudioAdapter gAudioAdapter;
    static ScriptAdapter gScriptAdapter;
    static GameStateProvider gProvider;
    static BuiltinPhysicsProvider gBuiltinPhys;

}

/*!
 @brief Registers all built-in component adapters and debug providers.
 @details Call once during startup (after ECS and GameStateManager are initialized).
          - Registers ECS component adapters with DebugReflection::ComponentRegistry
          - Installs the Game State provider used by the Game State panel
          - Registers physics providers (e.g., gravity/damping) with PhysicsRegistry
 @note    Safe to call exactly once; repeated calls will duplicate entries.
 @see     DebugReflection::ComponentRegistry
 @see     DebugReflection::PhysicsRegistry
 @see     DebugReflection::SetGameStateProvider
*/
extern "C" void RegisterBuiltinComponentAdapters() {
    using namespace DebugReflection;
    ComponentRegistry::instance().registerAdapter(&gNameAdapter);
    ComponentRegistry::instance().registerAdapter(&gTransformAdapter);
    ComponentRegistry::instance().registerAdapter(&gRigidBodyAdapter);
    ComponentRegistry::instance().registerAdapter(&gColliderAdapter);
    ComponentRegistry::instance().registerAdapter(&gShapeAdapter);
    ComponentRegistry::instance().registerAdapter(&gTextAdapter);
    ComponentRegistry::instance().registerAdapter(&gAudioAdapter);
    ComponentRegistry::instance().registerAdapter(&gScriptAdapter);
    DebugReflection::SetGameStateProvider(&gProvider);
    DebugReflection::PhysicsRegistry::instance().registerProvider(&gBuiltinPhys);

}
