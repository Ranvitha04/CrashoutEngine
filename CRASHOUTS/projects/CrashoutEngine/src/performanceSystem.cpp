/*!
@file       performanceSystem.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       04/11/2025

@brief      ImGui glue for the engine's performance viewer.
@details    This file implements the rendering layer of the Performance System.
            It draws the performance window and hooks into the engine’s profiler
            to visualize runtime metrics such as frame times and memory usage.

            The functions here are intentionally thin wrappers. All heavy lifting
            (profiling logic, memory stats, UI composition) is done in
            performanceViewer.h.

            Responsibilities:
            - Check if a profiler is attached
            - Create the ImGui window for the "Performance" tab
            - Delegate to DrawEngineMemoryPanel() and DrawProfilerPanel()
*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "performanceSystem.h"
#include "imgui.h"
#include "debugmenu.h"

/*
----------------------------------------------------------------------------- */

/*!
 @brief  Renders the "Performance" panel UI.
 @details
 Displays both CPU timing (per-system profiler breakdown) and engine memory usage.
 The visual layout is handled by helpers declared in @ref performanceViewer.h:
 - DrawEngineMemoryPanel() - shows memory usage per tag.
 - DrawProfilerPanel() - shows CPU system timing data.
*/
void PerformanceSystem::RenderUI()
{
    // Safety: ensure we have a valid profiler before drawing
    if (!profiler)
        return;

    // Begin Performance Window
    if (ImGui::Begin("Performance")) {
        DrawEngineMemoryPanel();      // memory view
        DrawProfilerPanel(*profiler); // CPU profiling table
    }
    ImGui::End();
}