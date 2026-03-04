/*!
@file		replaymanager.cpp
@author     lee.r@digipen.edu
@date       03/03/2026
@brief		Maintain code used for tracking gameplay and performing replay
            of past gameplay funtionalities.
*/

#include "engine.h"
#include "replaymanager.h"
#include "log.h"

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace ReplayManager {

    // We use these to reset the "Replay Clock" to 0 every time we start.
    static Tick s_RecordStartTick = 0;
    static bool s_FirstRecordTick = false;

    static Tick s_PlaybackStartTick = 0;
    static bool s_FirstPlaybackTick = false;

    /*
    @brief Declare start of recording
    */
    void Replay::StartRecording() {
        if (state == ReplayState::RECORDING) return; // Already recording
        if (state == ReplayState::PLAYING) Stop();   // Stop playing before recording

        state = ReplayState::RECORDING;
        replayData.clear();
        playbackIndex = 0;
        InputManager::SetReplayMode(false); // Ensure we read real hardware

        s_FirstRecordTick = true; // Flag to capture the baseline time on the first event

        LOGI(Debug, "[Replay] Started Recording.");
    }

    /*
    @brief "Replay" the events by sending recorded input events
    */
    void Replay::StartPlayback() {
        if (state == ReplayState::PLAYING) return;   // Already playing
        if (state == ReplayState::RECORDING) Stop(); // Stop recording first

        if (replayData.empty()) {
            LOGI(Debug, "[Replay] No data to playback!");
            return;
        }
        state = ReplayState::PLAYING;
        playbackIndex = 0;
        InputManager::SetReplayMode(true); // Lock out real hardware!

        s_FirstPlaybackTick = true; // Flag to capture the baseline time on frame 1 of playback

        LOGI(Debug, "[Replay] Started Playback.");
    }

    /*
    @brief Reset the state and stop all playback / recording
    */
    void Replay::Stop() {
        if (state == ReplayState::IDLE) return; // Already stopped

        state = ReplayState::IDLE;
        InputManager::SetReplayMode(false); // Return control to player
        LOGI(Debug, "[Replay] Stopped.");
    }

    /*
    @brief Record given event
    */
    void Replay::RecordEvent(Tick tick, Key key, bool isPressed) {
        if (state == ReplayState::RECORDING) {

            int code = static_cast<int>(key);

            // --- IGNORE ALL FUNCTION KEYS (F1 - F25) ---
            // In GLFW, F1 starts at 290 and F25 ends at 314.
            // By ignoring this entire block, we prevent any engine-level 
            // hotkeys from polluting the gameplay replay buffer.
            if (code >= 290 && code <= 314) {
                return; // Silently drop it
            }

            // --- RELATIVE TIME CONVERSION ---
            if (s_FirstRecordTick) {
                s_RecordStartTick = tick;
                s_FirstRecordTick = false;
            }

            // If we started at tick 5000, and this is tick 5005, relative is 5.
            Tick relativeTick = tick - s_RecordStartTick;

            replayData.push_back({ relativeTick, key, isPressed });

            std::string stateStr = isPressed ? "Pressed" : "Released";
            std::string logMsg = "[Replay] RECORDED KeyCode: " + std::to_string(code) +
                " | State: " + stateStr +
                " | RelTick: " + std::to_string(relativeTick);
            LOGI(Debug, logMsg);
        }
    }

    /*
    @brief Updates the state and send input based on current input event and current tick "time"
    */
    void Replay::UpdateFixedTick(Tick currentTick) {

        if (state != ReplayState::PLAYING) return;

        // --- RELATIVE TIME CONVERSION ---
        if (s_FirstPlaybackTick) {
            s_PlaybackStartTick = currentTick;
            s_FirstPlaybackTick = false;
        }

        Tick relativeCurrentTick = currentTick - s_PlaybackStartTick;

        // Process all events scheduled for the current tick
        while (playbackIndex < replayData.size() &&
            replayData[playbackIndex].fixedTick == relativeCurrentTick)
        {
            InputEvent& ev = replayData[playbackIndex];
            int code = static_cast<int>(ev.key);

            std::string stateStr = ev.isPressed ? "Pressed" : "Released";
            std::string logMsg = "[Replay] Injecting KeyCode: " + std::to_string(code) +
                " | State: " + stateStr +
                " | RelTick: " + std::to_string(relativeCurrentTick);
            LOGI(Debug, logMsg);

            // Inject based on whether it's a mouse button (0-7) or keyboard key
            if (code >= 0 && code <= GLFW_MOUSE_BUTTON_LAST) {
                InputManager::InjectMouseButton(ev.key, ev.isPressed);
            }
            else {
                InputManager::InjectKey(ev.key, ev.isPressed);
            }

            playbackIndex++;
        }

        // Auto-stop when we reach the end of the recording
        if (playbackIndex >= replayData.size()) {
            LOGI(Debug, "[Replay] Playback finished.");
            Stop();
        }
    }

    /*
    @brief Save the current replay buffer to a binary file
    */
    void Replay::SaveRecording(const std::string& filename) {
        // --- SAFE PATH RESOLUTION ---
#if USE_IMGUI_EDITOR
        // Editor Mode: Save right into the project assets folder
        std::filesystem::path dirPath = std::filesystem::path(GetGameRoot()) / "assets" / "replays";
#else
        // Standalone Game: Save to the user's Windows AppData folder!
        std::filesystem::path dirPath = std::filesystem::path(GetLocalAppData()) / "replays";
#endif

        // Ensure the directory exists
        std::filesystem::create_directories(dirPath);
        std::filesystem::path fullPath = dirPath / (filename + ".bin");

        std::ofstream out(fullPath, std::ios::binary);
        if (!out.is_open()) {
            LOGE(Core, "[Replay] Failed to open file for saving: " + fullPath.generic_string());
            return;
        }

        // 1. Write the number of events
        size_t count = replayData.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(size_t));

        // 2. Dump the entire vector directly into the file! (Blazing fast)
        out.write(reinterpret_cast<const char*>(replayData.data()), count * sizeof(InputEvent));

        out.close();
        LOGI(Core, "[Replay] Saved recording to: " + fullPath.generic_string());
    }

    /*
    @brief Load a replay buffer from a binary file
    */
    void Replay::LoadRecording(const std::string& filename) {
        // --- SAFE PATH RESOLUTION ---
#if USE_IMGUI_EDITOR
        std::filesystem::path dirPath = std::filesystem::path(GetGameRoot()) / "assets" / "replays";
#else
        std::filesystem::path dirPath = std::filesystem::path(GetLocalAppData()) / "replays";
#endif

        std::filesystem::path fullPath = dirPath / (filename + ".bin");
        std::ifstream in(fullPath, std::ios::binary);

        if (!in.is_open()) {
            LOGE(Core, "[Replay] Failed to load replay file: " + fullPath.generic_string());
            return;
        }

        // 1. Read how many events are in the file
        size_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(size_t));

        // 2. Resize our vector and read the data straight into memory
        replayData.resize(count);
        in.read(reinterpret_cast<char*>(replayData.data()), count * sizeof(InputEvent));

        in.close();
        LOGI(Core, "[Replay] Loaded recording from: " + fullPath.generic_string() + " (" + std::to_string(count) + " events)");
    }
}