/*!
@file       debugmenu.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

This file provides the graphics for Crashout Engine's ImGUI.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Engine / app headers
#include "glapp.h"
#include "performanceViewer.h"
#include "managers/entitymanager.h"
#include "managers/collisionmanager.h"
#include "managers/physicsmanager.h"
#include "gamestatemanager.h"
#include "shapes.h"
#include "log.h"
#include "debugmenu.h"
#include "debugreflection.h"


// ImGui headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <string>
#include <cstring>
#include "math/CEVec3.h"

#include "frameratecontroller.h"

/*                                                                   
----------------------------------------------------------------------------- */

using namespace CrashoutsEngine;
using namespace Component;

namespace { //----------------------------------------------------------------------------- file-local state & helpers

    /*!
     @brief File-local helpers and state for ImGui panels.
    */
    ECS* gECS{ nullptr };                       //!< Local ECS sandbox for demo purposes
    std::vector<Entity> gEntities;              //!< Sandbox entities
    Entity gSelectedEntity = 0;                 //!< Currently selected entity
    std::vector<ProfilerSample> gPerfSnapshot;  //!< Performance profiler snapshot
    double gPerfSnapshotTotalMs = 0.0;          //!< Total frame time in ms

    // Small UI helpers

    /*!
     @brief RAII wrapper to automatically open and close ImGui windows.
    */
    struct ScopedWindow {
        ScopedWindow(const char* title, ImGuiWindowFlags flags = 0) { ImGui::Begin(title, nullptr, flags); }
        ~ScopedWindow() { ImGui::End(); }
    };

    /*!
     @brief Locks or unlocks ImGui window movement and resizing.
     @param locked Whether to lock the window.
     @return ImGuiWindowFlags
    */
    inline ImGuiWindowFlags LockFlags(bool locked) {
        return locked ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)
            : ImGuiWindowFlags_None;
    }

    /*!
     @brief Displays a small tooltip helper next to UI elements.
     @param text Tooltip text.
    */
    inline void Help(const char* text) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(400.f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

} // anonymous namespace

namespace UI { // ----------------------------------------------------------------------------- public UI API

    // Forward so panels can call it
    static void DrawCollisionOverlayInternal(ECS& ecs, const std::vector<Collision::CollisionEvent>& events);

    // -- Lifecycle ------------------------------------------------------------
    /*!
     @brief Initializes ImGui context, allocators, and platform bindings.
     @param window GLFW window handle.
    */
    void Init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();

        // Use your existing allocator tags (kept intact)
        ImGui::SetAllocatorFunctions(
            [](size_t sz, void*) {
                ScopedMemTag tag(MemTag::UI);
                return ::operator new(sz);
            },
            [](void* p, void*) {
                ScopedMemTag tag(MemTag::UI);
                ::operator delete(p);
            },
            nullptr);

        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();

        ScopedMemTag boot(MemTag::Resources);
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        LOGS(Core, "Engine initialized successfully");
        LOGW(Core, "Debug mode is ON");
        LOGI(Core, "Welcome to Crashout Engine Debugger!");
    }

    /*!
     @brief Shuts down ImGui and cleans up resources.
    */
    void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    // -- Frame ownership (exactly one pair per frame) -------------------------
    /*!
     @brief Begins a new ImGui frame.
    */
    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    /*!
     @brief Ends the current ImGui frame and renders all draw data.
    */
    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
    }

    // -- Panels ---------------------------------------------------------------
    /*!
     @brief Renders the ECS debug panel, allowing creation, deletion, and inspection of entities.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param entities List of ECS entities.
     @param selected Currently selected entity.
    */
    void RenderEcsPanel(FrameProfiler& profiler, ECS& ecs, std::vector<Entity>& entities, Entity& selected, UIState& ui)
    {

        FrameProfiler::Scoped s(profiler, "ImGui");
        ScopedMemTag mem(MemTag::UI);

        ScopedWindow win("ECS Demo", LockFlags(ui.lockLayout));

        // Create entity in the REAL ECS
        if (ImGui::Button("Create Entity")) {
            Entity e = ecs.CreateEntity();
            entities.push_back(e);
            if (!ecs.HasComponent<NameComponent>(e)) {
                ecs.AddComponent<NameComponent>(e, "Entity " + std::to_string(e));
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Entities");
        if (entities.empty()) {
            ImGui::TextDisabled("No entities created yet.");
        }
        else {
            for (Entity e : entities) {
                // build visible name
                std::string visible = ecs.HasComponent<NameComponent>(e) ? ecs.GetComponent<NameComponent>(e)->name : ("Entity " + std::to_string(e));

                // make the ID unique via "##"
                std::string id = visible + "##ent" + std::to_string((unsigned)e);

                bool isSel = (e == selected);
                if (ImGui::Selectable(id.c_str(), isSel)) {
                    selected = e;
                }
            }
        }

        ImGui::Separator();
        ImGui::Text("Selected Entity: %u", (unsigned)selected);

        if (!selected) return;

        if (ImGui::Button("Destroy Selected")) {
            ecs.DestroyEntity(selected);
            if (auto it = std::find(entities.begin(), entities.end(), selected);
                it != entities.end()) entities.erase(it);
            selected = 0;
            return;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Add Components");

        // List all registered component types with an "Add" button
        for (auto* ad : DebugReflection::ComponentRegistry::instance().all()) {
            ImGui::PushID(ad);
            bool has = ad->has(ecs, selected);
            ImGui::BeginDisabled(has);
            if (ImGui::Button(ad->name())) {
                ad->add(ecs, selected);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextUnformatted(ad->name());

            ImGui::PopID();
        }
        ImGui::NewLine();

        // --- Inspect components present on selected entity ---
        ImGui::Separator();
        ImGui::TextUnformatted("Components on Selected Entity");

        for (auto* ad : DebugReflection::ComponentRegistry::instance().all()) {
            if (!ad->has(ecs, selected)) continue;

            ImGui::PushID(ad); // unique scope for this component’s UI

            // TreeNode label can be the display name now that ID is scoped
            if (ImGui::TreeNode("Component")) {   // id = "Component" under PushID(ad)
                ad->drawInspector(ecs, selected);

                if (ImGui::Button("Remove##btn")) {  // id = "Remove##btn" under PushID(ad)
                    ad->remove(ecs, selected);
                    ImGui::TreePop();
                    ImGui::PopID();
                    break; // components changed; bail this frame
                }
                ImGui::TreePop();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", ad->name()); // visible label next to the node

            ImGui::PopID();
        }
    }

    /*!
     @brief Renders the performance panel with profiler results.
     @param profiler Frame profiler reference.
     @param ui UI state toggles.
    */
    void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui) {
        if (!ui.showPerf) return;

        ScopedWindow w("Performance", LockFlags(ui.lockLayout));

        if (profiler.present_ready()) {
            gPerfSnapshot = profiler.results();
            gPerfSnapshotTotalMs = profiler.total_ms();
        }
        ImGui::Text("Frame: %.3f ms (%.1f FPS)", gPerfSnapshotTotalMs, FrameRateController::GetInstance().FPS());

        ImGui::Separator();
        DrawEngineMemoryPanel();
        ImGui::Separator();
        DrawProfilerPanel(profiler);
        ImGui::Separator();

        if (ImGui::BeginTable("perf_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("System");
            ImGui::TableSetupColumn("ms");
            ImGui::TableSetupColumn("% of frame");
            ImGui::TableHeadersRow();

            for (const auto& s : gPerfSnapshot) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(s.name.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", s.ms);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.1f%%", s.pct);
                ImGui::ProgressBar(float(s.pct / 100.0), ImVec2(-1, 0), "");
            }
            ImGui::EndTable();
        }
    }

    /*!
     @brief Renders the console log viewer panel.
     @param ui UI state toggles.
    */
    void RenderConsolePanel(UIState& ui) {
        if (!ui.showConsole) return;

        ScopedWindow w("Console", LockFlags(ui.lockLayout));

        static bool showInfo = true, showWarn = true, showErr = true, showSucc = true;
        static bool sysCore = true, sysPhys = true, sysColl = true, sysInput = true, sysAI = true, sysECS = true, sysRend = true, sysUI = true;

        if (ImGui::Checkbox("Info", &showInfo)) {}
        ImGui::SameLine(); ImGui::Checkbox("Warn", &showWarn);
        ImGui::SameLine(); ImGui::Checkbox("Error", &showErr);
        ImGui::SameLine(); ImGui::Checkbox("Success", &showSucc);

        ImGui::Separator();
        ImGui::TextUnformatted("Subsystems:");
        ImGui::Checkbox("Core", &sysCore); ImGui::SameLine();
        ImGui::Checkbox("Physics", &sysPhys); ImGui::SameLine();
        ImGui::Checkbox("Collision", &sysColl); ImGui::SameLine();
        ImGui::Checkbox("Input", &sysInput);
        ImGui::Checkbox("AI", &sysAI); ImGui::SameLine();
        ImGui::Checkbox("ECS", &sysECS); ImGui::SameLine();
        ImGui::Checkbox("Renderer", &sysRend); ImGui::SameLine();
        ImGui::Checkbox("UI", &sysUI);

        ImGui::Separator();

        const auto& logs = Logger::Instance().Get();
        for (const auto& log : logs) {
            // Level filter
            if ((log.level == LogMessage::Level::Info && !showInfo) ||
                (log.level == LogMessage::Level::Warning && !showWarn) ||
                (log.level == LogMessage::Level::Error && !showErr) ||
                (log.level == LogMessage::Level::Success && !showSucc)) continue;

            // Subsystem filter
            bool sysPass =
                (log.system == LogMessage::Subsystem::Core && sysCore) ||
                (log.system == LogMessage::Subsystem::Physics && sysPhys) ||
                (log.system == LogMessage::Subsystem::Collision && sysColl) ||
                (log.system == LogMessage::Subsystem::Input && sysInput) ||
                (log.system == LogMessage::Subsystem::AI && sysAI) ||
                (log.system == LogMessage::Subsystem::ECS && sysECS) ||
                (log.system == LogMessage::Subsystem::Renderer && sysRend) ||
                (log.system == LogMessage::Subsystem::UI && sysUI);
            if (!sysPass) continue;

            ImVec4 color =
                (log.level == LogMessage::Level::Error) ? ImVec4(1, 0.3f, 0.3f, 1) :
                (log.level == LogMessage::Level::Warning) ? ImVec4(1, 0.9f, 0.4f, 1) :
                (log.level == LogMessage::Level::Success) ? ImVec4(0.4f, 1, 0.4f, 1) :
                ImVec4(0.4f, 0.8f, 1, 1);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.text.c_str());
            ImGui::PopStyleColor();
        }

        if (ImGui::Button("Clear")) {
            // Non-intrusive: only clear if Logger exposes a non-const view.
            const_cast<std::vector<LogMessage>&>(logs).clear();
        }
    }

    /*!
     @brief Renders the gameState panel
     @param state gameState reference.
     @param ui UI state toggles.
    */
    void RenderGameStatePanel(UIState& ui) {
        if (!ui.showGameState) return;
        ScopedWindow w("GameState", LockFlags(ui.lockLayout));

        ImGui::Text("Current: %d", (int)CurrentGameState);
        ImGui::Text("Previous: %d", (int)PreviousGameState);
        ImGui::Text("Next: %d", (int)NextGameState);

        //Helper function to make a button for each state
        auto stateButton = [](const char* label, GameState gs) {
            bool pressed = ImGui::Button(label);
            if (pressed) {
                ChangeGameState(gs);
            }
            ImGui::SameLine();
            return pressed;
        };

        // Row 1
        stateButton("Main Menu", MAIN_MENU);
        stateButton("Renfred", RENFRED);
        stateButton("Rain", RAIN);

        ImGui::NewLine();

        // Row 2
        stateButton("Amir", AMIR);
        stateButton("Afiq", AFIQ);
        stateButton("Tristan", TRISTAN);
        stateButton("Ranvitha", RAN);

        ImGui::Separator();

        // Utility Action
        if (ImGui::Button("Restart Current State")) {
            RestartGameState();
        }
        ImGui::SameLine();
        if (ImGui::Button("Quit")) {
            glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
        }
    }

    /*!
     @brief Renders the physics panel, including collision event listings and controls.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param phys Physics handler reference.
     @param ui UI state toggles.
    */
    void RenderPhysicsPanel(FrameProfiler& profiler, ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui)
    {
        if (!ui.showPhysics) return;
        ScopedWindow w("Physics / Collision", LockFlags(ui.lockLayout));

        bool paused = Physics::PhysicsHandler::GetPaused();
        if (ImGui::Checkbox("Paused", &paused)) {
            Physics::PhysicsHandler::SetPaused(paused);
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            Physics::PhysicsHandler::StepOnce(); Help("Advance one simulation tick when paused.");
        }

        ImGui::Checkbox("Freeze on collision", &ui.freezeOnCollision);
        ImGui::SameLine(); Help("Automatically pause when any collision is detected this frame.");
       
        static bool overrideDt = false;
        static double dbgDt = 1.0 / 30.0;
        ImGui::Checkbox("Override dt", &overrideDt); ImGui::SameLine();
        if (overrideDt && ImGui::DragScalar("dt", ImGuiDataType_Double, &dbgDt, 0.001f, nullptr, nullptr, "%.4f")) {
            Physics::PhysicsHandler::SetOverrideDt(true);
            Physics::PhysicsHandler::SetDebugDt(dbgDt);
        }
        else if (!overrideDt) {
            Physics::PhysicsHandler::SetOverrideDt(false);
        }

        ImGui::Separator();

        const auto& evs = phys.collisions();

        // auto pause if requested and collision occured
        if (ui.freezeOnCollision && !paused && !evs.empty()) {
            Physics::PhysicsHandler::SetPaused(true);
            paused = true;
        }

        ImGui::Separator();

        // Collisions table
        // Will be implemented later
        /*
        if (ImGui::BeginTable("coll_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Entity A");
            ImGui::TableSetupColumn("Entity B");
            ImGui::TableSetupColumn("Depth");
            ImGui::TableSetupColumn("Normal");
            ImGui::TableHeadersRow();
            auto getEntityLabel = [&](CrashoutsEngine::Entity e) -> const char* {
                if (ecs.HasComponent<NameComponent>(e)) {
                    return ecs.GetComponent<NameComponent>(e)->name.c_str();   // const char*
                }
                else {
                    static thread_local char buf[32];
                    std::snprintf(buf, sizeof(buf), "Entity %u", (unsigned)e);
                    return buf;                                                // decays to const char*
                }
            };

            for (auto const& ev : evs) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(getEntityLabel(ev.entity1));
                ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(getEntityLabel(ev.entity2));
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", ev.colManifold.depth);
                ImGui::TableSetColumnIndex(3); ImGui::Text("(%.2f, %.2f)", ev.colManifold.normal.x, ev.colManifold.normal.y);
            }
            ImGui::EndTable();
        }
        */

        ImGui::Checkbox("Draw overlay", &ui.drawCollOverlay);
        ImGui::SameLine(); ImGui::Checkbox("All colliders", &ui.drawAllColliders);

        // When a way to implement the velocity line happens
        // ImGui::SameLine(); ImGui::Checkbox("Velocities", &ui.drawVelocities);

        // When a way to figure out how to make it a line instead of a whole shape
        //ImGui::DragFloat("Line thickness", &ui.overlayThickness, 0.1f, 1.0f, 8.0f, "%.1f");

        ImGui::TextDisabled("Pairs this frame: %d", (int)evs.size());

        // Overlay drawn after windows so it appears on top
        if (ui.drawCollOverlay) {
            DrawCollisionOverlayInternal(ecs, evs);
        }

        if (ui.drawAllColliders) {
            DrawColliderWireframesOutlineGL(ecs, evs, ui.overlayThickness, true);
        }
    }

    /*!
     @brief Renders the audio panel.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param ui UI state toggles.
    */
    void RenderAudioPanel(FrameProfiler& profiler, ECS& ecs, UIState& ui) {
        if (!ui.showAudio) return;
        ScopedWindow w("Audio", LockFlags(ui.lockLayout));

        // Testing Purposes
        int vol = 10;
        ImGui::SliderInt("Audio", &vol, 0, 100);
    }

    static void RenderTogglesWindow(UIState& ui)
    {
        ImGui::SetNextWindowSize(ImVec2(260.f, 300.f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Debug Controls", nullptr, LockFlags(ui.lockLayout))) {
            ImGui::TextDisabled("Visibility");
            ImGui::Separator();

            // Default / legacy toggles (kept so old projects still work)
            ImGui::Checkbox("ECS", &ui.showECS);
            ImGui::Checkbox("Physics", &ui.showPhysics);
            ImGui::Checkbox("Performance", &ui.showPerf);
            ImGui::Checkbox("Console", &ui.showConsole);
            ImGui::Checkbox("Game State", &ui.showGameState);

            // Dynamically registered panels
            ImGui::Spacing();
            ImGui::TextDisabled("Panels");
            ImGui::Separator();
            for (auto const& p : DebugReflection::PanelRegistry::instance().panels()) {
                if (p.toggleFlag) {
                    ImGui::Checkbox(p.name.c_str(), p.toggleFlag);
                }
                else {
                    ImGui::TextDisabled("%s (always on-demand)", p.name.c_str());
                }
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Settings");
            ImGui::Separator();
            ImGui::Checkbox("Lock Layout", &ui.lockLayout);
            ImGui::Checkbox("Draw Colliders", &ui.drawCollOverlay);

            ImGui::Spacing();
            if (ImGui::Button("Show All")) {
                ui.showECS = ui.showPhysics = ui.showPerf = ui.showConsole = ui.showGameState = true;
                // Also turn on any registered, toggle-backed panels:
                for (auto const& p : DebugReflection::PanelRegistry::instance().panels())
                    if (p.toggleFlag) *p.toggleFlag = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Hide All")) {
                ui.showECS = ui.showPhysics = ui.showPerf = ui.showConsole = ui.showGameState = false;
                for (auto const& p : DebugReflection::PanelRegistry::instance().panels())
                    if (p.toggleFlag) *p.toggleFlag = false;
            }

            ImGui::Spacing();
            if (ImGui::Button("Save Layout")) ImGui::SaveIniSettingsToDisk(nullptr);
            ImGui::SameLine();
            if (ImGui::Button("Reload Layout")) ImGui::LoadIniSettingsFromDisk(nullptr);
        }
        ImGui::End();
    }

    // -- Overlays -------------------------------------------------------------
    /*!
     @brief Draws collision overlays.
     @param ecs ECS reference.
     @param events List of collision events.
    */
    static void DrawCollisionOverlayInternal(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>& events)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        // TODO: replace with real world->screen transform
        auto W2S = [](const ImVec2& p) -> ImVec2 { return p; };

        auto drawCollider = [&](CrashoutsEngine::Entity e, ImU32 color) {
            if (auto* t = ecs.GetComponent<TransformComponent>(e)) {
                if (auto* cc = ecs.GetComponent<Component::ColliderComponent>(e)) {
                    if (cc->collider) {
                        const Collision::Collider* base = cc->collider.get();
                        if (base->type == Collision::SPHERE) {
                            if (auto const* s = dynamic_cast<const Collision::Sphere*>(base)) {
                                ImVec2 c = W2S(ImVec2(t->position.x + s->center.x, t->position.y + s->center.y));
                                dl->AddCircle(c, s->radius, IM_COL32(255, 80, 80, 220), 32, 2.0f);
                            }
                        }
                        else if (base->type == Collision::AABB) {
                            if (auto const* a = dynamic_cast<const Collision::AABBRect*>(base)) {
                                ImVec2 pmin = W2S(ImVec2(t->position.x + a->min.x, t->position.y + a->min.y));
                                ImVec2 pmax = W2S(ImVec2(t->position.x + a->max.x, t->position.y + a->max.y));
                                dl->AddRect(pmin, pmax, IM_COL32(255, 80, 80, 220), 0.0f, 0, 2.0f);
                            }
                        }
                    }
                }
            }
            };

        // Draw only colliding pairs + contacts
        for (const auto& ev : events) {
            drawCollider(ev.entity1, IM_COL32(255, 80, 80, 220));
            drawCollider(ev.entity2, IM_COL32(255, 80, 80, 220));

            // Contact points
            for (const auto& cp : ev.colManifold.contactPoints) {
                ImVec2 p = W2S(ImVec2(cp.x, cp.y));
                dl->AddCircleFilled(p, 4.0f, IM_COL32(255, 200, 0, 255));
            }
            // Normal (if any points exist)
            if (!ev.colManifold.contactPoints.empty()) {
                const auto& cp0 = ev.colManifold.contactPoints[0];
                ImVec2 p0 = W2S(ImVec2(cp0.x, cp0.y));
                ImVec2 p1 = W2S(ImVec2(cp0.x + ev.colManifold.normal.x * 20.0f,
                    cp0.y + ev.colManifold.normal.y * 20.0f));
                dl->AddLine(p0, p1, IM_COL32(255, 100, 0, 255), 2.0f);
            }
        }
    }

    /*!
     @brief Draws collider shapes and optional contact normals in the scene.
     @param ecs ECS reference used to query transforms, colliders, and shapes.
     @param events List of collision events from the current frame.
     @param lineThickness Line thickness for normal/marker rendering.
     @param drawNormals Whether to render collision contact normals and points.
    */
    static void DrawColliderWireframesOutlineGL(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>& events, float lineThickness, bool drawNormals)
    {
        auto& collArr = ecs.GetComponentArray<Component::ColliderComponent>();

        auto isColliding = [&](CrashoutsEngine::Entity e)->bool {
            for (const auto& ev : events)
            {
                if (ev.entity1 == e || ev.entity2 == e) { 
                    return true;
                }
            }
            return false;
            };

        // --- 1) Wireframes for all colliders (but actually not because currently they are solid shapes) ---
        for (auto e : collArr.indexToEntity) {
            auto* t = ecs.GetComponent<Component::TransformComponent>(e);
            auto* cc = ecs.GetComponent<Component::ColliderComponent>(e);
            if (!t || !cc || !cc->collider) continue;

            const CEVec3 ok = CEVec3(0.2f, 1.0f, 0.6f);
            const CEVec3 hit = CEVec3(1.0f, 0.3f, 0.3f);
            const CEVec3 color = isColliding(e) ? hit : ok;

            bool drawn = false;

            // Draw via existing shape components - honestly sphere gonna be gaslit to be a square here ngl
            if (ecs.HasComponent<Component::Shapes>(e)) {
                Component::Shapes::CreateRectangle(t->position, t->size.x, t->size.y, t->rotation, 1.0f, color);
                drawn = true;
            }

            if (drawn) continue;

            // if no shape component, take directly from shape.cpp
            if (cc->collider->type == Collision::AABB) {
                auto* a = dynamic_cast<Collision::AABBRect*>(cc->collider.get());
                if (!a) continue;

                const float hx = std::max(std::fabs(a->min.x), std::fabs(a->max.x));
                const float hy = std::max(std::fabs(a->min.y), std::fabs(a->max.y));

                Component::Shapes::CreateRectangle(CEVec3(t->position.x, t->position.y, 0.0f), 2.0f * hx, 2.0f * hy, 0.0f, 1.0f, color);
            }

            // Currently using createrect for this
            else if (cc->collider->type == Collision::SPHERE) {
                auto* s = dynamic_cast<Collision::AABBRect*>(cc->collider.get());
                if (!s) continue;

                const float hx = std::max(std::fabs(s->min.x), std::fabs(s->max.x));
                const float hy = std::max(std::fabs(s->min.y), std::fabs(s->max.y));
                Component::Shapes::CreateRectangle(CEVec3(t->position.x, t->position.y, 0.0f), 2.0f * hx, 2.0f * hy, 0.0f, 1.0f, color);
            }
        }

        // --- 2) Contact normals ---
        if (drawNormals) {
            for (const auto& ev : events) {
                for (const auto& cp : ev.colManifold.contactPoints) {
                    CEVec3 p(cp.x, cp.y, 0);
                    CEVec3 n(ev.colManifold.normal.x, ev.colManifold.normal.y, 0);

                    Component::Shapes::CreateLine(
                        p, p + n * ev.colManifold.depth, lineThickness, 0.0f, 1.0f, CEVec3(1, 1, 0));

                    float d = 0.02f;
                    Component::Shapes::CreateLine(CEVec3(p.x - d, p.y, 0), CEVec3(p.x + d, p.y, 0), lineThickness, 0, 1, CEVec3(1, 1, 0));
                    Component::Shapes::CreateLine(CEVec3(p.x, p.y - d, 0), CEVec3(p.x, p.y + d, 0), lineThickness, 0, 1, CEVec3(1, 1, 0));
                }
            }
        }
    }

    /*!
     @brief Draws collision overlays directly on the viewport.
     @param ecs ECS reference.
     @param events List of collision events.
     @param ui UI state toggles.
    */
    void DrawCollisionOverlay(ECS& ecs, const std::vector<Collision::CollisionEvent>& events, const UIState& ui)
    {
        if (!ui.drawCollOverlay) return;
        DrawCollisionOverlayInternal(ecs, events);
    }

    // -- Coordinator(s) --------------------------------------------------------

    /*!
     @brief Renders all panels and overlays in a single call.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param phys Physics handler reference.
     @param ui UI state toggles.
     @param entities List of ECS entities.
     @param selected Currently selected entity.
    */
    void RenderAll(FrameProfiler& profiler, Physics::PhysicsHandler& phys, UIState& ui, ECS& ecs, std::vector<Entity>& entities, Entity& selected)
    {

        BeginFrame();
        RenderTogglesWindow(ui);
        if (ui.showECS) {
            RenderEcsPanel(profiler, ecs, entities, selected, ui); 
        }
        if (ui.showPhysics) {
            RenderPhysicsPanel(profiler, ecs, phys, ui);
        }
        if (ui.showAudio) {
            RenderAudioPanel(profiler, ecs, ui);
        }
        if (ui.showPerf) {
            RenderPerformancePanel(profiler, ui);
        }
        if (ui.showConsole) {
            RenderConsolePanel(ui);
        }
        if (ui.showGameState) {
            RenderGameStatePanel(ui);
        }
        EndFrame();
    }

    /*!
     @brief Backward-compatible wrapper for rendering without explicit UIState.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param phys Physics handler reference.
     @param entities List of ECS entities.
     @param selected Currently selected entity.
    */
    void Render(FrameProfiler& profiler, Physics::PhysicsHandler& phys, Entity& selected)
    {
        ECS* ecs = ECS::getInstance();

        std::vector<CrashoutsEngine::Entity> entities =
            ecs->GetComponentArray<Component::NameComponent>().indexToEntity;
        //std::cout << entities.size() << " ";

        static UIState ui;
        RenderAll(profiler, phys, ui, *ecs, entities, selected);
    }

} // namespace UI