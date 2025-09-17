/*!
@file       debugmenu.h
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

This header provides the graphics for Crashout Engine's ImGUI.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include <GLFW/glfw3.h>

// Forward declarations
class FrameProfiler;
namespace CrashoutsEngine { class ECS; }
namespace Physics { class PhysicsHandler; }
namespace Collision { struct CollisionEvent; }

namespace UI {

    // Shared toggles/state across panels
    struct UIState {
        bool lockLayout = false;
        bool showPerf = false;
        bool showConsole = true;
        bool drawCollOverlay = true;
    };

    // Initialize / shutdown
    void Init(GLFWwindow* window);
    void Shutdown();

    // Backward-compatibility moment
    // Internally this now delegates to smaller panel functions.
    void Render(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);

    // Optional helpers if you prefer explicit control:
    void BeginFrame();
    void EndFrame();

    // Individual Render Panels (Make it easier to Implement more features as time goes on)
    void RenderEcsPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);
    void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui);
    void RenderConsolePanel(UIState& ui);
    void RenderPhysicsPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui);
    void DrawCollisionOverlay(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>& events, const UIState& ui);

    // Do everything (BeginFrame -> panels -> overlays -> EndFrame)
    void RenderAll(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);

} // namespace UI