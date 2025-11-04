#pragma once
#include "performanceViewer.h"

class PerformanceSystem {
public:
    static PerformanceSystem& Get() {
        static PerformanceSystem instance;
        return instance;
    }

    // NEW
    void Attach(FrameProfiler& p) { profiler = &p; }

    void BeginFrame() { if (profiler) profiler->begin_frame(); }
    void EndFrame() { if (profiler) profiler->end_frame(); }
    FrameProfiler& GetProfiler() { return *profiler; }

    void RenderUI();

private:
    PerformanceSystem() = default;
    FrameProfiler* profiler = nullptr;
};