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
#include "entitymanager.h"
#include "collisionmanager.h"
#include "physicsmanager.h"
#include "gamestatemanager.h"
#include "shapes.h"
#include "log.h"
#include "logdefs.h"
#include "debugmenu.h"
#include "debugreflection.h"
#include "debugcollision.h"
#include "frameratecontroller.h"
#include "audioSystem.h"
#include <fmod_studio.hpp> 
#include "prefab.h"
using CrashoutsEngine::Audio::AudioSystem;
// ImGui headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <string>
#include <cstring>
#include "CEVec3.h"

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
    // --- File-local audio state used by the panel ---

    static bool g_audioReady = false;
    static bool g_banksLoaded = false;

    // SFX
    static const char* kSfx_Beep = "event:/sfexbeep"; // must exist in sfx.bank
    // Music A (fixed path)
    static const char* kEvent_MusicA = "event:/Steven Beddall - Hold My ";
    static FMOD::Studio::EventInstance* s_musicA = nullptr;
    static float s_musicAVol = 1.0f;

    // Music B (editable path)
    static FMOD::Studio::EventInstance* s_musicB = nullptr;
    static float s_musicBVol = 1.0f;
    static char  g_musicBPath[128] = "event:/Music/Alt"; // type another event path if needed

    // Ensure audio is initialized and both banks are loaded
    static bool ensureAudio() {
        if (!g_audioReady) {
            // keep your current API; do NOT change signature
            g_audioReady = AudioSystem::instance().init();
        }
        if (g_audioReady && !g_banksLoaded) {
            bool ok = true;
            ok = ok && AudioSystem::instance().loadBank("sfx.bank");
            ok = ok && AudioSystem::instance().loadBank("Music.bank");
            g_banksLoaded = ok;
        }
        return g_audioReady && g_banksLoaded;
    }



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
        static std::vector<Entity> pinned;      // keep these inspectors visible
        static bool showAllInspectors = false;  // renders inspectors for every entity
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
            if (!ecs.HasComponent<TransformComponent>(e)) {
                ecs.AddComponent<TransformComponent>(e);
            }
        }
        ImGui::SeparatorText("Prefab Creator");
        static char nameBuf[128] = { 0 };
        ImGui::InputTextWithHint("Prefab name", "e.g. Box, Platform, Player...", nameBuf, IM_ARRAYSIZE(nameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Create##PrefabByName")) {
            std::string n = nameBuf;
            if (n.empty()) n = "Entity";
            Prefab::Create(ecs, n);      // calls your prefab module (registered names or fallback)
            nameBuf[0] = '\0';
        }

        // Optional: quick picker of registered prefab names
        {
            auto names = Prefab::Names();
            if (!names.empty()) {
                static int sel = 0;
                if (sel >= (int)names.size()) sel = 0;
                if (ImGui::BeginCombo("Choose prefab", names[sel].c_str())) {
                    for (int i = 0; i < (int)names.size(); ++i) {
                        bool isSel = (i == sel);
                        if (ImGui::Selectable(names[i].c_str(), isSel)) sel = i;
                        if (isSel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Create Selected##Prefab")) {
                    Prefab::Create(ecs, names[sel]);
                }
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
        
        if (entities.size() > 0) {
            if (ImGui::Button("Clone Selected")) {
                selected = ecs.CloneEntity(selected);
                return;
            }

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

                ImGui::PushID(ad); // unique scope for this component�s UI

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

        auto& frc = FrameRateController::GetInstance();
        const double frame_ms = frc.DT() * 1000.0;
        const double fps = frc.FPS();
        //const int cap = frc.CAP();

        ImGui::Text("Frame: %.3f ms (%.1f FPS)", frame_ms, fps);

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

        // Dynamic filters driven by the X-macro lists - I don't even know if this is like the socially acceptable way of doing it
        static bool levelOn[] = {
        #define X(n) true,
            LOG_LEVELS(X)
        #undef X
        };
        static bool systemOn[] = {
        #define X(n) true,
            LOG_SYSTEMS(X)
        #undef X
        };
        static char search[128] = { 0 };
        static bool autoscroll = true;

        // Level row
        ImGui::TextDisabled("Levels:");
        int li = 0;
        #define X(n) do { if (li > 0) ImGui::SameLine(); ImGui::Checkbox(#n, &levelOn[li++]); } while(0);
            LOG_LEVELS(X)
        #undef X

        // Subsystems row (simple grid)
        ImGui::Separator();
        ImGui::TextDisabled("Subsystems:");
        int si = 0;
        const int cols = 4;
        #define X(n) do { if (si % cols != 0) ImGui::SameLine(); ImGui::Checkbox(#n, &systemOn[si++]); } while(0);
            LOG_SYSTEMS(X)
        #undef X

        // Search & options
        ImGui::Separator();
        ImGui::InputTextWithHint("##search", "search text...", search, sizeof(search));
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoscroll);

        ImGui::Separator();

        // Log list
        const auto& logs = Logger::Instance().Get();
        for (const auto& log : logs)
        {
            // Filters
            if (!levelOn[static_cast<int>(log.level)])  continue;
            if (!systemOn[static_cast<int>(log.system)]) continue;
            if (search[0] && std::string(log.text).find(search) == std::string::npos) continue;

            // Color by level
            ImVec4 color =
                (log.level == LogMessage::Level::Error) ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
                (log.level == LogMessage::Level::Warning) ? ImVec4(1.0f, 0.9f, 0.4f, 1.0f) :
                (log.level == LogMessage::Level::Success) ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) :
                ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            // Prefix with level/system for readability
            ImGui::Text("[%s][%s] %s",
                ToString(log.level),
                ToString(log.system),
                log.text.c_str());

            ImGui::PopStyleColor();
        }

        // Controls
        if (autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        if (ImGui::Button("Clear")) {
            Logger::Instance().Clear();
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

        auto* P = DebugReflection::GetGameStateProvider();
        if (!P) { ImGui::TextDisabled("No GameStateProvider registered."); return; }

        auto nameFor = [&](int id) -> const char* {
            for (auto const& kv : P->states()) {
                if (kv.second == id) return kv.first;   // kv = {label, id}
            }
            return "(unknown)";
        };

        const int cur = P->current() ? P->current() : -1;
        const int prev = P->previous() ? P->previous() : -1;
        //const int next = P->next() ? P->next() : -1;

        ImGui::Text("Current:  %s", nameFor(cur));
        ImGui::Text("Previous: %s", nameFor(prev));

        ImGui::Separator();
        int col = 0;
        for (auto const& [label, id] : P->states()) {
            if (ImGui::Button(label)) P->changeTo(id);
            if (++col % 4 != 0) ImGui::SameLine();
        }

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
    void RenderPhysicsPanel([[maybe_unused]] FrameProfiler& profiler, ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui)
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

        ImGui::TextDisabled("Forces & Integrators");

        for (auto* p : DebugReflection::PhysicsRegistry::instance().all()) {
            if (!p) continue;
            if (ImGui::TreeNode(p->name())) {
                p->draw(ecs);            // <- use the ecs param from this function
                ImGui::TreePop();
            }
        }

        ImGui::Separator();

        const auto& evs = phys.collisions();

        // auto pause if requested and collision occured
        if (ui.freezeOnCollision && !paused && !evs.empty()) {
            Physics::PhysicsHandler::SetPaused(true);
            paused = true;
        }

        ImGui::Separator();

        ImGui::Checkbox("Draw overlay", &ui.drawCollOverlay);
        ImGui::SameLine(); ImGui::Checkbox("All colliders", &ui.drawAllColliders);

        // When a way to implement the velocity line happens
        // ImGui::SameLine(); ImGui::Checkbox("Velocities", &ui.drawVelocities);

        // When a way to figure out how to make it a line instead of a whole shape
        //ImGui::DragFloat("Line thickness", &ui.overlayThickness, 0.1f, 1.0f, 8.0f, "%.1f");

        ImGui::TextDisabled("Pairs this frame: %d", (int)evs.size());
    }

    /*!
     @brief Renders the audio panel.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param ui UI state toggles.
    */
    void UI::RenderAudioPanel([[maybe_unused]] FrameProfiler& profiler, [[maybe_unused]] CrashoutsEngine::ECS& ecs, UIState& ui) {

        if (!ui.showAudio) return;
        ScopedWindow w("Audio", LockFlags(ui.lockLayout));

        // shows that fmod is there
        if (!ensureAudio()) {
            ImGui::TextDisabled("FMOD not initialized or banks missing.");
            if (ImGui::Button("Initialize Audio & Load Banks")) {
                ensureAudio();
            }
            return;
        }
        //shows what is loaded
        ImGui::TextDisabled("Banks loaded: Master.*, sfx.bank, Music.bank");
        //button to play the first song
        if (ImGui::Button("Play song")) {
            AudioSystem::instance().playOneShot(kSfx_Beep); 
        }
        //button to play the second song
        if (ImGui::Button("Play SFX )")) {
          
            AudioSystem::instance().playOneShot(kSfx_Beep);
        }
       
        //Stop all music
        if (ImGui::Button("Stop")) {
            AudioSystem::instance().stopAll(true); 
        }

        AudioSystem::instance().update();
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
            ImGui::Checkbox("Audio", &ui.showAudio);
            // Dynamically registered panels
            /*ImGui::Spacing();
            ImGui::TextDisabled("Panels");
            ImGui::Separator();
            for (auto const& p : DebugReflection::PanelRegistry::instance().panels()) {
                if (p.toggleFlag) {
                    ImGui::Checkbox(p.name.c_str(), p.toggleFlag);
                }
                else {
                    ImGui::TextDisabled("%s (always on-demand)", p.name.c_str());
                }
            }*/

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

            // To be fixed later
            /*ImGui::Spacing();
            if (ImGui::Button("Save Layout")) ImGui::SaveIniSettingsToDisk(nullptr);*/
            ImGui::Spacing();
            ImGui::SameLine();
            if (ImGui::Button("Simulate Crash")) ImGui::LoadIniSettingsFromDisk(nullptr);
        }
        ImGui::End();
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
        // Dockspace
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("Dockspace", 0,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

        ImGuiID dockspace_id = ImGui::GetID("Dockspace");
        int ds_flags_i = (int)ImGuiDockNodeFlags_None | (int)ImGuiDockNodeFlags_NoDockingInCentralNode | (int)ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiDockNodeFlags ds_flags = (ImGuiDockNodeFlags)ds_flags_i;
        ImGui::DockSpace(dockspace_id, { 0,0 }, ds_flags);
        ImGui::End();

        // it's all hardcoded? *pulls out glocko* always has been
        if (!s_layoutBuilt) {
            s_layoutBuilt = true;

            // Rebuild docking layout
            ImGui::DockBuilderRemoveNode(dockspace_id);
            int builder_flags_i = (int)ImGuiDockNodeFlags_DockSpace | (int)ImGuiDockNodeFlags_NoDockingInCentralNode;
            ImGuiDockNodeFlags builder_flags = (ImGuiDockNodeFlags)builder_flags_i;
            ImGui::DockBuilderAddNode(dockspace_id, builder_flags);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            // Split into main zones
            ImGuiID dock_main = dockspace_id;
            ImGuiID dock_left, dock_right, dock_bottom;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.18f, &dock_left, &dock_main);
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.26f, &dock_right, &dock_main);
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.28f, &dock_bottom, &dock_main);

            // Further split right side
            ImGuiID dock_right_bottom;
            ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.5f, &dock_right_bottom, &dock_right);

            // Dock windows into regions (matching your actual window titles)
            ImGui::DockBuilderDockWindow("Debug Controls", dock_left);
            ImGui::DockBuilderDockWindow("ECS Demo", dock_right);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
            ImGui::DockBuilderDockWindow("Physics / Collision", dock_bottom);
            ImGui::DockBuilderDockWindow("Console", dock_bottom);
            ImGui::DockBuilderDockWindow("Performance", dock_left);
            ImGui::DockBuilderDockWindow("GameState", dock_bottom);
            ImGui::DockBuilderDockWindow("Audio", dock_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
        }

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

        if (ui.drawCollOverlay) {
            auto& collisions = phys.collisions();
            CollisionGizmos::DrawAllColliders(ecs, &collisions, ui.overlayThickness);
            //CollisionGizmos::DrawContactsAndNormals(collisions, 0.25f, ui.overlayThickness);

            collisions.clear();
        }
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