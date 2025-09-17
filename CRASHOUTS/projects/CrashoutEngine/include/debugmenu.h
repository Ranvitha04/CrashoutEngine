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

namespace UI {
    // Initialize ImGui and related UI state
    void Init(GLFWwindow* window);

    // Render all UI each frame
    void Render(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys);

    // Shutdown / free ImGui resources
    void Shutdown();
} // namespace UI