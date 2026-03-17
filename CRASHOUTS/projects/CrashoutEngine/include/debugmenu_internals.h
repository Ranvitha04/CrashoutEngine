/*!
@file       debugmenu_internals.h
@author     Renfred Lee
@author     Afiq
@author     Mohamad Amir Akmal
@date       01/12/2025
@brief      [Brief description of debugmenu_internals.h]

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#pragma once

// --- 1. ImGui & Visuals ---
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

// --- 2. Math & Standards ---
#include <glm/gtc/type_ptr.hpp>
#include "CEVec3.h"
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

// --- 3. Engine Core ---
#include "engine.h"
#include "glapp.h"
#include "rendering.h"
#include "camera.h"
#include "settings.h"
#include "frameratecontroller.h"
#include "inputmanager.h"
#include "log.h"
#include "logdefs.h"

// --- 4. ECS & Components (The most important part for Inspector) ---
#include "entitymanager.h"
#include "namecomponent.h"
#include "transformcomponent.h"
#include "uuidcomponent.h"
#include "shapescomponent.h"
#include "audiocomponent.h"
#include "tilemapcomponent.h"
#include "prefabcomponent.h"
#include "hierarchycomponent.h" // Parent/Child components
#include "keyframe.h" // Animator component
#include "activecomponent.h"
#include "tagcomponent.h"
#include "hierarchy.h"
#include "visualscriptcomponent.h" 

// --- 5. Systems & Managers ---
#include "physicsmanager.h"
#include "collisionmanager.h"
#include "gamestatemanager.h"
#include "assetmanager.h"
#include "audioSystem.h" // or "audioSystem.h" based on your project
#include "monomanager.h"
#include "scriptsystem.h"
#include "ppmanager.h"
#include "undomanager.h"

// --- 6. Assets & Resources ---
#include "textureasset.h"
#include "sceneasset.h"
#include "spritesheet.h"
#include "prefab.h"
#include "stb_image.h"

// --- 7. Debug Specific Tools ---
#include "debugmenu.h"
#include "debugreflection.h"
#include "debugcollision.h"
#include "debugAssetLoader.h"
#include "performanceViewer.h"
#include "hierarchy.h"

// --- 8. External / Utils ---
#include <fmod_studio.hpp>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"

// --- 9. Commands (Undo/Redo) ---
#include "changevaluecommand.h"
#include "transformentitycommand.h"

// Forward declarations of Engine types to speed up compilation
namespace CrashoutsEngine { class ECS; }
namespace CrashoutsEngine::Physics { class PhysicsHandler; }
class FrameProfiler;

namespace UI {

    // --- Shared Helper Structs & Functions ---

    static std::vector<ProfilerSample> gPerfSnapshot;
    static double gPerfSnapshotTotalMs = 0.0;

    extern std::vector<std::string> gDroppedPaths;

    namespace KeyframeUnityEditor
    {
        struct Selection {
            CrashoutsEngine::Entity entity{};
            int bindingIndex = -1;
            int keyIndex = -1;
        };

        static Selection gSel;

        inline const char* KFPropertyName(KFProperty p)
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

    inline ImGuiWindowFlags LockFlags(bool locked) {
        return locked ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking)
            : ImGuiWindowFlags_None;
    }

    // RAII wrapper for Windows
    struct ScopedWindow {
        ScopedWindow(const char* title, ImGuiWindowFlags flags = 0) { ImGui::Begin(title, nullptr, flags); }
        ~ScopedWindow() { ImGui::End(); }
    };

    // Tooltip helper
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

    // Shared Externs (State needed across files)
    extern std::vector<std::string> gDroppedPaths; // Shared for Init and AssetBrowser

    // --- Panel Render Functions (Implemented in other .cpp files) ---

    void RenderGameViewport(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity& selected);
    void RenderAssetBrowserPanel(UIState& ui, std::string& outSelectedAsset);
    void RenderEcsPanel(CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected, UIState& ui);
    void RenderInspectorPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui);
    void RenderAnimationPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui);
    void RenderAudioPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, UIState& ui);
    void RenderPhysicsPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, UIState& ui);
    void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui);
    void RenderConsolePanel(UIState& ui);
    void RenderGameStatePanel(UIState& ui);
    void RenderGameScenePanel(UIState& ui);
    void RenderEngineStateBar(UIState& ui);
    void RenderProjectManager(UIState& ui);
    void RenderVisualScriptingPanel(UIState& ui);
    void RenderBuildPanel(UIState& ui);
}
