/*!
@file       debugmenu.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

@brief      Renders the editor/debug Im

for Crashout Engine.
@details    This module draws every editor-facing window:
            - engine controls (play / pause / stop)
            - game viewport (with ImGuizmo)
            - ECS / Inspector panels
            - Asset browser / drag-and-drop
            - Performance / profiler view
            - Audio test panel

            It also contains all the small, file-local editor state
            (selected entity, drag payloads, gizmo mode, etc.)
            so that the rest of the engine code stays clean.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Engine / app headers
#include "glapp.h"
#include "rendering.h"
#include "performanceViewer.h"
#include "camera.h"
#include "entitymanager.h"
#include "collisionmanager.h"
#include "physicsmanager.h"
#include "gamestatemanager.h"
#include "assetmanager.h"
#include "textureasset.h"
#include "shapes.h"
#include "log.h"
#include "logdefs.h"
#include "debugmenu.h"
#include "debugreflection.h"
#include "debugcollision.h"
#include "frameratecontroller.h"
#include "audioSystem.h"
#include "prefab_init.h"  
#include <fmod_studio.hpp> 
#include "prefab.h"
#include "hierarchy.h"
#include "hierarchy_components.h"
#include "monomanager.h"
#include "scriptsystem.h"
#include "ppmanager.h"
#include "keyframe.h"

#include "engine.h"
#include <filesystem>
#include <iostream>
#include <fstream>

#include "changevaluecommand.h"
#include "undomanager.h"
#include "inputmanager.h"
#include "transformentitycommand.h"
#include "namecomponent.h"
#include "shapescomponent.h"
#include "audiocomponent.h"
#include "sceneasset.h"
#include <vector>
#include "CEVec3.h"
#include "stb_image.h"
#include <numeric>

#include "settings.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"

using CrashoutsEngine::Audio::AudioSystem;

// ImGui headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>


/*
----------------------------------------------------------------------------- */

using namespace CrashoutsEngine;
using namespace Component;
using CrashoutsEngine::Audio::AudioSystem;
using namespace KeyframeUnity;

namespace { //----------------------------------------------------------------------------- file-local state & helpers

    // ============================================================
    //  Performance scratch (fed by PerformanceSystem in engine.cpp)
    // ============================================================
    std::vector<ProfilerSample> gPerfSnapshot;  //!< Performance profiler snapshot
    double gPerfSnapshotTotalMs = 0.0;          //!< Total frame time in ms

    namespace fs = std::filesystem;

    // ============================================================
    //  Saving world Pos (editor-only)
    // ============================================================
    struct SavedChild {
        Entity e;
        CEVec3 worldPos;
    };

    static bool  s_gizmoRotating = false;
    static float s_gizmoStartDeg = 0.0f;

    // ============================================================
    //  Audio panel scratch (editor-only)
    // ============================================================
    static bool s_layoutBuilt = false;


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

    // ============================================================
    //  Drag-and-drop payload description for asset browser
    // ============================================================
    enum class AssetKind : uint8_t { Texture, SpriteSheet, Shader, Audio, Scene, Prefab, Unknown };
    struct AssetDragPayload {
        AssetKind kind{ AssetKind::Unknown };
        char name[128]{};     // manifest/manager "name" (stem)
        char relPath[256]{};  // relative path under assets/ (optional)
    };

    static std::vector<std::string> gDroppedPaths;

    // ============================================================
    //  Viewport / gizmo shared state
    // ============================================================
    static ImGuizmo::OPERATION g_gizmoOp = ImGuizmo::TRANSLATE; // T/R/S
    static ImGuizmo::MODE      g_gizmoMode = ImGuizmo::LOCAL;    // LOCAL or WORLD
    static bool                g_snapOn = false;
    static float               g_snapTrans[3] = { 1.0f, 1.0f, 1.0f }; // snap grid for move/scale
    static float               g_snapAngle = 15.0f;

    // using things from renderer.cpp
    static inline CEVec3 ViewUV_toWorld(float u, float v) {

        const float ndcX = u * 2.f - 1.f;
        const float ndcY = v * 2.f - 1.f;
        const glm::mat4 VP = Component::Rendering::Get().projectionMatrix * Component::Rendering::Get().viewMatrix;
        const glm::mat4 invVP = glm::inverse(VP);
        glm::vec4 w = invVP * glm::vec4(ndcX, ndcY, 0.f, 1.f);
        if (w.w != 0.f) w /= w.w;
        return CEVec3{ (float)w.x, (float)w.y, (float)w.z };
    }

    static inline bool PointInAABB(const CEVec3& p, const Collision::AABBRect& a) {
        return p.x >= a.min.x && p.x <= a.max.x && p.y >= a.min.y && p.y <= a.max.y;
    }

    static void DrawHierarchyNode(ECS& ecs, Entity e, Entity& selected)
    {
        std::string label = ecs.HasComponent<NameComponent>(e)
            ? ecs.GetComponent<NameComponent>(e)->name
            : ("Entity " + std::to_string(e));

        const bool hasKids = ecs.HasComponent<ChildrenComponent>(e)
            && !ecs.GetComponent<ChildrenComponent>(e)->children.empty();

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanFullWidth |
            (selected == e ? ImGuiTreeNodeFlags_Selected : 0) |
            (!hasKids ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0);

        ImGui::PushID((int)e);
        const bool opened = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            selected = e;

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
                if (!ecs.HasComponent<NameComponent>(child))
                    ecs.AddComponent<NameComponent>(child, "New Child");
                if (!ecs.HasComponent<TransformComponent>(child))
                    ecs.AddComponent<TransformComponent>(child);
                CrashoutsEngine::AttachChild(ecs, e, child);
            }
            ImGui::EndPopup();

        }

        if (hasKids && opened) {
            auto* kids = ecs.GetComponent<ChildrenComponent>(e);
            for (Entity c : kids->children)
                DrawHierarchyNode(ecs, c, selected);
            ImGui::TreePop();
        }

        ImGui::PopID();
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
        SetDarkGrayTheme();
        ScopedMemTag boot(MemTag::Resources);
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        glfwSetDropCallback(window, [](GLFWwindow*, int count, const char** paths) {
            for (int i = 0; i < count; ++i) {
                if (paths[i]) gDroppedPaths.emplace_back(paths[i]);
            }
        });

        LOGS(Core, "Engine initialized successfully");
        LOGW(Core, "Debug mode is ON");
        LOGI(Core, "Welcome to Crashout Engine Debugger!");
    }

    /*!
     @brief Shuts down ImGui and cleans up resources.
    */
    void Shutdown() {

        if (!currSceneName.empty()) Serialization::SaveScenes(currSceneName);

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
    // ----------------- Keyframe editor helpers -----------------
    namespace KeyframeUnityEditor
    {
        struct Selection {
            CrashoutsEngine::Entity entity{};
            int bindingIndex = -1;
            int keyIndex = -1;
        };

        static Selection gSel;

        static const char* KFPropertyName(KFProperty p)
        {
            switch (p) {
            case KFProperty::PosX:        return "Position X";
            case KFProperty::PosY:        return "Position Y";
            case KFProperty::RotZ:        return "Rotation Z";
            case KFProperty::ScaleX:      return "Scale X";
            case KFProperty::ScaleY:      return "Scale Y";
            case KFProperty::SpriteIndex: return "Sprite Index";
            default:                      return "Unknown";
            }
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
    static void RenderEcsPanel(ECS& ecs, std::vector<Entity>& entities, Entity& selected, UIState& ui)
    {
        static std::vector<Entity> multiSel;   // persistent multi-selection in the panel

        ScopedWindow win("Hierarchy", LockFlags(ui.lockLayout));

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
        // --- Unity-style tree: draw roots (entities with no ParentComponent) ---
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


        ImGui::Separator();
        ImGui::Text("Selected Entity: %u", (unsigned)selected);

        if (!selected) return;
       
        static char prefabNameBuf[64] = "MyBox";
        ImGui::SetNextItemWidth(220);
        ImGui::InputTextWithHint("##save_prefab_name", "Prefab name...", prefabNameBuf, IM_ARRAYSIZE(prefabNameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Save Prefab")) {
            namespace fs = std::filesystem;

            
            const fs::path jsonPath = fs::path(CrashoutsEngine::Assets::rootPath) / "prefabs" / "prefab.json";
            fs::create_directories(jsonPath.parent_path()); // safe if exists

            
            const std::string newName = (prefabNameBuf[0] ? prefabNameBuf : "Entity");

            const bool ok = SaveBoxPrefabToJson_RJ(
                ecs, selected,
                newName,
                jsonPath.generic_string(),    
                /*matchName*/ newName        
            );

            if (ok) {
               
                CrashoutsEngine::ReloadPrefabsFromDisk(jsonPath.generic_string());

                
                LOGI(Core, std::string("Saved & reloaded prefab json: ") + jsonPath.generic_string());
                prefabNameBuf[0] = '\0';
            }
        }

        ImGui::Separator();


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
        }
    }
    // Draw a Unity-like keyframe timeline for the selected entity
    static void RenderAnimationPanel(CrashoutsEngine::ECS& ecs,
        CrashoutsEngine::Entity selected,
        UIState& ui)
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

            // Property label
            ImGui::SetCursorScreenPos(ImVec2(origin.x, rowTop));
            ImGui::TextUnformatted(KFPropertyName(b.property));

            // Row background
            ImU32 rowCol = IM_COL32(40, 40, 40, 255);
            draw->AddRectFilled(
                ImVec2(origin.x + labelW, rowTop),
                ImVec2(origin.x + labelW + timeW, rowTop + rowH),
                rowCol);

            // Keyframes 
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

                // Clickable area
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

    static void UI::RenderAssetBrowserPanel(UIState& ui, const char* root, std::string& outSelectedAsset)
    {
        ScopedWindow win("Assets", LockFlags(ui.lockLayout));

        static char filter[128] = { 0 };
        static fs::path current = root ? fs::path(root) : fs::path(".");
        static std::vector<fs::directory_entry> entries;
        static bool needScan = true;

        // Top bar controls
        if (ImGui::Button("Up") && current.has_parent_path()) {
            current = current.parent_path();
            needScan = true;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(current.generic_string().c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220);
        ImGui::InputTextWithHint("##asset_filter", "Filter (name / ext)...", filter, IM_ARRAYSIZE(filter));

        ImGui::SameLine();
        // ImGUI button to reload assets
        if (ImGui::Button("Reload Assets")) {
            using namespace CrashoutsEngine::Assets;
            GenerateAssetManifest();
            AssetManager::GetInstance().LoadManifestAssets();
        }

        ImGui::Separator();

        if (!gDroppedPaths.empty()) {
            namespace fs = std::filesystem;

            for (const std::string& absPath : gDroppedPaths) {
                std::error_code ec;
                fs::path src(absPath);
                if (!fs::is_regular_file(src, ec)) continue;

                std::string ext = src.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                bool allowed =
                    ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" ||
                    ext == ".json" || ext == ".scene" || ext == ".bank" || ext == ".wav" || ext == ".ogg" ||
                    ext == ".vert" || ext == ".frag";
                if (!allowed) continue;

                fs::path dst = current / src.filename();
                if (fs::exists(dst, ec)) {
                    for (int i = 1; i < 10000; ++i) {
                        fs::path candidate =
                            dst.parent_path() /
                            (dst.stem().string() + " (" + std::to_string(i) + ")" + dst.extension().string());
                        if (!fs::exists(candidate, ec)) { dst = candidate; break; }
                    }
                }

                fs::create_directories(dst.parent_path(), ec);
                fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
            }

            gDroppedPaths.clear();
            needScan = true;

            // Remind me to ask if I can implement something in assetManager, so I can do efficient loading
            using namespace CrashoutsEngine::Assets;
            GenerateAssetManifest();
            AssetManager::GetInstance().LoadManifestAssets();
        }

        // LEFT: Folder tree
        ImGui::BeginChild("asset_left", ImVec2(220, 0), true);

        if (ImGui::Selectable("assets", current == fs::path(root))) {
            current = fs::path(root);
            needScan = true;
        }

        try {
            for (auto& de : fs::directory_iterator(root)) {
                if (!de.is_directory()) continue;
                bool isCur = (current == de.path());
                if (ImGui::Selectable(de.path().filename().generic_string().c_str(), isCur)) {
                    current = de.path();
                    needScan = true;
                }
            }
        }
        catch (...) {}

        ImGui::EndChild();

        // RIGHT: Files + Preview
        ImGui::SameLine();
        ImGui::BeginChild("asset_right", ImVec2(0, 0), true);

        const float kPreviewMaxHeight = 260.0f; 
        ImVec2 availRight = ImGui::GetContentRegionAvail();
        float listHeight = availRight.y - kPreviewMaxHeight - ImGui::GetStyle().ItemSpacing.y;
        if (listHeight < 120.0f) listHeight = availRight.y * 0.6f;

        // FILE LIST 
        ImGui::BeginChild("asset_right_list", ImVec2(0, listHeight), true);

        if (needScan) {
            entries.clear();
            try {
                for (auto& e : fs::directory_iterator(current))
                    entries.push_back(e);

                std::sort(entries.begin(), entries.end(), [](auto const& a, auto const& b) {
                    if (a.is_directory() != b.is_directory()) return a.is_directory() && !b.is_directory();
                    return a.path().filename().generic_string() < b.path().filename().generic_string();
                    });
            }
            catch (...) {}
            needScan = false;
        }

        // Build filtered list of indices
        static std::vector<int> vis;
        vis.clear();
        vis.reserve(entries.size());
        for (int i = 0; i < (int)entries.size(); ++i) {
            const auto& e = entries[i];
            std::string name = e.path().filename().generic_string();
            if (filter[0] && name.find(filter) == std::string::npos) continue;
            vis.push_back(i);
        }

        ImGuiListClipper clip;
        clip.Begin((int)vis.size());
        while (clip.Step()) {
            for (int row = clip.DisplayStart; row < clip.DisplayEnd; ++row) {
                const auto& e = entries[vis[row]];
                std::string name = e.path().filename().generic_string();

                if (e.is_directory()) {
                    if (ImGui::Selectable((std::string("[Dir] ") + name).c_str(), false)) {
                        current = e.path(); needScan = true;
                    }
                }
                else {
                    bool sel = (outSelectedAsset == e.path().generic_string());
                    if (ImGui::Selectable(name.c_str(), sel))
                        outSelectedAsset = e.path().generic_string();

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        AssetDragPayload p{};
                        std::string rel = fs::relative(e.path(), fs::path(root)).generic_string();
                        std::string stem = e.path().stem().generic_string();
                        std::string ext = e.path().extension().generic_string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        // Heuristics to pick kind by folder/extension
                        if (rel.rfind("images/", 0) == 0 ||
                            ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                            p.kind = AssetKind::Texture;
                        }
                        else if (rel.rfind("sprite_sheet/", 0) == 0) {
                            p.kind = AssetKind::SpriteSheet;
                        }
                        else if (rel.rfind("scenes/", 0) == 0 || ext == ".scene" || ext == ".json") {
                            p.kind = AssetKind::Scene;
                        }
                        else if (rel.rfind("audio/", 0) == 0 || ext == ".bank" || ext == ".ogg" || ext == ".wav" || ext == ".mp3") {
                            p.kind = AssetKind::Audio;
                        }
                        else if (rel.rfind("prefabs/", 0) == 0) {
                            p.kind = AssetKind::Prefab;
                        }
                        else if (rel.rfind("shaders/", 0) == 0 || ext == ".vert" || ext == ".frag") {
                            p.kind = AssetKind::Shader;
                        }
                        else {
                            p.kind = AssetKind::Unknown;
                        }

                        std::strncpy(p.name, stem.c_str(), sizeof(p.name) - 1);
                        std::strncpy(p.relPath, rel.c_str(), sizeof(p.relPath) - 1);

                        ImGui::SetDragDropPayload("ASSET/PAYLOAD", &p, sizeof(p));
                        ImGui::TextUnformatted(p.name);
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        outSelectedAsset = e.path().generic_string();
                }
            }
        }

        ImGui::EndChild(); // asset_right_list

        // PREVIEW 
        ImGui::Separator();
        ImGui::BeginChild("asset_right_preview", ImVec2(0, 0), true);

        if (!outSelectedAsset.empty()) {
            std::string ext = fs::path(outSelectedAsset).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                static GLuint previewTex = 0;
                static std::string loadedPath;
                static int imgW = 0, imgH = 0;

                // Load texture when selection changes
                if (loadedPath != outSelectedAsset) {
                    if (previewTex) {
                        glDeleteTextures(1, &previewTex);
                        previewTex = 0;
                    }

                    int w, h, channels;
                    unsigned char* data = stbi_load(outSelectedAsset.c_str(), &w, &h, &channels, 4);
                    if (data) {
                        const int MAX_DIM = 4096;
                        if (w > MAX_DIM || h > MAX_DIM) {
                            std::cout << "FAILED: oversize preview (" << w << "x" << h << ") -> " << outSelectedAsset << std::endl;
                            stbi_image_free(data);
                            data = nullptr;
                        }
                    }
                    if (data) {
                        glGenTextures(1, &previewTex);
                        glBindTexture(GL_TEXTURE_2D, previewTex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        stbi_image_free(data);
                        loadedPath = outSelectedAsset;
                        imgW = w; imgH = h;
                    }
                }

                if (previewTex && imgW > 0 && imgH > 0) {
                    ImGui::TextUnformatted("Preview:");

                    ImVec2 area = ImGui::GetContentRegionAvail();
                    area.y = std::min(area.y, kPreviewMaxHeight);

                    // Keep aspect ratio
                    float maxW = area.x;
                    float maxH = area.y;
                    float scale = std::min(maxW / float(imgW), maxH / float(imgH));
                    if (scale <= 0.0f) scale = 1.0f;
                    ImVec2 drawSize(float(imgW) * scale, float(imgH) * scale);

                    // Center horizontally
                    float xPad = (area.x - drawSize.x) * 0.5f;
                    if (xPad > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPad);

                    ImGui::Image((ImTextureID)(intptr_t)previewTex, drawSize);
                }
            }
        }

        ImGui::EndChild(); // asset_right_preview
        ImGui::EndChild(); // asset_right
    }

    /*!
    @brief Renders the Inspector debug panel.
    @param ecs ECS reference.
    @param selected Currently selected entity.
    @param ui UI state toggles.
    */
    static void UI::RenderInspectorPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui)
    {
        if (!ui.showInspector) return;
        ScopedWindow w("Inspector", LockFlags(ui.lockLayout));

        if (!selected) {
            ImGui::TextDisabled("No entity selected.");
            return;
        }

        ImGui::Text("Selected Entity: %u", (unsigned)selected);
        ImGui::SeparatorText("Add Components");

     
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
        if (!ecs.HasComponent<KeyframeAnimatorComponent>(selected)) {
            if (ImGui::Button("Add Animation (KeyframeAnimator)")) {

                // 1) Add the component
                auto& anim = AddAnimator(ecs, selected);

                // =========================================================
                // WALK CLIP  (loops 4 frames on a spritesheet)
                // =========================================================
                AnimationClip walk{};
                walk.name = "Walk";
                walk.wrap = KFWrap::Loop;      // keep looping

                KFBinding walkSprite{};
                walkSprite.property = KFProperty::SpriteIndex;

                // 4 frames: 0,1,2,3 over 0.4s (0.1s each)
                {
                    KFKey k{};
                    k.interp = KFInterp::Constant;   // sprites: no interpolation

                    k.time = 0.0f; k.value = 0.f;
                    walkSprite.curve.Add(k);

                    k.time = 0.1f; k.value = 1.f;
                    walkSprite.curve.Add(k);

                    k.time = 0.2f; k.value = 2.f;
                    walkSprite.curve.Add(k);

                    k.time = 0.3f; k.value = 3.f;
                    walkSprite.curve.Add(k);

                    // Close loop back to frame 0 at 0.4s
                    k.time = 0.4f; k.value = 0.f;
                    walkSprite.curve.Add(k);
                }

                walk.bindings.push_back(walkSprite);

                // =========================================================
                // JUMP CLIP  (uses different frames 4–7)
                // =========================================================
                AnimationClip jump{};
                jump.name = "Jump";
                jump.wrap = KFWrap::Clamp;     // play once then hold last frame

                KFBinding jumpSprite{};
                jumpSprite.property = KFProperty::SpriteIndex;

                {
                    KFKey k{};
                    k.interp = KFInterp::Constant;

                    // start stand frame
                    k.time = 0.0f; k.value = 4.f;   // e.g. idle-before-jump
                    jumpSprite.curve.Add(k);

                    // in-air frame
                    k.time = 0.15f; k.value = 5.f;
                    jumpSprite.curve.Add(k);

                    // peak
                    k.time = 0.30f; k.value = 6.f;
                    jumpSprite.curve.Add(k);

                    // landing frame
                    k.time = 0.45f; k.value = 7.f;
                    jumpSprite.curve.Add(k);
                }

                jump.bindings.push_back(jumpSprite);

                // =========================================================
                // Register both clips on this animator
                // =========================================================
                anim.clips[walk.name] = walk;
                anim.clips[jump.name] = jump;

                // Start in WALK
                anim.current = walk.name;
                anim.t = 0.f;
                anim.playing = true;
            }
        }
        ImGui::NewLine();

        ImGui::SeparatorText("Components on Selected Entity");
        // ---- Manual inspector for KeyframeAnimatorComponent ----
        if (ecs.HasComponent<KeyframeAnimatorComponent>(selected)) {
            auto* anim = ecs.GetComponent<KeyframeAnimatorComponent>(selected);

            ImGui::SeparatorText("Animation");

            if (anim->current.empty()) {
                ImGui::TextDisabled("No clip set. Use the Add Animation button or create clips in code.");
            }
            else {
                ImGui::Text("Current clip: %s", anim->current.c_str());
            }

            ImGui::DragFloat("Time", &anim->t, 0.01f, 0.0f);

            // Play / Pause / Stop controls
            if (ImGui::Button(anim->playing ? "Pause" : "Play")) {
                anim->playing = !anim->playing;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                anim->playing = false;
                anim->t = 0.f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Restart")) {
                anim->t = 0.f;
                anim->playing = true;
            }
        }

        
        for (auto* ad : DebugReflection::ComponentRegistry::instance().all()) {
            if (!ad->has(ecs, selected)) continue;

            ImGui::PushID(ad);

            ImGui::Separator();
            ImGui::TextUnformatted(ad->name());

            // Right-align "Remove"
            float remove_w = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float x = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - remove_w;
            ImGui::SameLine(x);
            if (ImGui::SmallButton("Remove")) {
                ad->remove(ecs, selected);
                ImGui::PopID();
                break; 
            }

            
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
            ad->drawInspector(ecs, selected);
            ImGui::PopStyleVar();

            ImGui::PopID();
        }
    }

    /*!
     @brief Renders the performance panel with profiler results.
     @param profiler Frame profiler reference.
     @param ui UI state toggles.
    */
    static void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui) {
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
    static void RenderConsolePanel(UIState& ui) {
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

        // Log list (virtualized)
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

        ImGuiListClipper clipper;
        clipper.Begin((int)filtered.size());
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const auto& log = logs[filtered[row]];
                // Color by level
                const ImVec4 color =
                    (log.level == LogMessage::Level::Error) ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
                    (log.level == LogMessage::Level::Warning) ? ImVec4(1.0f, 0.9f, 0.4f, 1.0f) :
                    (log.level == LogMessage::Level::Success) ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) :
                    ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, color);

                // Prefix + message (avoid fmt when not needed)
                ImGui::Text("[%s][%s] %s", ToString(log.level), ToString(log.system), log.text.c_str());

                ImGui::PopStyleColor();
            }
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
    static void RenderGameStatePanel(UIState& ui) {
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
        //const int next = P->next() ? P->next() : -1;

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
     @brief Renders the scene panel
     @param ui UI state toggles.
    */
    static void RenderGameScenePanel(UIState& ui) {

        if (!ui.showGameScene) return;

        ScopedWindow w("GameScene", LockFlags(ui.lockLayout));

        ImGui::Separator();

        // 1. Buffer to store the scene name from the text field
        static char sceneNameBuffer[128] = "";

        // 2. Draw the text input field. 
        //    We use "##" to hide the label but keep a unique ID
        ImGui::InputText("##SceneNameInput", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));
        //    Add a hint if the buffer is empty
        if (sceneNameBuffer[0] == '\0') {
            ImGui::SameLine();
            ImGui::TextDisabled("Enter new scene name...");
        }


        // 3. Draw the "Create Scene" button
        ImGui::SameLine();
        if (ImGui::Button("Create Scene")) {
            // Only create if the name field is not empty
            if (std::strlen(sceneNameBuffer) > 0) {

                // 4. Call your create function
                Serialization::SaveScenes(sceneNameBuffer);

                // 5. Clear the buffer after creation for good UX
                sceneNameBuffer[0] = '\0';
            }
        }
        // --- End New UI ---

        ImGui::Separator();
        
        // Draw all scene buttons
        // Check manifest json file
        std::ifstream ifs(Assets::assetManifestPath);
        if (!ifs.is_open()) {
            std::cerr << "Error: Could not open manifest file: " << Assets::assetManifestPath << std::endl;
            return;
        }

        // Convert file to rapidJSON stream interface
        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) {
            std::cerr << "Error parsing manifest JSON." << std::endl;
            return;
        }

        if (doc.HasMember("scenes") && doc["scenes"].IsArray()) {
            //std::cout << "Loading Scenes..." << std::endl;

            const rapidjson::Value& sceneArray = doc["scenes"];

            for (rapidjson::SizeType i = 0; i < sceneArray.Size(); ++i) {
                const rapidjson::Value& asset = sceneArray[i];
                if (asset.HasMember("name") && asset.HasMember("path")) {
                    std::string name = asset["name"].GetString();
                    std::string relativePath = asset["path"].GetString();

                    if (ImGui::Button(name.c_str())) {
                        if (!currSceneName.empty()) {
                            if(engineState == EngineState::STOP) Serialization::SaveScenes(currSceneName);
                        }
                        ECS::getInstance()->Reset();
                        currSceneName = name;
                        Serialization::LoadScenes(name);
                    }

                    ImGui::SameLine();
                }
                else {
                    std::cerr << "Warning: Scene asset entry missing 'name' or 'path' in manifest." << std::endl;
                }
            }
        }
    }

    /*!
     @brief Renders the physics panel, including collision event listings and controls.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param phys Physics handler reference.
     @param ui UI state toggles.
    */
    static void RenderPhysicsPanel([[maybe_unused]] FrameProfiler& profiler, ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui)
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
        ImGui::SameLine(); ImGui::Checkbox("QuadTree", &ui.drawQuadTree);

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
    static void UI::RenderAudioPanel([[maybe_unused]] FrameProfiler& profiler, [[maybe_unused]] CrashoutsEngine::ECS& ecs, UIState& ui) {

        if (!ui.showAudio) return;
        ScopedWindow w("Audio", LockFlags(ui.lockLayout));

        const bool audioInitOk = AudioSystem::instance().isInitialized();
        if (!audioInitOk) {
            ImGui::TextDisabled("FMOD not initialized (AssetManager will do this).");
            if (ImGui::Button("Initialize Audio (assets/audio)")) {
                CrashoutsEngine::Audio::AudioSystem::instance().init("assets/audio");
            }
        }

        // Loading Bank
        static bool needRefresh = true;
        if (ImGui::Button("Load banks from Asset Manager")) {
            CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
            needRefresh = true; // trigger rebuild after loading
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh events")) needRefresh = true;

        // Build List
        static std::vector<std::string> allEvents;
        if (needRefresh || allEvents.empty()) {
            allEvents.clear();
            AudioSystem::instance().listAllEventPaths(allEvents);
            std::sort(allEvents.begin(), allEvents.end());
            allEvents.erase(std::unique(allEvents.begin(), allEvents.end()), allEvents.end());
            needRefresh = false;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%zu events", allEvents.size());

        // Event Combo
        auto EventCombo = [&](const char* label, std::string& sel) {
            if (ImGui::BeginCombo(label, sel.empty() ? "(select?)" : sel.c_str())) {
                static char filterBuf[128] = {};
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
                ImGui::InputTextWithHint("##filter", "type to filter...", filterBuf, sizeof(filterBuf));
                ImGui::Separator();

                const std::string needle = filterBuf;
                for (auto const& e : allEvents) {
                    if (!needle.empty() && e.find(needle) == std::string::npos) continue;
                    const bool chosen = (e == sel);
                    if (ImGui::Selectable(e.c_str(), chosen)) sel = e;
                    if (chosen) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            };

        // Test Section
        ImGui::SeparatorText("Quick Test (1 BGM + 3 SFX)");
        static std::string pickBGM, pickSFX1, pickSFX2, pickSFX3;

        EventCombo("Background Music", pickBGM);
        if (ImGui::Button("Play")) {
            CrashoutsEngine::Audio::AudioSystem::instance().playOneShot(pickBGM);
        }

        EventCombo("SFX 1", pickSFX1);
        if (ImGui::Button("Play##sfx1")) {
            CrashoutsEngine::Audio::AudioSystem::instance().playOneShot(pickSFX1);
        }

        EventCombo("SFX 2", pickSFX2);
        if (ImGui::Button("Play##sfx2")) {
            CrashoutsEngine::Audio::AudioSystem::instance().playOneShot(pickSFX2);
        }

        EventCombo("SFX 3", pickSFX3);
        if (ImGui::Button("Play##sfx3")) {
            CrashoutsEngine::Audio::AudioSystem::instance().playOneShot(pickSFX3);
        }

        ImGui::SeparatorText("Master Volume");

        // Cache slider state
        static float sMasterVol = 1.0f;
        if (ImGui::SliderFloat("Master", &sMasterVol, 0.0f, 1.0f, "%.2f"))
            CrashoutsEngine::Audio::AudioSystem::instance().setBusVolume("bus:/", sMasterVol);

        // Optional mute toggle (reuses same call)
        static bool sMuted = false;
        if (ImGui::Checkbox("Mute All", &sMuted)) {
            if (sMuted)
                CrashoutsEngine::Audio::AudioSystem::instance().setBusVolume("bus:/", 0.0f);
            else
                CrashoutsEngine::Audio::AudioSystem::instance().setBusVolume("bus:/", sMasterVol);
        }

        ImGui::Separator();
        if (ImGui::Button("Stop All"))
            AudioSystem::instance().stopAll(true);

        AudioSystem::instance().update();
    }

    /*!
     @brief Renders the menu bar ontop of screen.
     @param ui UI state toggles.
    */
    void RenderDebugMenuBar(UIState& ui) {
        if (!ImGui::BeginMenuBar()) return;

        if (ImGui::BeginMenu("View")) {
            struct Toggle { const char* label; bool* flag; };
            const Toggle toggles[] = {
                {"ECS", &ui.showECS},
                {"Assets", &ui.showAssets},
                {"Inspector", &ui.showInspector},
                {"Physics", &ui.showPhysics},
                {"Performance", &ui.showPerf},
                {"Console", &ui.showConsole},
                {"Game State", &ui.showGameState},
                {"Game Scene", &ui.showGameScene},
                {"Audio", &ui.showAudio},
                {"Animation", &ui.showAnimation},

            };
            for (auto& t : toggles) ImGui::MenuItem(t.label, nullptr, t.flag);

            ImGui::Separator();
            if (ImGui::MenuItem("Show All")) {
                ui.showECS = ui.showAssets = ui.showInspector = ui.showPhysics =
                    ui.showPerf = ui.showConsole = ui.showGameState = ui.showGameScene = ui.showAudio = ui.showEngineControl = true;
                for (auto const& p : DebugReflection::PanelRegistry::instance().panels())
                    if (p.toggleFlag) *p.toggleFlag = true;
            }
            if (ImGui::MenuItem("Hide All")) {
                ui.showECS = ui.showAssets = ui.showInspector = ui.showPhysics =
                    ui.showPerf = ui.showConsole = ui.showGameState = ui.showGameScene = ui.showAudio = ui.showEngineControl = false;
                for (auto const& p : DebugReflection::PanelRegistry::instance().panels())
                    if (p.toggleFlag) *p.toggleFlag = false;
            }
            if (ImGui::MenuItem("Reset Dock Layout")) {
                s_layoutBuilt = false;   // triggers rebuild next frame
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Lock Layout", nullptr, &ui.lockLayout);
            ImGui::MenuItem("Draw Colliders", nullptr, &ui.drawCollOverlay);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    /*!
     @brief Renders the Scene control bar on top of game viewport
    */
    static void RenderEngineStateBar(UIState& ui) {
        if (!ui.showEngineControl) return;

        ImGuiWindowFlags window_flags = 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("Engine Controls", nullptr, window_flags)) {

            // Play Stop Pause buttons
            // Play
            if (ImGui::Button("PLAY", ImVec2(70, 25))) {
                engineState = EngineState::PLAY;
                CrashoutsEngine::RenderCamera::engineCamera.SetPostiion(CEVec3{ 0.f, 0.f, 0.f });
                CrashoutsEngine::RenderCamera::engineCamera.SetZoom(1.f);
                CrashoutsEngine::RenderCamera::engineCamera.SetTargetAR(16.0f / 9.0f);
                CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(Settings::GetScreenWidth(), Settings::GetScreenHeight());

                if (MonoManager::GetDomain() && MonoManager::GetAssembly()) {
                    CrashoutsEngine::ScriptSystem::InitializeAll( MonoManager::GetDomain(), MonoManager::GetAssembly() );
                }
            }
            ImGui::SameLine();
            // Pause
            if (ImGui::Button("PAUSE", ImVec2(70, 25))) {
                engineState = EngineState::PAUSE;
            }
            ImGui::SameLine();
            // Stop
            if (ImGui::Button("STOP", ImVec2(70, 25))) {
                if(currSceneName.empty()) RestartGameState();
                else {
                    ECS::getInstance()->Reset();
                    Serialization::LoadScenes(currSceneName);
                }
                //MonoManager::ReloadScripts();
                engineState = EngineState::STOP;
            }
            
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

#if USE_IMGUI_EDITOR

    /*!
     @brief  Draws the editor game viewport.
     @details
     Steps performed:
     1. Draw tiny toolbar (Translate / Rotate / Scale / Snap)
     2. Compute letterboxed size to respect target aspect ratio
     3. Draw the framebuffer texture
     4. Set up ImGuizmo to draw on top of the image
     5. If an entity is selected, allow gizmo to edit its transform
     6. Allow clicking in the image to pick an entity
     7. Accept drag-and-drop payloads from the asset browser
    */
    static void RenderGameViewport(ECS& ecs, Entity& selected) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // No padding
        // Begin the ImGUI window, enable Docking
        if (ImGui::Begin("Game Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

            ImVec2 start = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(start.x + 10, start.y + 10));
            ImGui::BeginChild("ViewportToolbar", ImVec2(1000, 32),
                false, ImGuiWindowFlags_NoBackground);

            // 1) toolbar (translate / rotate / scale)
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
            if (ImGui::Button("None")) g_gizmoOp = (ImGuizmo::OPERATION)0;
            ImGui::SameLine();
            if (ImGui::Button("Translate")) g_gizmoOp = ImGuizmo::TRANSLATE;
            ImGui::SameLine();
            if (ImGui::Button("Rotate")) g_gizmoOp = ImGuizmo::ROTATE;
            ImGui::SameLine();
            if (ImGui::Button("Scale")) g_gizmoOp = ImGuizmo::SCALE;
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            bool isLocal = (g_gizmoMode == ImGuizmo::LOCAL);
            if (ImGui::Checkbox("Local", &isLocal)) g_gizmoMode = isLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            ImGui::SameLine();
            ImGui::Checkbox("Snap", &g_snapOn);
            ImGui::PopStyleVar();

            ImGui::Separator();
            ImGui::EndChild();
            ImGui::SetCursorScreenPos(start);

            gameWinHovered = ImGui::IsWindowHovered();

            // Get the size of ImGui window content region
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

            // 2) aspect-ratio / letterbox calculation
            float panelWidth = viewportPanelSize.x;
            float panelHeight = viewportPanelSize.y;
            float panelAspect = panelWidth / panelHeight;

            float targetWidth = panelWidth;
            float targetHeight = panelHeight;
            float paddingX = 0.0f;
            float paddingY = 0.0f;

            if (panelAspect > Settings::TARGET_ASPECT_RATIO) { // Viewport is wider than game (Letterbox)
                targetWidth = panelHeight * Settings::TARGET_ASPECT_RATIO;
                paddingX = (panelWidth - targetWidth) * 0.5f;
            }
            else if (panelAspect < Settings::TARGET_ASPECT_RATIO) { // Viewport is taller than game (Pillarbox)
                targetHeight = panelWidth / Settings::TARGET_ASPECT_RATIO;
                paddingY = (panelHeight - targetHeight) * 0.5f;
            }

            // Ensure dimensions are at least 1 pixel
            int finalTextureWidth = std::max(1, (int)targetWidth);
            int finalTextureHeight = std::max(1, (int)targetHeight);

            // 3) draw the actual viewport texture
            if (gameTextureWidth != finalTextureWidth || gameTextureHeight != finalTextureHeight) {
                if (finalTextureWidth > 0 && finalTextureHeight > 0) {
                    gameTextureWidth = finalTextureWidth;
                    gameTextureHeight = finalTextureHeight;

                    // Resize Texture
                    glBindTexture(GL_TEXTURE_2D, gameTextureColorBuffer);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                    // Resize RenderBuffer
                    glBindRenderbuffer(GL_RENDERBUFFER, gameRboDepthStencil);
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gameTextureWidth, gameTextureHeight);

                    // Resize Texture 2
                    glBindTexture(GL_TEXTURE_2D, CrashoutsEngine::PostProcessing::ppTextureID);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                    char logBuffer[256];
                    // Format the string into the buffer
                    std::snprintf(logBuffer, sizeof(logBuffer),
                        "Resized Game Viewport Texture to %d x %d",
                        gameTextureWidth, gameTextureHeight);

                    // Pass the formatted string to the macro
                    LOGI(Core, logBuffer);
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            // Set Cursor Position: Apply the calculated padding to center the image.
            // GetCursorScreenPos() gives the top-left corner of the content area.
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(screenPos.x + paddingX, screenPos.y + paddingY));
             
            // Let the Gizmo recieve hover/drag over image
            ImGui::SetNextItemAllowOverlap();

            // Display the texture within the ImGui window
            // UV flip (0,1) to (1,0) since OpenGL textures are often upside-down
            ImGui::Image((void*)(intptr_t)CrashoutsEngine::PostProcessing::ppTextureID, 
                ImVec2((float)finalTextureWidth, (float)finalTextureHeight), ImVec2(0, 1), ImVec2(1, 0));

            // Rectangle of that Image in screen space (used by gizmo & picking)
            ImVec2 imgMin = ImGui::GetItemRectMin();
            ImVec2 imgMax = ImGui::GetItemRectMax();
            ImVec2 imgSize = ImGui::GetItemRectSize();

            // 4) ImGuizmo overlay is configured to match the image rect
            ImGuizmo::BeginFrame();
            ImGuizmo::Enable(true);

            ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
            ImGuizmo::SetRect(imgMin.x, imgMin.y, imgSize.x, imgSize.y);

            // TRUE - ORTOGRAPHIC, FALSE - PERSPECTIVE
            ImGuizmo::SetOrthographic(true);

            // 5) if we have a selected entity, let gizmo edit its TRS
            if (selected && ecs.HasComponent<Component::TransformComponent>(selected))
            {
                Entity root = CrashoutsEngine::GetRoot(ecs, selected);

                std::vector<Entity> group;
                group.reserve(16);
                CrashoutsEngine::GatherDescendants(ecs, root, group);

                CEVec3 center{ 0.f, 0.f, 0.f };
                int count = 0;
                for (CrashoutsEngine::Entity e : group)
                {
                    if (!ecs.HasComponent<Component::TransformComponent>(e))
                        continue;
                    auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                    center.x += t->position.x;
                    center.y += t->position.y;
                    center.z += t->position.z;
                    ++count;
                }
                if (count > 0)
                {
                    center.x /= count;
                    center.y /= count;
                    center.z /= count;
                }

                auto* rootT = ecs.GetComponent<Component::TransformComponent>(root);
                if (rootT)
                {
                    float tr[3] = { rootT->position.x, rootT->position.y, rootT->position.z };

                    float rt[3] = { 0.f, 0.f, rootT->rotation };

                    float sc[3] = { rootT->size.x, rootT->size.y, 1.f };

                    float m[16];
                    ImGuizmo::RecomposeMatrixFromComponents(tr, rt, sc, m);

                    const float* view = glm::value_ptr(Component::Rendering::Get().viewMatrix);
                    const float* proj = glm::value_ptr(Component::Rendering::Get().projectionMatrix);

                    const float* snapPtr = nullptr;
                    float snap[3]{};
                    if (g_snapOn)
                    {
                        if (g_gizmoOp == ImGuizmo::ROTATE)
                        {
                            snap[0] = g_snapAngle;
                            snapPtr = snap;
                        }
                        else
                        {
                            snap[0] = g_snapTrans[0];
                            snap[1] = g_snapTrans[1];
                            snap[2] = g_snapTrans[2];
                            snapPtr = snap;
                        }
                    }

                    ImGuizmo::Manipulate(view, proj, g_gizmoOp, g_gizmoMode, m, nullptr, snapPtr);

                    // ---- state for undo + rotation accumulation ----
                    static bool dragging = false;
                    static std::vector<CrashoutsEngine::Entity> movedEntities;
                    static std::vector<CrashoutsEngine::TransformState> startStates;
                    static bool  undoTriggeredThisFrame = false;

                    // full accumulated parent rotation (DEGREES)
                    static float s_parentAccumRotDeg = 0.0f;
                    // last frame's ImGuizmo angle (degrees, wrapped [-180, 180])
                    static float s_prevGizmoDeg = 0.0f;

                    if (ImGuizmo::IsUsing())
                    {
                        float newTr[3], newRt[3], newSc[3];
                        ImGuizmo::DecomposeMatrixToComponents(m, newTr, newRt, newSc);

                        // Begin drag — record all entities' starting positions
                        if (!dragging)
                        {
                            dragging = true;
                            undoTriggeredThisFrame = false;
                            movedEntities.clear();
                            startStates.clear();

                            for (Entity e : group)
                            {
                                if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                                auto* transform = ecs.GetComponent<Component::TransformComponent>(e);

                                CrashoutsEngine::TransformState state;
                                state.position = transform->position;
                                state.rotation = transform->rotation;
                                state.scale = transform->size;
                                startStates.push_back(state);
                                movedEntities.push_back(e);
                            }

                            // cache parent's current rotation (full degree) and gizmo angle
                            s_parentAccumRotDeg = rootT->rotation;
                            s_prevGizmoDeg = newRt[2];
                        }

                        if (g_gizmoOp == ImGuizmo::TRANSLATE)
                        {
                            CEVec3 offset{
                                newTr[0] - rootT->position.x,
                                newTr[1] - rootT->position.y,
                                newTr[2] - rootT->position.z
                            };

                            for (Entity e : group)
                            {
                                if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                                auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                                t->position.x += offset.x;
                                t->position.y += offset.y;
                                t->position.z += offset.z;
                            }
                        }
                        else
                        {
                            rootT->position = { newTr[0], newTr[1], newTr[2] };
                            rootT->size.x = newSc[0];
                            rootT->size.y = newSc[1];

                            if (g_gizmoOp == ImGuizmo::ROTATE)
                            {
                                
                                float gizmoDeg = newRt[2];
                                float stepDeg = gizmoDeg - s_prevGizmoDeg;

                                if (stepDeg > 180.0f) stepDeg -= 360.0f;
                                if (stepDeg < -180.0f) stepDeg += 360.0f;

                                s_parentAccumRotDeg += stepDeg;
                                rootT->rotation = s_parentAccumRotDeg;

                                s_prevGizmoDeg = gizmoDeg;

                                for (Entity e : group)
                                {
                                    if (e == root) continue;
                                    if (!ecs.HasComponent<Component::ParentComponent>(e)) continue;

                                    auto* ppc = ecs.GetComponent<Component::ParentComponent>(e);
                                    ppc->localRot += stepDeg;
                                }
                            }
                            else if (g_gizmoOp == ImGuizmo::SCALE)
                            {
                                // scale should NOT write degrees into a radians field
                                rootT->position = { newTr[0], newTr[1], newTr[2] };
                                // keep existing rotation:
                                // rootT->rotation stays as-is
                                rootT->size.x = newSc[0];
                                rootT->size.y = newSc[1];
                            }
                        }
                    }
                    else if (dragging && !undoTriggeredThisFrame)
                    {
                        dragging = false;
                        undoTriggeredThisFrame = true;

                        std::vector<CrashoutsEngine::TransformState> endStates;
                        endStates.reserve(movedEntities.size());

                        for (auto e : movedEntities)
                        {
                            auto* t = ecs.GetComponent<Component::TransformComponent>(e);
                            if (t)
                            {
                                CrashoutsEngine::TransformState s;
                                s.position = t->position;
                                s.rotation = t->rotation;
                                s.scale = t->size;
                                endStates.push_back(s);
                            }
                        }

                        bool changed = false;
                        for (size_t i = 0; i < startStates.size(); ++i)
                        {
                            if (startStates[i].position != endStates[i].position ||
                                startStates[i].rotation != endStates[i].rotation ||
                                startStates[i].scale != endStates[i].scale)
                            {
                                changed = true;
                                break;
                            }
                        }

                        if (changed)
                        {
                            if (movedEntities.size() > 1)
                            {

                                UndoManager::GetInstance().AddCommand(
                                    std::make_unique<CrashoutsEngine::TransformGroupCommand>(
                                        movedEntities, startStates, endStates));
                            }
                            /*else
                            {
                                UndoManager::GetInstance().AddCommand(
                                    std::make_unique<CrashoutsEngine::TransformEntityCommand>(
                                        movedEntities.front(), startStates.front(), endStates.front()));
                            } */
                        }

                        movedEntities.clear();
                        startStates.clear();
                    }

                    if (!ImGuizmo::IsUsing())
                    {
                        // we let the next drag re-init the state;
                        // no need to reset s_parentAccumRot/s_prevGizmoDeg here
                        undoTriggeredThisFrame = false;
                    }
                }
            }

            // 6) mouse picking: screen -> uv -> world -> pick ECS entity
            ImVec2 mouse = ImGui::GetMousePos();
            const bool over = (mouse.x >= imgMin.x && mouse.x <= imgMax.x && mouse.y >= imgMin.y && mouse.y <= imgMax.y);

            float u = 0.f, v = 0.f;
            if (over) {
                u = (mouse.x - imgMin.x) / (imgMax.x - imgMin.x);
                v = 1.f - (mouse.y - imgMin.y) / (imgMax.y - imgMin.y);
            }

            // Left click: select the top-most hit entity (do NOT clear if we hit nothing)
            // ------------------------------------------------------------
            //  Mouse picking
            //  - only runs if mouse is over the viewport image
            //  - converts mouse to UV, then to world
            //  - walks ECS transforms and does a simple 2D AABB hit test
            //  - picks the entity with the highest Z so clicking overlaps works
            // ------------------------------------------------------------
            if (over && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
            {
                CEVec3 worldPt = ViewUV_toWorld(u, v);

                Entity best = 0;
                float  bestZ = -FLT_MAX;

                // Iterate transforms (top-most by Z wins)
                auto& tArr = ecs.GetComponentArray<Component::TransformComponent>();
                for (Entity e : tArr.indexToEntity)
                {
                    if (!ecs.HasComponent<Component::TransformComponent>(e)) continue;
                    auto* t = ecs.GetComponent<Component::TransformComponent>(e);

                    // Default half-extents from transform size (rectangle-like)
                    float hx = std::max(0.5f * t->size.x, 0.5f);
                    float hy = std::max(0.5f * t->size.y, 0.5f);
                    bool  hit = false;

                    if (auto* sc = ecs.GetComponent<Component::ShapeComponent>(e))
                    {
                        using Component::ShapeType;
                        switch (sc->type)
                        {
                        case ShapeType::Circle:
                        {
                            const float r = 0.5f * std::min(t->size.x, t->size.y);
                            const float dx = worldPt.x - t->position.x;
                            const float dy = worldPt.y - t->position.y;
                            hit = (dx * dx + dy * dy) <= r * r;
                        }
                        break;

                        case ShapeType::Line:
                        {
                            // conservative AABB around line + thickness
                            const float minx = std::min(sc->p1.x, sc->p2.x);
                            const float maxx = std::max(sc->p1.x, sc->p2.x);
                            const float miny = std::min(sc->p1.y, sc->p2.y);
                            const float maxy = std::max(sc->p1.y, sc->p2.y);
                            hx = std::max(hx, std::max(0.5f * (maxx - minx), sc->thickness * 0.5f));
                            hy = std::max(hy, std::max(0.5f * (maxy - miny), sc->thickness * 0.5f));
                            // fall through to AABB test below
                        }
                        [[fallthrough]];

                        case ShapeType::Rectangle:
                        case ShapeType::Triangle:
                        default:
                        {
                            const float minx = t->position.x - hx, maxx = t->position.x + hx;
                            const float miny = t->position.y - hy, maxy = t->position.y + hy;
                            hit = (worldPt.x >= minx && worldPt.x <= maxx &&
                                worldPt.y >= miny && worldPt.y <= maxy);
                        }
                        break;
                        }
                    }
                    else
                    {
                        // No ShapeComponent -> rectangle using transform size
                        const float minx = t->position.x - hx, maxx = t->position.x + hx;
                        const float miny = t->position.y - hy, maxy = t->position.y + hy;
                        hit = (worldPt.x >= minx && worldPt.x <= maxx &&
                            worldPt.y >= miny && worldPt.y <= maxy);
                    }

                    if (hit && t->position.z >= bestZ) { bestZ = t->position.z; best = e; }
                }

                // Toggle selection with LEFT mouse
                if (best) selected = (selected == best) ? (Entity)0 : best;
                // else selected = 0; // click empty -> deselect
            }

            if (over && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                if (!ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
                    selected = 0;
            }
            if (selected && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGuizmo::IsUsing() && ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                ecs.DestroyEntity(selected);
                selected = 0;
            }

            //  7) drag and drop from asset browser into world
            //  Drag-and-drop spawn
            //  Payload is built in the Asset Browser panel.
            //  Here we just read the payload and instantiate a texture/prefab
            //  at the mouse drop location in world space.
            // ------------------------------------------------------------
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("ASSET/PAYLOAD")) {
                    const auto* p = static_cast<const AssetDragPayload*>(pl->Data);
                    CEVec3 dropWorld = ViewUV_toWorld(u, v);

                    using CrashoutsEngine::Assets::AssetManager;
                    using CrashoutsEngine::Assets::TextureAsset;

                    if (p && p->kind == AssetKind::Texture) {
                        if (auto* tex = AssetManager::GetInstance().Get<TextureAsset>(p->name)) {
                            Entity e = ecs.CreateEntity();
                            ecs.AddComponent<NameComponent>(e, std::string("Tex_") + p->name);
                            ecs.AddComponent<TransformComponent>(e);
                            ecs.AddComponent<Component::ShapeComponent>(e);
                            auto* sc = ecs.GetComponent<Component::ShapeComponent>(e);
                            sc->enableTexture = true;
                            sc->textureName = p->name;
                            ecs.GetComponent<Component::TransformComponent>(e)->position = dropWorld;
                        }
                    }
                    else if (p && p->kind == AssetKind::Prefab) {
                        Entity e = Prefab::Create(ecs, std::string(p->name));
                        if (e && ecs.HasComponent<Component::TransformComponent>(e))
                            ecs.GetComponent<Component::TransformComponent>(e)->position = dropWorld;
                    }
                    else if (p && p->kind == AssetKind::Audio) {
                        std::string rel = p->relPath;
                       
                        const std::string prefix = "audio/";
                        auto pos = rel.find(prefix);
                        if (pos != std::string::npos) rel = rel.substr(pos + prefix.size());
                        
                        if (auto dot = rel.find_last_of('.'); dot != std::string::npos) rel.erase(dot);
                        
                        std::string eventLike = std::string("event:/") + rel; // "event:/SFX/Beep"

                        Entity target = selected;
                        if (!target) {
                            target = ecs.CreateEntity();
                            ecs.AddComponent<NameComponent>(target, std::string("Audio_") + p->name);
                            ecs.AddComponent<TransformComponent>(target);
                            ecs.GetComponent<TransformComponent>(target)->position = dropWorld;
                        }

                        if (!ecs.HasComponent<Component::AudioEmitterComponent>(target))
                            ecs.AddComponent<Component::AudioEmitterComponent>(target);

                        if (auto* ac = ecs.GetComponent<Component::AudioEmitterComponent>(target)) {
                            ac->eventPath = eventLike;
                            ac->volume = 1.0f;
                            ac->pitch = 1.0f;
                            ac->spatial3D = true;
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
        
            CEVec3 worldPt = ViewUV_toWorld(u, v);

            InputManager::g_mouseWorldPos = CEVec2{ worldPt.x, worldPt.y };

        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
#endif

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
        auto& collisions = phys.collisions();
        if (ui.drawCollOverlay) {
            CollisionGizmos::DrawAllColliders(ecs, &collisions, ui.overlayThickness);
            if (ui.drawQuadTree) {
                Collision::CollisionHandler::quadTree.Draw();
            }
            //CollisionGizmos::DrawContactsAndNormals(collisions, 0.25f, ui.overlayThickness); //Removed for now, will need to calculate the collision pair for normals properly
        }
        collisions = std::vector<Collision::CollisionEvent>{};

#if 0
        BeginFrame();
        // Dockspace
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("Dockspace", 0,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar(3);

        RenderDebugMenuBar(ui);


        ImGuiID dockspace_id = ImGui::GetID("Dockspace");
        int ds_flags_i = (int)ImGuiDockNodeFlags_None | (int)ImGuiDockNodeFlags_NoDockingInCentralNode | (int)ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiDockNodeFlags ds_flags = (ImGuiDockNodeFlags)ds_flags_i;
        ImGui::DockSpace(dockspace_id, { 0,0 }, ds_flags);
        ImGui::End();
#endif
        ImGuiID dockspace_id = ImGui::GetID("Dockspace"); // Get ID needed for DockBuilder
        ImGuiViewport* viewport = ImGui::GetMainViewport(); // Needed for DockBuilder size

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
            ImGuiID dock_left, dock_right, dock_bottom, dock_top;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Up, 0.07f, &dock_top, &dock_main);
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.18f, &dock_left, &dock_main);
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.26f, &dock_right, &dock_main);
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.28f, &dock_bottom, &dock_main);

            ImGuiID dock_right_bottom;
            ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.5f, &dock_right_bottom, &dock_right);

            // Dock windows into regions (matching your actual window titles)
            ImGui::DockBuilderDockWindow("Debug Controls", dock_left);
            ImGui::DockBuilderDockWindow("Hierarchy", dock_right);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_bottom);
            ImGui::DockBuilderDockWindow("Physics / Collision", dock_bottom);
            ImGui::DockBuilderDockWindow("Console", dock_bottom);
            ImGui::DockBuilderDockWindow("Performance", dock_left);
            ImGui::DockBuilderDockWindow("GameState", dock_bottom);
            ImGui::DockBuilderDockWindow("GameScene", dock_bottom);
            ImGui::DockBuilderDockWindow("Animation", dock_bottom);
            ImGui::DockBuilderDockWindow("Audio", dock_right_bottom);
            ImGui::DockBuilderDockWindow("Engine Controls", dock_top);
            ImGui::DockBuilderDockWindow("Game Viewport", dock_main);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        //auto& io = ImGui::GetIO();

        //if (io.KeyCtrl)
        //{
	       // if (ImGui::IsKeyPressed(ImGuiKey_Z))
	       // {
        //        UndoManager::GetInstance().Undo();
	       // }
        //}

        if (InputManager::IsKeyHeld(Key::LeftCtrl) && InputManager::IsKeyTriggered(Key::Z))
        {
			LOGI(Core, "Undo Triggered");
            UndoManager::GetInstance().Undo();
        }
        if (InputManager::IsKeyHeld(Key::LeftCtrl) && InputManager::IsKeyTriggered(Key::Y))
        {
			LOGI(Core, "Redo Triggered");
            UndoManager::GetInstance().Redo();
        }

        if (ui.showECS) {
            RenderEcsPanel(ecs, entities, selected, ui);
        }
        if (ui.showAssets) {
            static std::string gPickedAsset;
            RenderAssetBrowserPanel(ui, Assets::rootPath.c_str(), gPickedAsset);
        }
        if (ui.showInspector) {
            RenderInspectorPanel(ecs, selected, ui);
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
        if (ui.showGameScene) {
            RenderGameScenePanel(ui);
        }
        if (ui.showEngineControl) {
            RenderEngineStateBar(ui);
        }
        if (ui.showAnimation) {
            RenderAnimationPanel(ecs, selected, ui);
        }


        // EndFrame();

#if USE_IMGUI_EDITOR
        RenderGameViewport(ecs, selected);
#endif


    }

    /*!
     @brief Backward-compatible wrapper for rendering without explicit UIState.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param phys Physics handler reference.
     @param entities List of ECS entities.
     @param selected Currently selected entity.
    */
    void Render(FrameProfiler& profiler, Physics::PhysicsHandler& phys, Entity& selected, UIState& ui)
    {
        ECS* ecs = ECS::getInstance();

        std::vector<CrashoutsEngine::Entity> entities =
            ecs->GetComponentArray<Component::NameComponent>().indexToEntity;
        //std::cout << entities.size() << " ";

        //static UIState ui;
        RenderAll(profiler, phys, ui, *ecs, entities, selected);
    }

    void SetDarkGrayTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Window & Panels
        colors[ImGuiCol_WindowBg] = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.08f, 0.08f, 0.08f, 0.6f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // Text
        colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

        // Menus & Title Bars
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

        // Buttons / Headers
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);

        colors[ImGuiCol_Header] = ImVec4(0.21f, 0.21f, 0.21f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.19f, 0.19f, 0.19f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
        colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);

        // Frames / Inputs
        colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);

        // Scrollbars
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

        // Checkboxes / Sliders
        colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);

        // Docking
        colors[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.35f, 0.35f, 0.35f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);

        // Misc
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.30f, 0.30f, 0.5f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.7f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.50f, 0.50f, 0.9f);

        // Rounding & Borders
        style.WindowRounding = 3.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;
        style.ScrollbarRounding = 5.0f;

        style.FrameBorderSize = 1.0f;
        style.WindowBorderSize = 1.0f;
    }


} // namespace UI