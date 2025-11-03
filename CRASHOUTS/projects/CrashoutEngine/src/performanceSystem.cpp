#include "performanceSystem.h"
#include "imgui.h"
#include "debugmenu.h" // if your UI helpers live there

void PerformanceSystem::RenderUI() {
    ImGui::Begin("Performance");
    DrawEngineMemoryPanel();
    DrawProfilerPanel(GetProfiler());   // was: DrawProfilerPanel(profiler);
    ImGui::End();
}