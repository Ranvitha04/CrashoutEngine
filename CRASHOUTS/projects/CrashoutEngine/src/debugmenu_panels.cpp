/*!
@file       debugmenu_panels.cpp
@author     Renfred Lee
@author     Afiq
@author     Mohamad Amir Akmal
@date       25/12/2025
@brief      File containing the implementation of the ECS Debug Panel for the Debug Menu.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "debugmenu_internals.h"
#include "debugmenu.h"
#include "physicsmanager.h"
#include "deleteentitycommand.h"
#include <unordered_set>
#include "engine.h"
#include "collidereventmanager.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include "createentitycommand.h"
#include "createprefabcommand.h"
#include "visualscriptcommands.h"
#include "undomanager.h"
#include <thread>
#include <atomic>

// --- NEW: Connect our newly modularized Build subsystem! ---
#include "buildanalyzer.h" 

using namespace CrashoutsEngine;
using namespace Serialization;
using namespace KeyframeUnity;

namespace UI {

    /*!
     * @brief Helper function to recursively draw ImGui Tree Nodes for the ECS Hierarchy panel.
     * @param ecs The active ECS registry.
     * @param e The specific Entity ID being drawn.
     * @param selected A reference to the currently selected entity to allow UI updating.
     */
    static void DrawHierarchyNode(ECS& ecs, Entity e, Entity& selected)
    {
        std::string label = ecs.HasComponent<Component::NameComponent>(e) ? ecs.GetComponent<Component::NameComponent>(e)->name : ("Entity " + std::to_string(e));

        const bool hasKids = ecs.HasComponent<Component::ChildrenComponent>(e) && !ecs.GetComponent<Component::ChildrenComponent>(e)->children.empty();

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanFullWidth |
            (selected == e ? ImGuiTreeNodeFlags_Selected : 0) |
            (!hasKids ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0);

        ImGui::PushID((int)e);
        const bool opened = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selected = e;
        }

        // I set up a drag-and-drop payload so users can visually parent entities inside the hierarchy window.
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("HIER_ENTITY", &e, sizeof(Entity));
            ImGui::TextUnformatted(label.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("HIER_ENTITY")) {
                Entity dragged = *(const Entity*)pl->Data;
                if (dragged && dragged != e) {
                    bool ok = true;
                    std::vector<Entity> subtree; subtree.reserve(16);
                    CrashoutsEngine::GatherDescendants(ecs, dragged, subtree);
                    // I prevent an entity from being parented to its own child, which would cause an infinite loop!
                    ok = (std::find(subtree.begin(), subtree.end(), e) == subtree.end());
                    if (ok) CrashoutsEngine::Reparent(ecs, dragged, e);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem("node_ctx")) {
            if (ImGui::MenuItem("Ungroup (detach)"))
                CrashoutsEngine::DetachChild(ecs, e);
            if (ImGui::MenuItem("Create Empty Child")) {
                Entity child = ecs.CreateEntity();
                if (!ecs.HasComponent<Component::NameComponent>(child))
                    ecs.AddComponent<Component::NameComponent>(child, "New Child");
                if (!ecs.HasComponent<Component::ActiveComponent>(child))
                    ecs.AddComponent<Component::ActiveComponent>(child, true);
                if (!ecs.HasComponent<Component::TransformComponent>(child))
                    ecs.AddComponent<Component::TransformComponent>(child);
                CrashoutsEngine::AttachChild(ecs, e, child);
            }
            ImGui::EndPopup();
        }

        if (hasKids && opened) {
            auto* kids = ecs.GetComponent<Component::ChildrenComponent>(e);
            for (Entity c : kids->children)
                DrawHierarchyNode(ecs, c, selected);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    /*!
     @brief Renders the main ECS debug panel, allowing creation, deletion, cloning, and inspection of entities.
     @param ecs ECS reference.
     @param entities List of ECS entities.
     @param selected Currently selected entity.
     @param ui UI state toggles.
    */
    void RenderEcsPanel(CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected, UIState& ui)
    {
        static std::vector<Entity> multiSel;

        ScopedWindow win("Hierarchy", LockFlags(ui.lockLayout));

        // Create entity in the REAL ECS
        if (ImGui::Button("Create Entity")) {
            UndoManager::GetInstance().AddCommand(
                std::make_unique<CreateEntityCommand>()
            );
        }
        ImGui::SeparatorText("Prefab Creator");
        ImGui::Text("Entity Counter: %d", ecs.GetComponentArray<Component::NameComponent>().dense.size());
        static char nameBuf[128] = { 0 };
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
                if (ImGui::Button("Clone Prefab##Prefab")) {
                    UndoManager::GetInstance().AddCommand(
                        std::make_unique<CreatePrefabCommand>(names[sel])
                    );
                }
            }
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Entities");

        ImGui::Separator();

        if (entities.size() > 0) {
            bool hasSelection = (selected != 0);

            // Gray out the button if nothing is selected
            if (!hasSelection) ImGui::BeginDisabled();

            if (ImGui::Button("Clone Selected")) {
                // Double check logic just to be safe
                if (hasSelection) {
                    selected = DeepCloneEntity(ecs, selected);
                    // If we disabled, we must end it before returning!
                    if (!hasSelection) ImGui::EndDisabled();
                    return;
                }
            }

            if (!hasSelection) ImGui::EndDisabled();

            const bool delPressed =
                ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                ImGui::IsKeyPressed(ImGuiKey_Delete, false) &&
                !ImGui::GetIO().WantTextInput &&
                !ImGui::IsAnyItemActive();

            ImGui::SameLine();

            if (ImGui::Button("Destroy Selected") || delPressed)
            {
                if (selected)
                {
                    // Use undoable delete command
                    UndoManager::GetInstance().AddCommand(
                        std::make_unique<DeleteEntityCommand>(selected)
                    );

                    // Update local entities list if necessary (remove invalid IDs)
                    entities.erase(std::remove_if(entities.begin(), entities.end(),
                        [&](Entity e) { return !ecs.HasComponent<Component::NameComponent>(e); }), entities.end());

                    selected = 0;
                }
            }

        }
        ImGui::Text("Selected Entity: %d", selected);
        ImGui::Separator();
        {
            auto& tArr = ecs.GetComponentArray<Component::TransformComponent>();
            bool anyDrawn = false;
            for (Entity e : tArr.indexToEntity)
            {
                if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                const bool hasParent = ecs.HasComponent<Component::ParentComponent>(e);
                if (!hasParent) {
                    DrawHierarchyNode(ecs, e, selected);
                    anyDrawn = true;
                }
            }
            if (!anyDrawn) ImGui::TextDisabled("No entities created yet.");
        }

        if (!selected) return;

        static char prefabNameBuf[64] = "MyBox";
        ImGui::SetNextItemWidth(220);
        ImGui::InputTextWithHint("##save_prefab_name", "Prefab name...", prefabNameBuf, IM_ARRAYSIZE(prefabNameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Save Prefab")) {
            namespace fs = std::filesystem;

            std::string newName;
            if (prefabNameBuf[0]) {
                newName = prefabNameBuf;
            }
            else if (ecs.HasComponent<Component::PrefabInstanceComponent>(selected)) {
                auto* inst = ecs.GetComponent<Component::PrefabInstanceComponent>(selected);
                newName = inst ? inst->prefabName : "Entity";
            }
            else {
                newName = "Entity";
            }

#if USE_IMGUI_EDITOR
            fs::path jsonPath = fs::path(GetGameRoot())
                / "assets"
                / "prefabs"
                / (newName + ".json");
#else
            fs::path jsonPath = fs::path(GetLocalAppData())
                / "prefabs"
                / (newName + ".json");
#endif

            fs::create_directories(jsonPath.parent_path());

            const bool ok = CrashoutsEngine::SavePrefabToJson(
                ecs,
                selected,
                newName,
                jsonPath.generic_string(),
                /*matchName*/ newName
            );

            if (ok) {
#if USE_IMGUI_EDITOR
                const fs::path dirPath = fs::path(GetGameRoot()) / "assets" / "prefabs";
#else
                const fs::path dirPath = fs::path(GetLocalAppData()) / "prefabs";
#endif
                CrashoutsEngine::ReloadPrefabsFromDisk(dirPath.generic_string());
                CrashoutsEngine::RecreateInstancesForPrefab(ecs, newName);

                prefabNameBuf[0] = '\0';
            }

        }
    }

    /*!
     @brief Draws a Unity-style keyframe timeline for animating variables over time.
     @param ecs ECS reference.
     @param selected Currently selected entity to inspect.
     @param ui UI state reference.
    */
    void RenderAnimationPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui)
    {
        using namespace KeyframeUnity;
        using namespace KeyframeUnityEditor;
        using CrashoutsEngine::Entity;

        if (!ui.showAnimation) return;

        ScopedWindow w("Animation", LockFlags(ui.lockLayout));

        if (!selected) {
            ImGui::TextDisabled("No entity selected.");
            return;
        }

        auto* anim = ecs.GetComponent<KeyframeAnimatorComponent>(selected);
        if (!anim) {
            ImGui::TextDisabled("Entity has no KeyframeAnimatorComponent.");
            ImGui::Text("Add it in Inspector first.");
            return;
        }

        AnimationClip* clip = nullptr;
        if (!anim->current.empty()) {
            auto it = anim->clips.find(anim->current);
            if (it != anim->clips.end())
                clip = &it->second;
        }

        if (!clip) {
            ImGui::TextDisabled("Animator has no active clip.");
            ImGui::Text("Use your code to SetClip() and Play().");
            return;
        }

        const float length = clip->Length();
        ImGui::Text("Clip: %s (%.2fs)", clip->name.c_str(), length);
        ImGui::Separator();

        // --- Timeline layout ---
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        const float labelW = 140.0f;
        const float rowH = 20.0f;
        const float totalW = ImGui::GetContentRegionAvail().x;
        const float timeW = totalW - labelW;
        const int   rows = (int)clip->bindings.size();

        for (int bi = 0; bi < rows; ++bi) {
            KFBinding& b = clip->bindings[bi];
            float rowTop = origin.y + bi * rowH;
            float rowMid = rowTop + rowH * 0.5f;

            ImGui::SetCursorScreenPos(ImVec2(origin.x, rowTop));
            ImGui::TextUnformatted(KFPropertyName(b.property));

            ImU32 rowCol = IM_COL32(40, 40, 40, 255);
            draw->AddRectFilled(
                ImVec2(origin.x + labelW, rowTop),
                ImVec2(origin.x + labelW + timeW, rowTop + rowH),
                rowCol);

            int keyCount = (int)b.curve.keys.size();
            for (int ki = 0; ki < keyCount; ++ki) {
                KFKey& k = b.curve.keys[ki];

                float t01 = (length > 0.f) ? (k.time / length) : 0.f;
                t01 = std::clamp(t01, 0.0f, 1.0f);
                float x = origin.x + labelW + t01 * timeW;
                ImVec2 p(x, rowMid);

                float size = 4.0f;
                ImVec2 diamond[4] = {
                    ImVec2(p.x,        p.y - size),
                    ImVec2(p.x + size, p.y),
                    ImVec2(p.x,        p.y + size),
                    ImVec2(p.x - size, p.y)
                };

                bool selectedKey =
                    (gSel.entity == selected &&
                        gSel.bindingIndex == bi &&
                        gSel.keyIndex == ki);

                ImU32 col = selectedKey
                    ? IM_COL32(255, 200, 0, 255)
                    : IM_COL32(200, 200, 200, 255);

                draw->AddConvexPolyFilled(diamond, 4, col);

                ImGui::SetCursorScreenPos(ImVec2(p.x - size - 3.f, p.y - size - 3.f));
                ImGui::InvisibleButton(
                    (std::string("kf_") + std::to_string(bi) + "_" + std::to_string(ki)).c_str(),
                    ImVec2((size + 3.f) * 2.f, (size + 3.f) * 2.f));

                if (ImGui::IsItemClicked()) {
                    gSel.entity = selected;
                    gSel.bindingIndex = bi;
                    gSel.keyIndex = ki;
                }
            }
        }

        ImGui::Dummy(ImVec2(0.f, rows * rowH + 4.f));
        ImGui::Separator();

        // --- Keyframe inspector ---
        if (gSel.entity == selected &&
            gSel.bindingIndex >= 0 &&
            gSel.bindingIndex < (int)clip->bindings.size())
        {
            KFBinding& b = clip->bindings[gSel.bindingIndex];
            if (gSel.keyIndex >= 0 &&
                gSel.keyIndex < (int)b.curve.keys.size())
            {
                KFKey& k = b.curve.keys[gSel.keyIndex];

                ImGui::Text("Keyframe: %s", KFPropertyName(b.property));
                ImGui::DragFloat("Time", &k.time, 0.01f, 0.0f, length);
                ImGui::DragFloat("Value", &k.value, 0.1f);
                ImGui::DragFloat("In Tan", &k.inTan, 0.1f);
                ImGui::DragFloat("Out Tan", &k.outTan, 0.1f);

                int interp = (int)k.interp;
                const char* names[] = { "Constant", "Linear", "Cubic" };
                if (ImGui::Combo("Interpolation", &interp, names, 3)) {
                    k.interp = (KFInterp)interp;
                }
            }
        }
        else {
            ImGui::TextDisabled("Click a keyframe diamond to edit it.");
        }
    }

    /*!
    @brief Renders the Inspector debug panel, dynamically displaying and editing components via reflection.
    @param ecs ECS reference.
    @param selected Currently selected entity.
    @param ui UI state toggles.
    */
    void RenderInspectorPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui)
    {
        if (!ui.showInspector) return;
        ScopedWindow w("Inspector", LockFlags(ui.lockLayout));

        if (!selected) {
            ImGui::TextDisabled("No entity selected.");
            return;
        }

        ImGui::Text("Selected Entity: %u", (unsigned)selected);
        ImGui::SeparatorText("Add Components");

        // Unity-style button
        if (ImGui::Button("Add Component..."))
            ImGui::OpenPopup("add_component_popup");

        // Popup with search + list
        if (ImGui::BeginPopup("add_component_popup"))
        {
            auto& reg = DebugReflection::ComponentRegistry::instance().all();

            static ImGuiTextFilter filter;
            filter.Draw("Search", ImGui::GetFontSize() * 16.0f);

            ImGui::Separator();

            // I build a sorted list of components that the entity DOESN'T yet have to avoid duplicates
            std::vector<DebugReflection::IComponentAdapter*> candidates;
            candidates.reserve(reg.size());
            for (auto* ad : reg)
            {
                if (!ad->has(ecs, selected))
                    candidates.push_back(ad);
            }

            std::sort(candidates.begin(), candidates.end(),
                [](auto* a, auto* b) { return std::strcmp(a->name(), b->name()) < 0; });

            ImGui::BeginChild("add_comp_list", ImVec2(0, 260), true);
            for (auto* ad : candidates)
            {
                const char* label = ad->name();
                if (!filter.PassFilter(label)) continue;

                if (ImGui::Selectable(label))
                {
                    ad->add(ecs, selected);
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
            ImGui::EndChild();

            ImGui::EndPopup();
        }

        ImGui::NewLine();
        ImGui::SeparatorText("Components on Selected Entity");

        // I ensure IDs are unique per entity so ImGui doesn't collapse identical components across different entities.
        ImGui::PushID((int)selected);
        for (auto* ad : DebugReflection::ComponentRegistry::instance().all()) {
            if (!ad->has(ecs, selected)) continue;

            ImGui::PushID(ad);

            ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_AllowOverlap;

            ImGui::Separator();

            bool isOpen = ImGui::TreeNodeEx((void*)ad, headerFlags, "%s", ad->name());

            // Safety Check: I hide the Remove button for "Core Settings" so users don't accidentally break the ECS by deleting Names/UUIDs!
            bool isCoreSettings = (std::strcmp(ad->name(), "Core Settings") == 0);

            if (!isCoreSettings) {
                float remove_w = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                float rightAlignX = ImGui::GetWindowWidth() - remove_w - ImGui::GetStyle().WindowPadding.x;

                ImGui::SameLine(rightAlignX);

                if (ImGui::SmallButton("Remove")) {
                    ad->remove(ecs, selected);

                    if (isOpen) ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }
            }

            if (isOpen) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                ImGui::Spacing();

                ad->drawInspector(ecs, selected);

                ImGui::Spacing();
                ImGui::PopStyleVar();

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
        ImGui::PopID();
    }

    /*!
     @brief Renders the performance panel with profiler results for CPU and Draw calls.
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
        const double fps = frc.FPS();
        const double frame_ms = (fps > 0.0) ? (1000.0 / fps) : 0.0;

        ImGui::Text("Frame: %.3f ms (%.1f FPS)", frame_ms, fps);
        ImGui::Separator();

        // FPS Controls
        int currentCap = frc.CAP();

        ImGui::Text("Target FPS:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        // 0 to 240 range. 0 = Uncapped.
        if (ImGui::DragInt("##FPSLimit", &currentCap, 1, 0, 240, "%d FPS")) {
            if (currentCap < 0) currentCap = 0;
            frc.SetTargetFPS(currentCap);
        }

        if (ImGui::Button("VSync (60)")) frc.SetTargetFPS(60);
        ImGui::SameLine();
        if (ImGui::Button("120")) frc.SetTargetFPS(120);
        ImGui::SameLine();
        if (ImGui::Button("Uncapped")) frc.SetTargetFPS(0);

        ImGui::Separator();
        ImGui::Text("Draw Calls: %u", Component::Rendering::Get().drawCalls);
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
        DrawEngineMemoryPanel();
        DrawProfilerPanel(profiler);
    }

    /*!
     @brief Renders the custom console log viewer panel with system/level filters and auto-scroll.
     @param ui UI state toggles.
    */
    void RenderConsolePanel(UIState& ui) {
        if (!ui.showConsole) return;

        ScopedWindow w("Console", LockFlags(ui.lockLayout));

        // I use X-macros here to automatically generate my filter toggles. It keeps the UI perfectly in sync with the log enums!
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

            ImGui::Separator();
        ImGui::TextDisabled("Subsystems:");
        int si = 0;
        const int cols = 4;
#define X(n) do { if (si % cols != 0) ImGui::SameLine(); ImGui::Checkbox(#n, &systemOn[si++]); } while(0);
        LOG_SYSTEMS(X)
#undef X

            ImGui::Separator();
        ImGui::InputTextWithHint("##search", "search text...", search, sizeof(search));
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoscroll);
        ImGui::Separator();

        const auto& logs = Logger::Instance().Get();

        // Build filtered view (indices so we can clip)
        static std::vector<int> filtered;
        filtered.clear();
        filtered.reserve(logs.size());
        for (int i = 0; i < (int)logs.size(); ++i) {
            const auto& log = logs[i];
            if (!levelOn[(int)log.level])  continue;
            if (!systemOn[(int)log.system]) continue;
            if (search[0] && std::string(log.text).find(search) == std::string::npos) continue;
            filtered.push_back(i);
        }

        // I use ImGuiListClipper here to ensure the engine doesn't drop frames rendering 10,000 logs at once
        ImGuiListClipper clipper;
        clipper.Begin((int)filtered.size());
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const auto& log = logs[filtered[row]];
                const ImVec4 color =
                    (log.level == LogMessage::Level::Error) ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
                    (log.level == LogMessage::Level::Warning) ? ImVec4(1.0f, 0.9f, 0.4f, 1.0f) :
                    (log.level == LogMessage::Level::Success) ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) :
                    ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, color);

                ImGui::Text("[%s][%s] %s", ToString(log.level), ToString(log.system), log.text.c_str());

                ImGui::PopStyleColor();
            }
        }

        static size_t lastLogSize = 0;
        bool newLogsArrived = (logs.size() > lastLogSize);
        lastLogSize = logs.size();

        if (autoscroll && newLogsArrived)
        {
            ImGui::SetScrollHereY(1.0f);
        }

        if (ImGui::Button("Clear")) {
            Logger::Instance().Clear();
        }
    }

    /*!
     @brief Renders the gameState panel for changing high-level FSM states.
     @param ui UI state toggles.
    */
    void RenderGameStatePanel(UIState& ui) {
        if (!ui.showGameState) return;

        ScopedWindow w("GameState", LockFlags(ui.lockLayout));

        auto* P = DebugReflection::GetGameStateProvider();
        if (!P) { ImGui::TextDisabled("No GameStateProvider registered."); return; }

        auto nameFor = [&](int id) -> const char* {
            for (auto const& kv : P->states()) {
                if (kv.second == id) return kv.first;
            }
            return "(unknown)";
            };

        const int cur = P->current() ? P->current() : -1;
        const int prev = P->previous() ? P->previous() : -1;

        ImGui::Text("Current:  %s", nameFor(cur));
        ImGui::Text("Previous: %s", nameFor(prev));

        if (cur != GameState::SCENE && !currSceneName.empty()) currSceneName.clear();

        ImGui::Separator();
        int col = 0;
        for (auto const& [label, id] : P->states()) {
            if (ImGui::Button(label)) P->changeTo(id);
            if (++col % 10 != 0) ImGui::SameLine();
        }

        ImGui::Separator();

        if (ImGui::Button("Restart Current State")) {
            RestartGameState();
        }
        ImGui::SameLine();
        if (ImGui::Button("Quit")) {
            glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
        }
    }

    /*!
     @brief Renders the scene management panel (Create, Switch, Set Start Scene).
     @param ui UI state toggles.
    */
    void RenderGameScenePanel(UIState& ui) {

        if (!ui.showGameScene) return;

        ScopedWindow w("GameScene", LockFlags(ui.lockLayout));

        ImGui::Separator();

        static char sceneNameBuffer[128] = "";

        ImGui::InputText("##SceneNameInput", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));
        if (sceneNameBuffer[0] == '\0') {
            ImGui::SameLine();
            ImGui::TextDisabled("Enter new scene name...");
        }

        ImGui::SameLine();
        if (ImGui::Button("Create Scene")) {
            if (std::strlen(sceneNameBuffer) > 0) {

                std::string newSceneName = sceneNameBuffer;

                if (newSceneName == currSceneName) {
                    LOGE(Core, "Cannot create scene with same name as active scene.");
                }
                else {
                    if (!currSceneName.empty()) {
                        Serialization::SaveScenes(currSceneName);
                        LOGI(Core, ("Saved previous scene: " + currSceneName).c_str());
                    }

                    ECS::getInstance()->Reset();
                    Serialization::SaveScenes(newSceneName);
                    currSceneName = newSceneName;
                    sceneNameBuffer[0] = '\0';
                }
            }
        }

        ImGui::SeparatorText("Loaded Scenes (Right-Click to set Start Scene)");

        if (ImGui::Button("Refresh / Sync Missing Scenes")) {
            std::string manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
            std::string scenesDir = GetGameRoot() + "/assets/scenes";

            std::ifstream ifs(manifestPath);
            if (ifs.is_open()) {
                rapidjson::IStreamWrapper isw(ifs);
                rapidjson::Document doc;
                doc.ParseStream(isw);
                ifs.close();

                if (!doc.HasParseError() && doc.HasMember("scenes") && doc["scenes"].IsArray()) {
                    rapidjson::Value& scenesArray = doc["scenes"];

                    for (int i = scenesArray.Size() - 1; i >= 0; --i) {
                        if (scenesArray[i].HasMember("name")) {
                            std::string sceneName = scenesArray[i]["name"].GetString();
                            std::filesystem::path fullPath = std::filesystem::path(scenesDir) / sceneName;

                            if (!std::filesystem::exists(fullPath)) {
                                LOGI(Core, ("Removing deleted scene from manifest: " + sceneName).c_str());
                                scenesArray.Erase(scenesArray.Begin() + i);
                            }
                        }
                    }

                    std::ofstream ofs(manifestPath);
                    rapidjson::OStreamWrapper osw(ofs);
                    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
                    doc.Accept(writer);
                }
            }
        }

        std::ifstream ifs(Assets::assetManifestPath);
        if (!ifs.is_open()) return;

        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) return;

        std::string currentStartScene = "";
        if (doc.HasMember("start_scene") && doc["start_scene"].IsString()) {
            currentStartScene = doc["start_scene"].GetString();
        }
        else if (doc.HasMember("scenes") && doc["scenes"].IsArray() && doc["scenes"].Size() > 0) {
            if (doc["scenes"][0].HasMember("name")) {
                currentStartScene = doc["scenes"][0]["name"].GetString();
            }
        }

        if (doc.HasMember("scenes") && doc["scenes"].IsArray()) {
            const rapidjson::Value& sceneArray = doc["scenes"];
            std::unordered_set<std::string> seenSceneNames;
            seenSceneNames.reserve(sceneArray.Size());

            ImGuiStyle& style = ImGui::GetStyle();
            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            bool first_button = true;

            for (rapidjson::SizeType i = 0; i < sceneArray.Size(); ++i) {
                const rapidjson::Value& asset = sceneArray[i];
                if (asset.HasMember("name") && asset.HasMember("path")) {
                    std::string name = asset["name"].GetString();
                    std::string relativePath = asset["path"].GetString();
                    if (!seenSceneNames.insert(name).second) continue;

                    bool isStartScene = (name == currentStartScene);
                    std::string btnText = isStartScene ? ("[*] " + name) : name;
                    std::string btnId = btnText + "##scene_" + relativePath + "_" + std::to_string((int)i);

                    ImVec2 text_size = ImGui::CalcTextSize(btnText.c_str());
                    float expected_button_width = text_size.x + style.FramePadding.x * 2.0f;

                    if (!first_button) {
                        float last_button_x2 = ImGui::GetItemRectMax().x;
                        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + expected_button_width;
                        if (next_button_x2 < window_visible_x2) ImGui::SameLine();
                    }
                    first_button = false;

                    if (isStartScene) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                    }

                    if (ImGui::Button(btnId.c_str())) {
                        if (!currSceneName.empty() && engineState == EngineState::STOP) {
                            Serialization::SaveScenes(currSceneName);
                        }
                        g_PendingSceneName = name;
                        g_IsSceneLoadPending = true;

                        if (engineState != EngineState::PLAY) {
                            ::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(true);
                        }
                    }

                    // Uses the exposed UI function from buildanalyzer.h
                    if (ImGui::BeginPopupContextItem(("Ctx_" + name).c_str())) {
                        if (ImGui::MenuItem("Set as Start Scene")) {
                            SetStartSceneInManifest(name);
                        }
                        ImGui::EndPopup();
                    }

                    if (isStartScene) {
                        ImGui::PopStyleColor(2);
                    }
                }
            }
        }
    }

    /*!
     @brief Renders the physics panel, including collision event listings and quadtree controls.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param ui UI state toggles.
    */
    void RenderPhysicsPanel([[maybe_unused]] FrameProfiler& profiler, ECS& ecs, UIState& ui)
    {
        if (!ui.showPhysics) return;
        ScopedWindow w("Physics / Collision", LockFlags(ui.lockLayout));

        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            ::Physics::PhysicsHandler::StepOnce(); Help("Advance one simulation tick when paused.");
        }

        ImGui::Checkbox("Freeze on collision", &ui.freezeOnCollision);
        ImGui::SameLine(); Help("Automatically pause when any collision is detected this frame.");

        static bool overrideDt = false;
        static double dbgDt = 1.0 / 30.0;
        ImGui::Checkbox("Override dt", &overrideDt); ImGui::SameLine();
        if (overrideDt && ImGui::DragScalar("dt", ImGuiDataType_Double, &dbgDt, 0.001f, nullptr, nullptr, "%.4f")) {
            ::Physics::PhysicsHandler::SetOverrideDt(true);
            ::Physics::PhysicsHandler::SetDebugDt(dbgDt);
        }
        else if (!overrideDt) {
            ::Physics::PhysicsHandler::SetOverrideDt(false);
        }

        ImGui::Separator();

        ImGui::TextDisabled("Forces & Integrators");

        for (auto* p : DebugReflection::PhysicsRegistry::instance().all()) {
            if (!p) continue;
            if (ImGui::TreeNode(p->name())) {
                p->draw(ecs);
                ImGui::TreePop();
            }
        }

        ImGui::Separator();

        const auto& evs = Collision::CollisionHandler::GetInstance().GetFrameCollisions();

        ImGui::Separator();

        ImGui::Checkbox("Draw overlay", &ui.drawCollOverlay);
        ImGui::SameLine(); ImGui::Checkbox("All colliders", &ui.drawAllColliders);
        ImGui::SameLine(); ImGui::Checkbox("QuadTree", &ui.drawQuadTree);
        ImGui::SameLine(); ImGui::Checkbox("Script Debug Draw", &ui.drawGameDebug);

        ImGui::Separator();

        Collision::CollisionHandler& collisionHandler{ Collision::CollisionHandler::GetInstance() };

        f32 newMin[2]{ collisionHandler.staticQuadTree.boundary.min.x, collisionHandler.staticQuadTree.boundary.min.y };
        f32 newMax[2]{ collisionHandler.staticQuadTree.boundary.max.x, collisionHandler.staticQuadTree.boundary.max.y };

        bool boundsChanged = false;
        if (ImGui::DragFloat2("Min", newMin)) {
            boundsChanged = true;
        }
        if (ImGui::DragFloat2("Max", newMax)) {
            boundsChanged = true;
        }

        if (boundsChanged) {
            Collision::AABBRect newBounds{ CEVec3{newMin[0], newMin[1], 0.f}, CEVec3{newMax[0], newMax[1], 0.f} };

            // I resize both trees so they match perfectly, preventing edge-case boundary errors
            collisionHandler.staticQuadTree.Resize(newBounds);
            collisionHandler.dynamicQuadTree.Resize(newBounds);

            collisionHandler.isStaticDirty = true;
        }

        ImGui::TextDisabled("Pairs this frame: %d", (int)evs.size());
    }

    /*!
     @brief Renders the Audio control panel, interfacing directly with the FMOD system.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param ui UI state toggles.
    */
    void RenderAudioPanel([[maybe_unused]] FrameProfiler& profiler, [[maybe_unused]] CrashoutsEngine::ECS& ecs, UIState& ui)
    {
        if (!ui.showAudio) return;
        ScopedWindow w("Audio", LockFlags(ui.lockLayout));

        auto& audio = Audio::AudioSystem::instance();

        struct EditorHandle {
            void* ptr = nullptr;
            enum Type { None, Event, Channel, MusicTag } type = None;
        };

        static float s_panelFadeDuration = 0.5f;

        auto PlayAny = [&](const std::string& id, bool bgm = false) -> EditorHandle
            {
                EditorHandle result;
                if (id.empty()) return result;

                if (id.rfind("event:/", 0) == 0) {
                    auto* inst = audio.createInstance(id);
                    if (inst) {
                        inst->start();
                        result.ptr = inst;
                        result.type = EditorHandle::Event;
                    }
                    return result;
                }

                std::string full = id;
                if (id.rfind("[RAW] ", 0) == 0) {
                    std::string rel = id.substr(6);

                    if (rel.rfind("assets/", 0) == 0 || rel.rfind("assets\\", 0) == 0) {
                        full = rel;
                    }
                    else {
                        full = "assets/audio/" + rel;
                    }
                }
                else if (full.rfind("assets/", 0) != 0) {
                    full = "assets/audio/" + full;
                }

                if (bgm) {
                    audio.playMusicFileTagged("BGM", full, true);
                    result.type = EditorHandle::MusicTag;
                    return result;
                }

                FMOD::Channel* ch = audio.playSFXFile(full, false, 1.0f, 100.0f);
                if (ch) {
                    result.ptr = ch;
                    result.type = EditorHandle::Channel;
                }
                return result;
            };

        auto StopAny = [&](EditorHandle& h)
            {
                if (h.type == EditorHandle::None) return;

                if (h.type == EditorHandle::Event) {
                    auto* inst = static_cast<FMOD::Studio::EventInstance*>(h.ptr);
                    if (inst) {
                        inst->stop(s_panelFadeDuration > 0.f ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
                        inst->release();
                    }
                }
                else if (h.type == EditorHandle::MusicTag) {
                    audio.stopTagged("BGM", s_panelFadeDuration);
                }
                else if (h.type == EditorHandle::Channel) {
                    auto* ch = static_cast<FMOD::Channel*>(h.ptr);
                    if (ch) ch->stop();
                }
                h = EditorHandle{};
            };

        auto IsPlaying = [&](EditorHandle& h) -> bool {
            if (h.type == EditorHandle::None) return false;
            if (h.type == EditorHandle::MusicTag) return true;
            if (h.type == EditorHandle::Channel) {
                bool p = false;
                ((FMOD::Channel*)h.ptr)->isPlaying(&p);
                return p;
            }
            if (h.type == EditorHandle::Event) {
                FMOD_STUDIO_PLAYBACK_STATE st;
                ((FMOD::Studio::EventInstance*)h.ptr)->getPlaybackState(&st);
                return st != FMOD_STUDIO_PLAYBACK_STOPPED;
            }
            return false;
            };

        if (!audio.isInitialized())
        {
            ImGui::TextDisabled("FMOD not initialized.");
            if (ImGui::Button("Initialize Audio (assets/audio)"))
                audio.init("assets/audio");
            return;
        }

        static bool needRefresh = false;
        if (ImGui::Button("Reload Manifest / Files"))
        {
            audio.stopAll(true);
            audio.stopAllRaw();
            audio.unloadAllBanks();
            audio.unloadAllRawSounds();
            CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
            needRefresh = true;
        }

        static std::vector<std::string> allEvents;
        if (needRefresh || allEvents.empty())
        {
            allEvents.clear();

            audio.listAllEventPaths(allEvents);

            std::vector<std::string> rawFiles;
            audio.listRawAudioFiles(rawFiles);

            for (auto& f : rawFiles) {
                allEvents.push_back(f);
            }

            std::sort(allEvents.begin(), allEvents.end());
            allEvents.erase(std::unique(allEvents.begin(), allEvents.end()), allEvents.end());
            needRefresh = false;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%zu items", allEvents.size());

        auto EventCombo = [&](const char* label, std::string& sel) {
            if (ImGui::BeginCombo(label, sel.empty() ? "(select)" : sel.c_str())) {
                for (auto& e : allEvents) {
                    if (ImGui::Selectable(e.c_str(), sel == e)) sel = e;
                }
                ImGui::EndCombo();
            }
            };

        ImGui::SeparatorText("Debug Settings");
        ImGui::SliderFloat("Fade Out Duration (s)", &s_panelFadeDuration, 0.0f, 5.0f);

        if (audio.isInitialized())
        {
            float masterVol = audio.GetMasterVolume();
            if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f, "%.2f")) {
                audio.setMasterVolume(masterVol);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##Master")) {
                masterVol = 1.0f;
                audio.setMasterVolume(masterVol);
            }

            float musicVol = audio.GetMusicVolume();
            if (ImGui::SliderFloat("Music Volume", &musicVol, 0.0f, 1.0f, "%.2f")) {
                audio.setMusicGroupVolume(musicVol);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Settings::SaveEditorSettings(ui);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##Music")) {
                musicVol = 1.0f;
                audio.setMusicGroupVolume(musicVol);
                Settings::SaveEditorSettings(ui);
            }

            float sfxVol = audio.GetSFXVolume();
            if (ImGui::SliderFloat("SFX Volume", &sfxVol,
                0.0f, 1.0f, "%.2f")) {
                audio.setSFXGroupVolume(sfxVol);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Settings::SaveEditorSettings(ui);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##SFX")) {
                sfxVol = 1.0f;
                audio.setSFXGroupVolume(sfxVol);
                Settings::SaveEditorSettings(ui);
            }

            if (ImGui::Button("Log Volumes")) {
                std::string msg = "Master=" + std::to_string(audio.GetMasterVolume()) +
                    " Music=" + std::to_string(audio.GetMusicVolume()) +
                    " SFX=" + std::to_string(audio.GetSFXVolume());
                LOGI(Core, msg.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Unmute All")) {
                audio.setBusMute("bus:/", false);
                audio.setBusMute("bus:/Master", false);
                audio.setMasterVolume(1.0f);
                audio.setMusicGroupVolume(1.0f);
                audio.setSFXGroupVolume(1.0f);
            }
        }
        else
        {
            ImGui::TextDisabled("Audio not initialized (init first to show volumes).");
        }

        ImGui::SeparatorText("Quick Test");

        static EditorHandle hBGM, hSFX1, hSFX2;
        static std::string pickBGM, pickSFX1, pickSFX2;

        auto DrawSmartControl = [&](const char* label, std::string& pick, EditorHandle& h, bool isBGM) {
            ImGui::PushID(label);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

            EventCombo(label, pick);

            ImGui::SameLine();

            if (IsPlaying(h)) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
                if (ImGui::Button("Stop", ImVec2(-1, 0))) StopAny(h);
                ImGui::PopStyleColor();
            }
            else {
                bool hasSel = !pick.empty();
                if (!hasSel) ImGui::BeginDisabled();
                if (ImGui::Button("Play", ImVec2(-1, 0))) h = PlayAny(pick, isBGM);
                if (!hasSel) ImGui::EndDisabled();
            }
            ImGui::PopID();
            };

        DrawSmartControl("Background Music", pickBGM, hBGM, true);
        DrawSmartControl("SFX 1", pickSFX1, hSFX1, false);
        DrawSmartControl("SFX 2", pickSFX2, hSFX2, false);

        ImGui::Separator();
        if (ImGui::Button("Stop All (Panic)", ImVec2(-1, 0))) {
            audio.stopAll(true);
            hBGM = {}; hSFX1 = {}; hSFX2 = {};
        }

        audio.Update((float)GLApp::delta_time);
    }

    /*!
     @brief Renders the Scene control bar on top of the game viewport (Play/Pause/Stop).
     @param ui UI state toggles.
    */
    void RenderEngineStateBar(UIState& ui) {
        if (!ui.showEngineControl) return;

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove;

        static CEVec3 s_SavedEditorCamPos{ 0.0f, 0.0f, 0.0f };
        static float s_SavedEditorCamZoom = 1.0f;

        if (ImGui::Begin("Engine Controls", nullptr, window_flags)) {

            // I only save the scene if we are moving from STOP to PLAY.
            if (ImGui::Button("PLAY", ImVec2(70, 25))) {
                if (engineState == EngineState::STOP && !currSceneName.empty()) {
                    Serialization::SaveScenes(currSceneName);

                    // I save the camera state before we enter play mode so we can restore the editor view when they stop.
                    s_SavedEditorCamPos = CrashoutsEngine::RenderCamera::engineCamera.GetPosition();
                    s_SavedEditorCamZoom = CrashoutsEngine::RenderCamera::engineCamera.GetZoom();
                }

                engineState = EngineState::PLAY;

                ::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(false);
                CrashoutsEngine::Audio::AudioSystem::instance().setPaused(false);
            }

            ImGui::SameLine();

            bool disablePause = (engineState == EngineState::STOP);

            if (disablePause) ImGui::BeginDisabled();

            if (ImGui::Button("PAUSE", ImVec2(70, 25))) {
                // I just change state and pause systems. NO SAVING HERE.
                engineState = EngineState::PAUSE;
                ::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(true);
                CrashoutsEngine::Audio::AudioSystem::instance().setPaused(true);
            }

            if (disablePause) ImGui::EndDisabled();

            ImGui::SameLine();

            bool disableStop = (engineState == EngineState::STOP);

            if (disableStop) ImGui::BeginDisabled();

            if (ImGui::Button("STOP", ImVec2(70, 25))) {
                // I NEVER save the scene here! 
                // We want to discard everything that happened during Play/Pause.

                if (currSceneName.empty()) RestartGameState();
                else {
                    g_PendingSceneName = currSceneName;
                    g_IsSceneLoadPending = true;
                }

                engineState = EngineState::STOP;

                ::Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(true);

                auto& audio = CrashoutsEngine::Audio::AudioSystem::instance();
                audio.stopAll(false);
                audio.stopAllChannels();
                audio.setPaused(false);

                // I restore the camera state on STOP so the user doesn't lose their place in the editor.
                CrashoutsEngine::RenderCamera::engineCamera.SetPostiion(s_SavedEditorCamPos);
                CrashoutsEngine::RenderCamera::engineCamera.SetZoom(s_SavedEditorCamZoom);
                CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(Settings::GetScreenWidth(), Settings::GetScreenHeight());
            }

            if (disableStop) ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            if (ImGui::Button("Recompile C#", ImVec2(100, 25))) {

                // Clear the console first for readability
                Logger::Instance().Clear();

                std::string gameName = std::filesystem::path(GetGameRoot()).filename().string();
                std::filesystem::path csprojPath = std::filesystem::path(GetGameRoot()) / "scripts" / (gameName + ".csproj");

                std::string config = "Release";
#ifdef _DEBUG
                config = "Debug";
#endif

                std::string buildCmd = "dotnet build \"" + csprojPath.generic_string() + "\" -c " + config;
                LOGI(Build, ("Recompiling scripts for " + gameName + " in " + config + " mode...").c_str());

                // Uses the exposed UI function from buildanalyzer.h
                int result = ExecuteCommandHidden(buildCmd);

                if (result == 0) {
                    LOGS(Build, "Compile Success! Hot Reloading...");

                    CrashoutsEngine::ScriptSystem::ReloadAll();
                }
                else {
                    LOGE(Build, "Compile Failed! Check your C# syntax in the console above.");
                }
            }
            ImGui::PopStyleColor();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            const char* stateText = "UNKNOWN";
            switch (engineState) {
            case EngineState::PLAY:  stateText = "PLAYING"; break;
            case EngineState::PAUSE: stateText = "PAUSED";  break;
            case EngineState::STOP:  stateText = "STOPPED"; break;
            }
            ImGui::TextDisabled("State: %s", stateText);
        }

        ImGui::End();
    }

    /*!
     @brief Renders the Visual Scripting Node Editor.
     @param ui UI state toggles.
    */
    void RenderVisualScriptingPanel(UIState& ui)
    {
        static ImVec2 dragStartPos(0.0f, 0.0f);
        if (!ui.showVisualScripting) return;

        ScopedWindow w("Visual Scripting", LockFlags(ui.lockLayout));

        auto& graph = UI::GetVisualScriptingGraph();
        auto& nodes = graph.nodes;
        auto& links = graph.links;
        auto& nextNodeId = graph.nextNodeId;

        ImGui::Text("Graph: %s", ui.openedVisualScriptPath.empty() ? "(none)" : ui.openedVisualScriptPath.c_str());

        if (ImGui::Button("Save Graph")) {
            if (!ui.openedVisualScriptPath.empty()) {
                if (!UI::SaveVisualScriptingGraph(ui.openedVisualScriptPath, graph)) {
                    ImGui::OpenPopup("VS Save Failed");
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Graph")) {
            if (!ui.openedVisualScriptPath.empty()) {
                UI::ClearVisualScriptingGraph();
                if (!UI::LoadVisualScriptingGraph(ui.openedVisualScriptPath, UI::GetVisualScriptingGraph())) {
                    ImGui::OpenPopup("VS Load Failed");
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Close Graph")) {
            ui.showVisualScripting = false;
        }

        if (ImGui::BeginPopupModal("VS Save Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to save graph.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("VS Load Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load graph.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        static int contextNodeId = -1;
        static int selectedNode = -1;
        static int draggingNode = -1;
        static ImVec2 dragOffset(0, 0);
        static bool showGrid = true;
        static bool creatingLink = false;
        static int linkStartPin = 0;
        static int linkStartNode = -1;
        static int inspectorNodeId = -1;
        static char nodeTitleBuffer[128] = "";

        // I use a lambda here so deleting a node instantly cleans up UI interaction states
        auto deleteNodeById = [&](int nodeId)
            {
                UndoManager::GetInstance().AddCommand(
                    std::make_unique<DeleteVSNodeCommand>(graph, nodeId)
                );

                if (selectedNode == nodeId) selectedNode = -1;
                if (draggingNode == nodeId) draggingNode = -1;
                if (linkStartNode == nodeId) linkStartNode = -1;

                creatingLink = false;
                inspectorNodeId = -1;
                nodeTitleBuffer[0] = '\0';
            };

        ImGui::Checkbox("Show Grid", &showGrid);
        ImGui::Separator();

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < 300.0f) canvasSize.x = 300.0f;
        if (canvasSize.y < 250.0f) canvasSize.y = 250.0f;

        ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        ImDrawList* draw = ImGui::GetWindowDrawList();

        draw->AddRectFilled(canvasPos, canvasEnd, IM_COL32(35, 35, 40, 255));
        draw->AddRect(canvasPos, canvasEnd, IM_COL32(255, 255, 255, 60));

        if (showGrid)
        {
            const float gridStep = 32.0f;
            for (float x = canvasPos.x; x < canvasEnd.x; x += gridStep)
                draw->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasEnd.y), IM_COL32(255, 255, 255, 20));

            for (float y = canvasPos.y; y < canvasEnd.y; y += gridStep)
                draw->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasEnd.x, y), IM_COL32(255, 255, 255, 20));
        }

        auto getNodeRectMin = [&](const VSNode& node)
            {
                return ImVec2(canvasPos.x + node.pos.x, canvasPos.y + node.pos.y);
            };

        auto getNodeRectMax = [&](const VSNode& node)
            {
                ImVec2 minP = getNodeRectMin(node);
                return ImVec2(minP.x + node.size.x, minP.y + node.size.y);
            };

        auto getInputPinPos = [&](const VSNode& node, int pinIndex)
            {
                ImVec2 minP = getNodeRectMin(node);

                int inputCount = (int)node.inputs.size();
                if (inputCount <= 0)
                    return ImVec2(minP.x, minP.y + node.size.y * 0.5f);

                if (inputCount == 1)
                    return ImVec2(minP.x, minP.y + node.size.y * 0.5f);

                // Space pins evenly across the Y axis of the node
                float spacing = node.size.y / float(inputCount + 1);
                return ImVec2(minP.x, minP.y + spacing * float(pinIndex + 1));
            };

        auto getOutputPinPos = [&](const VSNode& node, int pinIndex)
            {
                ImVec2 minP = getNodeRectMin(node);

                if (node.type == VSNodeType::Branch)
                {
                    if (pinIndex == 0)
                        return ImVec2(minP.x + node.size.x, minP.y + node.size.y * 0.30f);
                    if (pinIndex == 1)
                        return ImVec2(minP.x + node.size.x, minP.y + node.size.y * 0.70f);
                }

                int outputCount = (int)node.outputs.size();
                if (outputCount <= 0)
                    return ImVec2(minP.x + node.size.x, minP.y + node.size.y * 0.5f);

                if (outputCount == 1)
                    return ImVec2(minP.x + node.size.x, minP.y + node.size.y * 0.5f);

                // Space pins evenly across the Y axis of the node
                float spacing = node.size.y / float(outputCount + 1);
                return ImVec2(minP.x + node.size.x, minP.y + spacing * float(pinIndex + 1));
            };

        auto findNodeById = [&](int id) -> VSNode*
            {
                for (auto& node : nodes)
                {
                    if (node.id == id) return &node;
                }
                return nullptr;
            };

        auto isMouseNearPoint = [&](ImVec2 p, float radius)
            {
                ImVec2 mouse = ImGui::GetIO().MousePos;
                float dx = mouse.x - p.x;
                float dy = mouse.y - p.y;
                return (dx * dx + dy * dy) <= (radius * radius);
            };

        auto getTypeName = [](VSNodeType type) -> const char*
            {
                switch (type)
                {
                case VSNodeType::Start:        return "Start";
                case VSNodeType::Move:         return "Move";
                case VSNodeType::Attack:       return "Attack";
                case VSNodeType::Wait:         return "Wait";
                case VSNodeType::Branch:       return "Branch";
                case VSNodeType::ScriptCall:   return "ScriptCall";
                case VSNodeType::BoolConstant: return "BoolConstant";
                case VSNodeType::IntConstant:  return "IntConstant";
                case VSNodeType::FloatConstant:return "FloatConstant";
                case VSNodeType::GetVariable:  return "GetVariable";
                case VSNodeType::SetVariable:  return "SetVariable";
                case VSNodeType::Add:          return "Add";
                case VSNodeType::Subtract:     return "Subtract";
                case VSNodeType::Multiply:     return "Multiply";
                case VSNodeType::Divide:       return "Divide";
                case VSNodeType::Equal:        return "Equal";
                case VSNodeType::NotEqual:     return "NotEqual";
                case VSNodeType::GreaterThan:  return "GreaterThan";
                case VSNodeType::LessThan:     return "LessThan";
                case VSNodeType::GreaterEqual: return "GreaterEqual";
                case VSNodeType::LessEqual:    return "LessEqual";
                case VSNodeType::And:          return "And";
                case VSNodeType::Or:           return "Or";
                case VSNodeType::Not:          return "Not";
                default:                       return "Unknown";
                }
            };

        auto rebuildNodePins = [&](VSNode& node)
            {
                node.inputs.clear();
                node.outputs.clear();

                switch (node.type)
                {
                case VSNodeType::Start:
                {
                    VSPin outPin;
                    outPin.id = graph.nextPinId++;
                    outPin.name = "Next";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Exec;
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(140.0f, 70.0f);
                    break;
                }

                case VSNodeType::Move:
                case VSNodeType::Attack:
                case VSNodeType::Wait:
                case VSNodeType::ScriptCall:
                {
                    VSPin inPin;
                    inPin.id = graph.nextPinId++;
                    inPin.name = "In";
                    inPin.kind = VSPinKind::Input;
                    inPin.type = VSDataType::Exec;
                    node.inputs.push_back(inPin);

                    VSPin outPin;
                    outPin.id = graph.nextPinId++;
                    outPin.name = "Next";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Exec;
                    node.outputs.push_back(outPin);

                    node.size = ImVec2(140.0f, 70.0f);
                    break;
                }
                case VSNodeType::GetVariable:
                case VSNodeType::SetVariable:
                {
                    VSPin outPin;
                    outPin.id = graph.nextPinId++;
                    outPin.name = "Next";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Exec;
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(140.0f, 70.0f);
                    break;
                }

                case VSNodeType::BoolConstant:
                {
                    VSPin outPin;
                    outPin.id = graph.nextPinId++;
                    outPin.name = "Value";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Bool;
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 90.0f);
                    break;
                }

                case VSNodeType::IntConstant:
                {
                    VSPin outPin;
                    outPin.id = graph.nextPinId++;
                    outPin.name = "Value";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Int;
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 90.0f);
                    break;
                }

                case VSNodeType::FloatConstant:
                {
                    VSPin outPin;
                    outPin.id = graph.nextPinId++;
                    outPin.name = "Value";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Float;
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 90.0f);
                    break;
                }

                case VSNodeType::Branch:
                {
                    VSPin execIn;
                    execIn.id = graph.nextPinId++;
                    execIn.name = "In";
                    execIn.kind = VSPinKind::Input;
                    execIn.type = VSDataType::Exec;
                    node.inputs.push_back(execIn);

                    VSPin condIn;
                    condIn.id = graph.nextPinId++;
                    condIn.name = "Condition";
                    condIn.kind = VSPinKind::Input;
                    condIn.type = VSDataType::Bool;
                    node.inputs.push_back(condIn);

                    VSPin truePin;
                    truePin.id = graph.nextPinId++;
                    truePin.name = "True";
                    truePin.kind = VSPinKind::Output;
                    truePin.type = VSDataType::Exec;
                    node.outputs.push_back(truePin);

                    VSPin falsePin;
                    falsePin.id = graph.nextPinId++;
                    falsePin.name = "False";
                    falsePin.kind = VSPinKind::Output;
                    falsePin.type = VSDataType::Exec;
                    node.outputs.push_back(falsePin);

                    node.size = ImVec2(180.0f, 110.0f);
                    break;
                }

                case VSNodeType::Add:
                case VSNodeType::Subtract:
                case VSNodeType::Multiply:
                case VSNodeType::Divide:
                {
                    VSPin inA, inB, outPin;
                    inA.id = graph.nextPinId++;
                    inA.name = "A";
                    inA.kind = VSPinKind::Input;
                    inA.type = VSDataType::Float;

                    inB.id = graph.nextPinId++;
                    inB.name = "B";
                    inB.kind = VSPinKind::Input;
                    inB.type = VSDataType::Float;

                    outPin.id = graph.nextPinId++;
                    outPin.name = "Result";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Float;

                    node.inputs.push_back(inA);
                    node.inputs.push_back(inB);
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 100.0f);
                    break;
                }

                case VSNodeType::Equal:
                case VSNodeType::NotEqual:
                case VSNodeType::GreaterThan:
                case VSNodeType::LessThan:
                case VSNodeType::GreaterEqual:
                case VSNodeType::LessEqual:
                {
                    VSPin inA, inB, outPin;
                    inA.id = graph.nextPinId++;
                    inA.name = "A";
                    inA.kind = VSPinKind::Input;
                    inA.type = VSDataType::Float;

                    inB.id = graph.nextPinId++;
                    inB.name = "B";
                    inB.kind = VSPinKind::Input;
                    inB.type = VSDataType::Float;

                    outPin.id = graph.nextPinId++;
                    outPin.name = "Result";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Bool;

                    node.inputs.push_back(inA);
                    node.inputs.push_back(inB);
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 100.0f);
                    break;
                }

                case VSNodeType::And:
                case VSNodeType::Or:
                {
                    VSPin inA, inB, outPin;
                    inA.id = graph.nextPinId++;
                    inA.name = "A";
                    inA.kind = VSPinKind::Input;
                    inA.type = VSDataType::Bool;

                    inB.id = graph.nextPinId++;
                    inB.name = "B";
                    inB.kind = VSPinKind::Input;
                    inB.type = VSDataType::Bool;

                    outPin.id = graph.nextPinId++;
                    outPin.name = "Result";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Bool;

                    node.inputs.push_back(inA);
                    node.inputs.push_back(inB);
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 100.0f);
                    break;
                }

                case VSNodeType::Not:
                {
                    VSPin inA, outPin;
                    inA.id = graph.nextPinId++;
                    inA.name = "A";
                    inA.kind = VSPinKind::Input;
                    inA.type = VSDataType::Bool;

                    outPin.id = graph.nextPinId++;
                    outPin.name = "Result";
                    outPin.kind = VSPinKind::Output;
                    outPin.type = VSDataType::Bool;

                    node.inputs.push_back(inA);
                    node.outputs.push_back(outPin);
                    node.size = ImVec2(180.0f, 90.0f);
                    break;
                }

                default:
                    node.size = ImVec2(140.0f, 70.0f);
                    break;
                }
            };

        ImGui::InvisibleButton("VisualScriptingCanvas", canvasSize);
        bool canvasHovered = ImGui::IsItemHovered();
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        bool mouseClicked = ImGui::IsMouseClicked(0);
        bool mouseDown = ImGui::IsMouseDown(0);
        bool mouseReleased = ImGui::IsMouseReleased(0);

        if (canvasHovered && ImGui::IsMouseClicked(1))
        {
            contextNodeId = -1;

            for (int i = (int)nodes.size() - 1; i >= 0; --i)
            {
                ImVec2 minP = getNodeRectMin(nodes[i]);
                ImVec2 maxP = getNodeRectMax(nodes[i]);

                if (mousePos.x >= minP.x && mousePos.x <= maxP.x &&
                    mousePos.y >= minP.y && mousePos.y <= maxP.y)
                {
                    contextNodeId = nodes[i].id;
                    selectedNode = nodes[i].id;
                    ImGui::OpenPopup("VSNodeContextPopup");
                    break;
                }
            }

            if (contextNodeId == -1)
                ImGui::OpenPopup("VSAddNodePopup");
        }

        if (ImGui::BeginPopup("VSAddNodePopup"))
        {
            ImVec2 popupMouse = ImGui::GetMousePos();
            ImVec2 localPos(
                popupMouse.x - canvasPos.x,
                popupMouse.y - canvasPos.y
            );

            auto addNode = [&](VSNodeType type, const char* title)
                {
                    VSNode node{};
                    node.id = nextNodeId++;
                    node.type = type;
                    node.title = title;
                    node.pos = localPos;
                    node.waitTime = 1.0f;

                    rebuildNodePins(node);

                    UndoManager::GetInstance().AddCommand(
                        std::make_unique<CreateVSNodeCommand>(graph, node)
                    );

                    selectedNode = node.id;
                    inspectorNodeId = node.id;
                };

            if (ImGui::MenuItem("Add Start Node"))          addNode(VSNodeType::Start, "Start");
            if (ImGui::MenuItem("Add Move Node"))           addNode(VSNodeType::Move, "Move");
            if (ImGui::MenuItem("Add Attack Node"))         addNode(VSNodeType::Attack, "Attack");
            if (ImGui::MenuItem("Add Wait Node"))           addNode(VSNodeType::Wait, "Wait");
            if (ImGui::MenuItem("Add Branch Node"))         addNode(VSNodeType::Branch, "Branch");
            if (ImGui::MenuItem("Add Script Call Node"))    addNode(VSNodeType::ScriptCall, "ScriptCall");
            if (ImGui::MenuItem("Add Bool Constant Node"))  addNode(VSNodeType::BoolConstant, "BoolConstant");
            if (ImGui::MenuItem("Add Int Constant Node"))   addNode(VSNodeType::IntConstant, "IntConstant");
            if (ImGui::MenuItem("Add Float Constant Node")) addNode(VSNodeType::FloatConstant, "FloatConstant");
            if (ImGui::MenuItem("Add Add Node"))            addNode(VSNodeType::Add, "Add");
            if (ImGui::MenuItem("Add Subtract Node"))       addNode(VSNodeType::Subtract, "Subtract");
            if (ImGui::MenuItem("Add Multiply Node"))       addNode(VSNodeType::Multiply, "Multiply");
            if (ImGui::MenuItem("Add Divide Node"))         addNode(VSNodeType::Divide, "Divide");
            if (ImGui::MenuItem("Add Equal Node"))          addNode(VSNodeType::Equal, "Equal");
            if (ImGui::MenuItem("Add Not Equal Node"))      addNode(VSNodeType::NotEqual, "NotEqual");
            if (ImGui::MenuItem("Add Greater Than Node"))   addNode(VSNodeType::GreaterThan, "GreaterThan");
            if (ImGui::MenuItem("Add Less Than Node"))      addNode(VSNodeType::LessThan, "LessThan");
            if (ImGui::MenuItem("Add Greater Equal Node"))  addNode(VSNodeType::GreaterEqual, "GreaterEqual");
            if (ImGui::MenuItem("Add Less Equal Node"))     addNode(VSNodeType::LessEqual, "LessEqual");
            if (ImGui::MenuItem("Add And Node"))            addNode(VSNodeType::And, "And");
            if (ImGui::MenuItem("Add Or Node"))             addNode(VSNodeType::Or, "Or");
            if (ImGui::MenuItem("Add Not Node"))            addNode(VSNodeType::Not, "Not");

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("VSNodeContextPopup"))
        {
            VSNode* contextNode = findNodeById(contextNodeId);

            if (contextNode)
            {
                if (ImGui::MenuItem("Delete Node"))
                {
                    deleteNodeById(contextNodeId);
                    contextNodeId = -1;
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::MenuItem("Duplicate Node"))
                {
                    VSNode copy = *contextNode;
                    copy.id = nextNodeId++;
                    copy.pos.x += 40.0f;
                    copy.pos.y += 40.0f;
                    UndoManager::GetInstance().AddCommand(
                        std::make_unique<CreateVSNodeCommand>(graph, copy)
                    );

                    contextNodeId = copy.id;
                    selectedNode = copy.id;
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::MenuItem("Rename To Type"))
                {
                    contextNode->title = getTypeName(contextNode->type);
                    std::snprintf(nodeTitleBuffer, sizeof(nodeTitleBuffer), "%s", contextNode->title.c_str());
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

        if (mouseClicked && canvasHovered)
        {
            selectedNode = -1;
            draggingNode = -1;

            bool pinHandled = false;
            for (int i = (int)nodes.size() - 1; i >= 0; --i)
            {
                for (int outIndex = 0; outIndex < (int)nodes[i].outputs.size(); ++outIndex)
                {
                    ImVec2 outPin = getOutputPinPos(nodes[i], outIndex);
                    if (isMouseNearPoint(outPin, 8.0f))
                    {
                        creatingLink = true;
                        linkStartNode = nodes[i].id;
                        linkStartPin = outIndex;
                        pinHandled = true;
                        break;
                    }
                }
                if (pinHandled) break;
            }

            if (!pinHandled)
            {
                for (int i = (int)nodes.size() - 1; i >= 0; --i)
                {
                    ImVec2 minP = getNodeRectMin(nodes[i]);
                    ImVec2 maxP = getNodeRectMax(nodes[i]);

                    if (mousePos.x >= minP.x && mousePos.x <= maxP.x &&
                        mousePos.y >= minP.y && mousePos.y <= maxP.y)
                    {
                        selectedNode = nodes[i].id;
                        draggingNode = nodes[i].id;
                        dragOffset = ImVec2(mousePos.x - minP.x, mousePos.y - minP.y);
                        dragStartPos = nodes[i].pos;
                        break;
                    }
                }
            }
        }

        if (mouseDown && draggingNode != -1 && !creatingLink)
        {
            VSNode* node = findNodeById(draggingNode);
            if (node)
            {
                node->pos = ImVec2(
                    mousePos.x - canvasPos.x - dragOffset.x,
                    mousePos.y - canvasPos.y - dragOffset.y
                );
            }
        }

        if (mouseReleased)
        {
            if (creatingLink && linkStartNode != -1)
            {
                for (const auto& node : nodes)
                {
                    if (node.id == linkStartNode) continue;

                    for (int pinIndex = 0; pinIndex < (int)node.inputs.size(); ++pinIndex)
                    {
                        ImVec2 inPin = getInputPinPos(node, pinIndex);
                        if (isMouseNearPoint(inPin, 8.0f))
                        {
                            bool alreadyExists = false;
                            for (const auto& link : links)
                            {
                                if (link.fromNode == linkStartNode &&
                                    link.fromPin == linkStartPin &&
                                    link.toNode == node.id &&
                                    link.toPin == pinIndex)
                                {
                                    alreadyExists = true;
                                    break;
                                }
                            }

                            if (!alreadyExists)
                            {
                                VSLink link;
                                link.fromNode = linkStartNode;
                                link.fromPin = linkStartPin;
                                link.toNode = node.id;
                                link.toPin = pinIndex;

                                UndoManager::GetInstance().AddCommand(
                                    std::make_unique<CreateVSLinkCommand>(graph, link)
                                );
                            }
                            goto done_linking;
                        }
                    }
                }
            }

        done_linking:
            if (draggingNode != -1)
            {
                VSNode* node = findNodeById(draggingNode);
                if (node)
                {
                    ImVec2 dragEndPos = node->pos;

                    if (dragEndPos.x != dragStartPos.x || dragEndPos.y != dragStartPos.y)
                    {
                        node->pos = dragStartPos;

                        UndoManager::GetInstance().AddCommand(
                            std::make_unique<MoveVSNodeCommand>(graph, draggingNode, dragStartPos, dragEndPos)
                        );
                    }
                }
            }
            linkStartPin = 0;
            draggingNode = -1;
            creatingLink = false;
            linkStartNode = -1;
        }

        if (selectedNode != -1 && selectedNode != inspectorNodeId)
        {
            inspectorNodeId = selectedNode;

            VSNode* selected = findNodeById(selectedNode);
            if (selected)
                std::snprintf(nodeTitleBuffer, sizeof(nodeTitleBuffer), "%s", selected->title.c_str());
        }
        else if (selectedNode == -1)
        {
            inspectorNodeId = -1;
            nodeTitleBuffer[0] = '\0';
        }

        // I draw Bezier curves between connected pins for visual clarity
        for (const auto& link : links)
        {
            VSNode* from = findNodeById(link.fromNode);
            VSNode* to = findNodeById(link.toNode);
            if (!from || !to) continue;

            ImVec2 p1 = getOutputPinPos(*from, link.fromPin);
            ImVec2 p2 = getInputPinPos(*to, link.toPin);

            draw->AddBezierCubic(
                p1,
                ImVec2(p1.x + 80.0f, p1.y),
                ImVec2(p2.x - 80.0f, p2.y),
                p2,
                IM_COL32(255, 220, 100, 255),
                3.0f
            );
        }

        // Draw temporary line while dragging a new link
        if (creatingLink && linkStartNode != -1)
        {
            VSNode* from = findNodeById(linkStartNode);
            if (from)
            {
                ImVec2 p1 = getOutputPinPos(*from, linkStartPin);
                ImVec2 p2 = mousePos;

                draw->AddBezierCubic(
                    p1,
                    ImVec2(p1.x + 80.0f, p1.y),
                    ImVec2(p2.x - 80.0f, p2.y),
                    p2,
                    IM_COL32(180, 255, 180, 255),
                    2.5f
                );
            }
        }

        for (const auto& node : nodes)
        {
            ImVec2 minP = getNodeRectMin(node);
            ImVec2 maxP = getNodeRectMax(node);

            bool isSelected = (selectedNode == node.id);

            ImU32 fillColor = isSelected
                ? IM_COL32(95, 120, 190, 255)
                : IM_COL32(70, 90, 140, 255);

            draw->AddRectFilled(minP, maxP, fillColor, 8.0f);
            draw->AddRect(minP, maxP, IM_COL32(255, 255, 255, 180), 8.0f);
            draw->AddText(ImVec2(minP.x + 16.0f, minP.y + 20.0f), IM_COL32(255, 255, 255, 255), node.title.c_str());
            draw->AddText(ImVec2(minP.x + 16.0f, minP.y + 40.0f), IM_COL32(200, 200, 220, 255), getTypeName(node.type));

            for (int pinIndex = 0; pinIndex < (int)node.inputs.size(); ++pinIndex)
            {
                ImVec2 inPin = getInputPinPos(node, pinIndex);
                draw->AddCircleFilled(inPin, 6.0f, IM_COL32(180, 220, 255, 255));

                draw->AddText(
                    ImVec2(inPin.x + 10.0f, inPin.y - 8.0f),
                    IM_COL32(220, 220, 220, 255),
                    node.inputs[pinIndex].name.c_str()
                );
            }

            for (int outIndex = 0; outIndex < (int)node.outputs.size(); ++outIndex)
            {
                ImVec2 outPin = getOutputPinPos(node, outIndex);
                draw->AddCircleFilled(outPin, 6.0f, IM_COL32(255, 210, 100, 255));

                draw->AddText(
                    ImVec2(outPin.x - 70.0f, outPin.y - 8.0f),
                    IM_COL32(220, 220, 220, 255),
                    node.outputs[outIndex].name.c_str()
                );
            }
        }

        if (selectedNode != -1)
        {
            if (ImGui::Button("Delete Selected Node"))
            {
                deleteNodeById(selectedNode);
            }
        }

        bool deletePressed =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::IsKeyPressed(ImGuiKey_Delete, false) &&
            !ImGui::GetIO().WantTextInput &&
            !ImGui::IsAnyItemActive();

        if (selectedNode != -1 && deletePressed)
        {
            deleteNodeById(selectedNode);
        }

        ImGui::Separator();
        ImGui::Text("Selected Node Inspector");

        if (selectedNode != -1)
        {
            VSNode* selected = findNodeById(selectedNode);
            if (selected)
            {
                VSNode beforeEdit = *selected;
                bool changed = false;

                ImGui::Text("Node ID: %d", selected->id);

                int nodeType = static_cast<int>(selected->type);
                const char* typeNames[] = {
                    "Start",
                    "Move",
                    "Attack",
                    "Wait",
                    "Branch",
                    "ScriptCall",
                    "BoolConstant",
                    "IntConstant",
                    "FloatConstant",
                    "GetVariable",
                    "SetVariable",
                    "Add",
                    "Subtract",
                    "Multiply",
                    "Divide",
                    "Equal",
                    "NotEqual",
                    "GreaterThan",
                    "LessThan",
                    "GreaterEqual",
                    "LessEqual",
                    "And",
                    "Or",
                    "Not"
                };

                if (ImGui::Combo("Type", &nodeType, typeNames, IM_ARRAYSIZE(typeNames)))
                {
                    selected->type = static_cast<VSNodeType>(nodeType);
                    rebuildNodePins(*selected);
                    selected->title = getTypeName(selected->type);
                    std::snprintf(nodeTitleBuffer, sizeof(nodeTitleBuffer), "%s", selected->title.c_str());
                    changed = true;
                }

                if (selected->type == VSNodeType::ScriptCall)
                {
                    ImGui::PushID(selected->id);

                    char scriptBuffer[128];
                    char functionBuffer[128];

                    std::snprintf(scriptBuffer, sizeof(scriptBuffer), "%s", selected->scriptName.c_str());
                    std::snprintf(functionBuffer, sizeof(functionBuffer), "%s", selected->functionName.c_str());

                    if (ImGui::InputText("Script Name", scriptBuffer, sizeof(scriptBuffer)))
                    {
                        selected->scriptName = scriptBuffer;
                        changed = true;
                    }

                    if (ImGui::InputText("Function Name", functionBuffer, sizeof(functionBuffer)))
                    {
                        selected->functionName = functionBuffer;
                        changed = true;
                    }

                    ImGui::PopID();
                }

                if (selected->type == VSNodeType::BoolConstant)
                {
                    if (ImGui::Checkbox("Bool Value", &selected->boolValue))
                    {
                        changed = true;

                        auto* ecs = CrashoutsEngine::ECS::getInstance();
                        UI::UIState* uiState = UI::GetUIState();

                        if (ecs && uiState)
                        {
                            auto& arr = ecs->GetComponentArray<Component::VisualScriptComponent>();

                            for (size_t i = 0; i < arr.dense.size(); ++i)
                            {
                                auto* vs = arr.dense[i];
                                if (!vs) continue;

                                if (vs->graphPath == uiState->openedVisualScriptPath && vs->graph)
                                {
                                    for (auto& runtimeNode : vs->graph->nodes)
                                    {
                                        if (runtimeNode.id == selected->id &&
                                            runtimeNode.type == UI::VSNodeType::BoolConstant)
                                        {
                                            runtimeNode.boolValue = selected->boolValue;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (selected->type == VSNodeType::IntConstant)
                {
                    if (ImGui::DragInt("Int Value", &selected->intValue, 1.0f))
                    {
                        changed = true;
                    }
                }

                if (selected->type == VSNodeType::FloatConstant)
                {
                    if (ImGui::DragFloat("Float Value", &selected->floatValue, 0.1f))
                    {
                        changed = true;
                    }
                }

                if (selected->type == VSNodeType::Wait)
                {
                    if (ImGui::DragFloat("Wait Time", &selected->waitTime, 0.1f, 0.0f, 100.0f))
                    {
                        changed = true;
                    }
                }

                ImGui::Indent(20.0f);
                if (ImGui::InputText("Title", nodeTitleBuffer, sizeof(nodeTitleBuffer)))
                {
                    selected->title = nodeTitleBuffer;
                    changed = true;
                }
                ImGui::Unindent(20.0f);

                if (ImGui::DragFloat2("Position", &selected->pos.x, 1.0f))
                {
                    changed = true;
                }

                ImGui::Text("Size: %.1f x %.1f", selected->size.x, selected->size.y);

                if (changed)
                {
                    VSNode afterEdit = *selected;

                    *selected = beforeEdit;

                    UndoManager::GetInstance().AddCommand(
                        std::make_unique<EditVSNodeCommand>(graph, selected->id, beforeEdit, afterEdit)
                    );
                }
            }
        }
        else
        {
            ImGui::TextDisabled("No node selected.");
        }
    }
}