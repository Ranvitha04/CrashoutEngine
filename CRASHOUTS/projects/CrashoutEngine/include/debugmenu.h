#pragma once
#include <GLFW/glfw3.h>

// Forward declarations
class FrameProfiler;

namespace UI {
    // Initialize ImGui and related UI state
    void Init(GLFWwindow* window);

    // Render all UI each frame
    void Render(FrameProfiler& profiler);

    // Shutdown / free ImGui resources
    void Shutdown();
} // namespace UI