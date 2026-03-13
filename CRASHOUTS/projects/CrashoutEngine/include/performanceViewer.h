/*!
@file       performanceViewer.h
@author     Mohamad Amir Akmal
@author     Shahrin Idzuan
@author     Renfred Lee
@date       06/09/2025

@brief      CPU profiling and memory tracking utilities for Crashout Engine.
@details    This header provides two major runtime diagnostic tools:
            - **Frame Profiler**: Measures per-frame CPU time across subsystems,
              with EMA smoothing, percent normalization, history tracking,
              and ImGui visualization.
            - **Memory Tracker**: Tracks memory allocations per subsystem
              (`MemTag`), monitors process memory usage (Working Set &
              Private Bytes), and renders a memory usage panel.

            These tools are intended for development builds to monitor
            performance bottlenecks, visualize profiling history, and
            analyze memory consumption in real-time.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono> // time tracking lib
#include <algorithm>
#include <array>
#include <imgui.h>
#include "entitymanager.h"


// ---- Windows process-memory helper (Working Set & Private Bytes in MB) ----
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

/*
* Frame Profiler
----------------------------------------------------------------------------- */
/*!
@struct ProfilerSample
@brief  A single row for UI presentation.
@var name
      label.
@var ms
      smoothed milliseconds (Exponential Moving Average - EMA) attributed to the region.
@var pct
      percentage of the chosen budget (see @ref FrameProfiler::PercentMode).
*/
struct ProfilerSample {
    std::string name;
    double ms = 0.0;
    double pct = 0.0;
};

/*!
@class  FrameProfiler
@brief  per-frame profiler with EMA smoothing and percent normalization.
*/
class FrameProfiler {
public:
    static constexpr size_t kHistory = 240;              // ~4s @60fps
    using HistArray = std::array<float, kHistory>;
    using EmaHistMap = std::unordered_map<std::string, HistArray>;

    /*!
    @enum PercentMode
    @brief how the percent column is computed for display.

    @var NormalizeToRealFrame
         percent is computed from this frame's real duration. Values are clamped to [0,100].

    @var NormalizeToEmaSum
         percent is computed from the sum of EMA milliseconds across regions This is the default and keeps rows stable so they sum to ~100%.
    
    */
    enum class PercentMode {
        NormalizeToRealFrame,  // % = ms / real frame time
        NormalizeToEmaSum      // % = ms / sum(ema_ms)  <-- default (stable, sums to 100)
    };

    /*!
    @brief  begin a new frame.

    clears per-frame accumulation and records the start timestamp.

    @note   call exactly once near the start of each frame.
    */
    void begin_frame() {
        if (frameOpen) {
            // guard against accidental re-entry
            return;
        }            
        sampleMS.clear();
        frameStart = Clock::now();
        frameOpen = true;
    }

    /*!
    @class scoped
    @brief RAII helper to time a code region.

    construct at scope entry and let it destruct at scope exit to record the
    elapsed milliseconds to the parent @ref FrameProfiler under a name.
    */
    class Scoped {
    public:
        /*!
        @brief  construct a timed scope.
        @param  p     owning profiler instance.
        @param  name  label for the timed region (must outlive the scope).
        */
        Scoped(FrameProfiler* p, const char* name) : p_(p), name(name), t0(Clock::now()) {
            if (p_) t0 = Clock::now();
        }

        /*!
        @brief  on destruction, records elapsed time to the profiler.
        */
        ~Scoped() {
            if (p_) {
                using namespace std::chrono;
                const double ms = duration<double, std::milli>(Clock::now() - t0).count();
                p_->add_ms(name, ms);
            }
        }
    private:
        FrameProfiler* p_;
        const char* name;
        using Clock = std::chrono::steady_clock;
        Clock::time_point t0;
    };

    /*!
    @brief  manually accumulate milliseconds for a region in the current frame.
    @param  name  region label.
    @param  ms    milliseconds to add.
    */
    void add_ms(const char* name, double ms) { sampleMS[name] += ms; }

    /*!
    @brief  end the current frame and Update smoothed values.
    Steps performed:
    1) compute real frame duration from @ref begin_frame.
    2) Update EMA per region using the per-frame accumulators.
    3) smooth the headline "frame ms" for UI display.
    4) build/sort rows and produce a throttled snapshot for the UI.
    @note Call after all @ref Scoped timers have been destroyed for this frame.
    */
    void end_frame() {
        using namespace std::chrono;

        // 1) real frame duration
        double frameMSReal = duration<double, std::milli>(Clock::now() - frameStart).count();
        if (frameMSReal <= 0.0) {
            frameMSReal = 0.0001;
        }

        // 2) Update EMA per system
        for (auto& kv : sampleMS) {
            double& ema = emaMS[kv.first];
            ema = (1.0 - alpha) * ema + alpha * kv.second;
        }

        // 3) smooth headline frame time
        frameMSDisplay = (frameMSDisplay <= 0.0) ? frameMSReal : (1.0 - alpha) * frameMSDisplay + alpha * frameMSReal;

        // 4) build rows
        rowsTmp.clear(); 
        rowsTmp.reserve(emaMS.size());
        double sum_ema = 0.0;

        for (auto& kv : emaMS) {
            sum_ema += kv.second;
        }
        if (sum_ema <= 0.0) {
            sum_ema = 0.0001;
        }

        const double denom = (pctMode == PercentMode::NormalizeToRealFrame) ? frameMSReal : sum_ema;

        for (auto& kv : emaMS) {
            ProfilerSample s;
            s.name = kv.first;
            s.ms = kv.second;
            s.pct = (s.ms / denom) * 100.0;

            // clamp to [0,100]
            if (s.pct < 0.0) s.pct = 0.0;
            if (s.pct > 100.0) s.pct = 100.0;
            rowsTmp.push_back(std::move(s));
        }

        // 5) sort by descending ms
        std::sort(rowsTmp.begin(), rowsTmp.end(),
            [](auto& a, auto& b) {
                return a.ms > b.ms;
            });

        // 6) throttle UI updates
        const auto now = Clock::now();
        if (lastPresent.time_since_epoch().count() == 0) lastPresent = now;
        const double since = duration<double>(now - lastPresent).count();

        frameMSHist[histIndex] = static_cast<float>(frameMSDisplay);

        // ensure every region seen so far has a history slot
        for (auto& kv : emaMS) {
            auto& arr = emaHist[kv.first];
            arr[histIndex] = static_cast<float>(kv.second);
        }

        histIndex = (histIndex + 1) % kHistory;
        histCount = std::min(histCount + 1, kHistory);

        if (since >= presentIntervalSec) {
            snapShot = rowsTmp;
            snapShotFrameMS = frameMSDisplay;
            lastPresent = now;
            presentReady = true;
        }
        else {
            presentReady = false;
        }
        frameOpen = false;
    }

    // ----------------------------- UI readouts ------------------------------

    /*!
    @brief  whether a fresh snapshot is available for UI.
    @return true if @ref results and @ref total_ms refer to a new snapshot.
    */
    bool present_ready() const { return presentReady; }

    /*!
    @brief  current snapshot of samples (labels, EMA ms, percent).
    @return reference to a vector of @ref ProfilerSample.
    */
    const std::vector<ProfilerSample>& results() const { return snapShot; }

    /*!
    @brief  smoothed headline per-frame time (ms) for display.
    @return EMA-smoothed frame time in milliseconds.
    */
    double total_ms() const { return snapShotFrameMS; }

    // ------------------------------ Tuning ----------------------------------

    /*!
    @brief  set EMA smoothing factor.
    @param  a  Alpha in [0, 1]; higher reacts faster, lower is smoother.
    @note   Typical range 0.12�0.30. Values are clamped to [0.05, 1.0].
    */
    void set_ema_alpha(double a) {
        if (a < 0.05) a = 0.05;
        if (a > 1.0)  a = 1.0;
        alpha = a;
    }

    /*!
    @brief  set minimum time between UI snapshot updates.
    @param  seconds  Interval in seconds (min 0.01).
    */
    void set_present_interval(double seconds) {
        if (seconds < 0.01) seconds = 0.01;
        presentIntervalSec = seconds;
    }

    /*!
    @brief  Select the percent normalization mode used in the UI.
    @param  m  Mode (see @ref PercentMode).
    */
    void set_percent_mode(PercentMode m) { pctMode = m; }

    // Read-only accessors for UI
    static constexpr size_t history_capacity() { return kHistory; }
    size_t history_count() const { return histCount; }
    size_t history_index() const { return histIndex; }

    const HistArray& frame_ms_history() const { return frameMSHist; }
    const EmaHistMap& ema_history()   const { return emaHist; }

    void Clear() {
        // 1. Force release memory for all maps and vectors
        std::unordered_map<std::string, double>().swap(sampleMS);
        std::unordered_map<std::string, double>().swap(emaMS);
        std::vector<ProfilerSample>().swap(rowsTmp);
        std::vector<ProfilerSample>().swap(snapShot);
        EmaHistMap().swap(emaHist);

        // 2. Reset primitive states
        frameOpen = false;
        frameMSDisplay = 0.0;
        snapShotFrameMS = 0.0;
        histIndex = 0;
        histCount = 0;

        // 3. Zero out the fixed array
        frameMSHist.fill(0.0f);
    }

private:

    using Clock = std::chrono::steady_clock;

    bool frameOpen = false;

    // raw per-frame accumulation
    std::unordered_map<std::string, double> sampleMS;

    // EMA state
    std::unordered_map<std::string, double> emaMS;
    double alpha = 0.20;

    // frame timing
    Clock::time_point frameStart{};
    double frameMSDisplay = 0.0;   // smoothed headline

    // working buffer and throttled snapshot
    std::vector<ProfilerSample> rowsTmp;
    std::vector<ProfilerSample> snapShot;
    double snapShotFrameMS = 0.0;
    double presentIntervalSec = 0.25;
    Clock::time_point lastPresent{};
    bool presentReady = true;

    // % mode
    PercentMode pctMode = PercentMode::NormalizeToEmaSum;

    size_t histIndex = 0;
    size_t histCount = 0;
    HistArray  frameMSHist{};  // total frame ms
    EmaHistMap emaHist;        // per-region EMA history

};

/*
* Memory Tracking
----------------------------------------------------------------------------- */
/*!

/*!
@class  MemTag
@brief  Categories for memory taggings
*/
enum class MemTag : uint8_t {
    Unknown = 0,
    Update,
    Physics,
    Graphics,
    AI,
    Audio,
    ECS,
    Resources,
    UI,
    Renderer,
    Count
};

/*!
 @struct MemTracker
 @brief Global memory tracking counters.
 @details
 Tracks total live, allocated, and freed memory for each `MemTag` category.
 Accessed as a singleton via `MemTracker::instance()`.
*/
struct MemTracker {
    std::atomic<long long> live[(size_t)MemTag::Count]{};
    std::atomic<long long> allocs[(size_t)MemTag::Count]{};
    std::atomic<long long> frees[(size_t)MemTag::Count]{};
    static inline MemTracker& instance() { static MemTracker T; return T; }
};

// Current tag for this thread; set by ScopedMemTag.
inline thread_local MemTag g_memTagTLS = MemTag::Unknown;

/*!
@struct ScopedMemTag
@brief  RAII helper to set a temporary tag in a scope
*/
struct ScopedMemTag {
    MemTag prev;
    explicit ScopedMemTag(MemTag t) : prev(g_memTagTLS)
    {
        g_memTagTLS = t;
    }

    ~ScopedMemTag()
    {
        g_memTagTLS = prev;
    }
};

/*! @brief Increments live memory for a given tag. */
inline void memtag_add(MemTag t, size_t bytes) { MemTracker::instance().live[(size_t)t] += (long long)bytes; }

/*! @brief Decrements live memory for a given tag. */
inline void memtag_sub(MemTag t, size_t bytes) { MemTracker::instance().live[(size_t)t] -= (long long)bytes; }

/*!
 @brief Retrieves the current process's Working Set and Private Bytes (in MB).
 @param workingSetMB   Output reference for Working Set size in MB.
 @param privateBytesMB  Output reference for Private Bytes in MB.
 @note  Returns 0.0 for both values if retrieval fails.
*/
inline void get_process_mem_mb(double& workingSetMB, double& privateBytesMB) {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
        sizeof(pmc))) {
        workingSetMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
        privateBytesMB = pmc.PrivateUsage / (1024.0 * 1024.0);
    }
    else {
        workingSetMB = privateBytesMB = 0.0;
    }
}

// ------------------------------ Rendering ----------------------------------
/*!
    @brief  Visualize memory per tag with ImGUI
    @note   Display Working Set, Private Bytes and total TAGGED memory.
*/
inline void DrawEngineMemoryPanel() {
    double workingSetMB = 0.0;
    double privateBytesMB = 0.0;
    get_process_mem_mb(workingSetMB, privateBytesMB);

    ImGui::SeparatorText("Memory Usage");
    ImGui::Text("Working Set: %.2f MB", workingSetMB);
    ImGui::Text("Private Bytes: %.2f MB", privateBytesMB);
    
    ImGui::Separator();
    bool usePool = MemMgrConfig::s_useMemoryPool;
    ImGui::Text("Component Allocator: %s", usePool ? "Custom Memory Pool" : "OS Allocator");

    size_t totalPages = 0;
    size_t totalActive = 0;
    CrashoutsEngine::ECS::getInstance()->GetMemoryStats(totalPages, totalActive);
    ImGui::Text("Total Pages: %zu", totalPages);
    ImGui::Text("Active Components: %zu", totalActive);

    // Add more performance stats here if you want

    //ImGui::SeparatorText("Framerate");
    //ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
}


/*!
 @brief Draws an ImGui profiler panel with per-frame CPU usage.
 @param P Reference to a `FrameProfiler` instance.
*/
inline void DrawProfilerPanel(FrameProfiler& P)
{
    using namespace ImGui;

    ImGui::SeparatorText("CPU Frame (ms) - Stream");

    const auto& rows = P.results();
    const size_t count = P.history_count();
    constexpr size_t cap = FrameProfiler::history_capacity();
    const size_t idx = P.history_index();
    const int icount = static_cast<int>(count);
    const int offset = static_cast<int>((idx + cap - count) % cap);

    if (count > 0) {
        const auto& fh = P.frame_ms_history();
        float maxFrame = 0.0f;
        for (int i = 0; i < icount; ++i) {
            int j = (offset + i) % (int)cap;
            maxFrame = std::max(maxFrame, fh[j]);
        }
        float yMaxFrame = std::max(1.0f, maxFrame * 1.15f);
        PlotLines("Frame ms", fh.data(), icount, offset, nullptr, 0.0f, yMaxFrame, ImVec2(-1, 60));
    }

    ImGui::SeparatorText("Systems (EMA ms) - Streams");
    const auto& eh = P.ema_history();
    for (auto const& r : rows) {
        auto it = eh.find(r.name);
        if (it == eh.end()) continue;

        TextUnformatted(r.name.c_str());
        float localMax = 0.0f;
        for (int i = 0; i < icount; ++i) {
            int j = (offset + i) % (int)cap;
            localMax = std::max(localMax, it->second[j]);
        }
        float yMaxRow = std::max(0.5f, localMax * 1.25f);

        std::string id = "##spark_" + r.name;
        PlotLines(id.c_str(), it->second.data(), icount, offset, nullptr, 0.0f, yMaxRow, ImVec2(-1, 40));
    }
}