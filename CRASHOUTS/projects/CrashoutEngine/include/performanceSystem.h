/*!
@file       performanceSystem.h
@author     mohamadamirakmal.b@digipen.edu
@date       04/11/2025

@brief      Thin runtime facade over the engine's profiler UI.
@details    `PerformanceSystem` is a tiny singleton whose only job is to:
            - keep a pointer to the engine-wide @ref FrameProfiler
              (declared in performanceViewer.h),
            - drive per-frame begin/end calls, and
            - expose a single `RenderUI()` that draws the Performance window.

            This keeps the ImGui performance panel code in one place instead of
            scattering it across engine/editor files.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include "performanceViewer.h"

/*
----------------------------------------------------------------------------- */
/*!
 @class PerformanceSystem
 @brief Singleton wrapper that owns no data by itself, but forwards to a
        FrameProfiler supplied by the engine.
 */
class PerformanceSystem {
public:
    /*!
     @brief  Access the single instance.
     */
    static PerformanceSystem& Get() {
        static PerformanceSystem instance;
        return instance;
    }

    /*!
     @brief  Provide the system with the profiler to drive.
     @param  p  engine/global profiler created elsewhere.
     @note   Must be called once during engine/editor init. If not attached,
             BeginFrame/EndFrame become no-ops. :contentReference[oaicite:1]{index=1}
     */
    void Attach(FrameProfiler& p) { profiler = &p; }

    /*!
     @brief  Start a new profiling frame.
     @note   Call this once near the start of the main loop, before you create
             any `FrameProfiler::Scoped` timers for this frame.
     */
    void BeginFrame() {
        if (profiler) profiler->begin_frame();
    }

    /*!
     @brief  Finish the profiling frame and Update UI snapshots.
     @note   Call this once after all timed scopes have been destroyed. :contentReference[oaicite:2]{index=2}
     */
    void EndFrame() {
        if (profiler) profiler->end_frame();
    }

    /*!
     @brief  Retrieve the attached profiler.
     @return Reference to the profiler. Caller must ensure one was attached.
     */
    FrameProfiler* GetProfiler() { return profiler; }

private:
    PerformanceSystem() = default;

    // profiler is owned elsewhere (engine/global); we just store the pointer.
    FrameProfiler* profiler = nullptr;
};