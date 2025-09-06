/*!
@file       performanceViewer.h
@author     mohamadamirakmal.b@digipen.edu
@date       06/09/2025

This file provides a small profiling utility for real-time applications.
//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono> // time tracking lib
#include <algorithm>
#include <cmath>
/*
----------------------------------------------------------------------------- */

/*!
@struct ProfilerSample
@brief  A single row for UI presentation.
@var ProfilerSample::name
      label.
@var ProfilerSample::ms
      smoothed milliseconds (Exponential Moving Average - EMA) attributed to the region.
@var ProfilerSample::pct
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
        sampleMS.clear();
        frameStart = Clock::now();
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
        Scoped(FrameProfiler& p, const char* name)
            : p_(p), name(name), t0(Clock::now()) {
        }

        /*!
        @brief  on destruction, records elapsed time to the profiler.
        */
        ~Scoped() {
            using namespace std::chrono;
            const double ms = duration<double, std::milli>(Clock::now() - t0).count();
            p_.add_ms(name, ms);
        }
    private:
        FrameProfiler& p_;
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
    @brief  end the current frame and update smoothed values.
    Steps performed:
    1) compute real frame duration from @ref begin_frame.
    2) update EMA per region using the per-frame accumulators.
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

        // 2) update EMA per system
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
            // clamp to [0,100] with tiny epsilon to avoid showing "100.0%" due to rounding
            if (s.pct < 0.0) s.pct = 0.0;
            if (s.pct > 100.0) s.pct = 100.0;
            rowsTmp.push_back(std::move(s));
        }

        // 5) sort by descending ms
        std::sort(rowsTmp.begin(), rowsTmp.end(),
            [](const ProfilerSample& a, const ProfilerSample& b) {
                return a.ms > b.ms;
            });

        // 6) throttle UI updates
        const auto now = Clock::now();
        if (lastPresent.time_since_epoch().count() == 0) lastPresent = now;
        const double since = duration<double>(now - lastPresent).count();

        if (since >= presentIntervalSec) {
            snapShot = rowsTmp;
            snapShotFrameMS = frameMSDisplay;
            lastPresent = now;
            presentReady = true;
        }
        else {
            presentReady = false;
        }
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
    @note   Typical range 0.12–0.30. Values are clamped to [0.05, 1.0].
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


private:
    using Clock = std::chrono::steady_clock;

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
};