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

    bool Replay::IsVirtualKeyPressed(int keycode) {
        // Return true if the key is pressed in the replay buffer
        return virtualInputState[keycode];
    }

    /*
    @brief Declare start of recording
    */
    void Replay::StartRecording() {
        recordData.clear(); // Only clear the RECORD buffer!
        isRecording = true;
        s_FirstRecordTick = true;
        LOGI(Debug, "[Replay] Started Recording.");
    }

    /*
    @brief Stop recording
    */
    void Replay::StopRecording() {
        isRecording = false;
        LOGI(Debug, "[Replay] Stopped Recording.");
    }

    /*
    @brief "Replay" the events by sending recorded input events
    */
    void Replay::StartPlayback() {
        if (playbackData.empty()) {
            LOGI(Debug, "[Replay] No ghost data to playback!");
            return;
        }
        isPlaying = true;
        playbackIndex = 0;
        virtualInputState.clear();
        s_FirstPlaybackTick = true;
        LOGI(Debug, "[Replay] Started Ghost Playback.");
    }

    /*
    @brief Stop playback
    */
    void Replay::StopPlayback() {
        isPlaying = false;
        virtualInputState.clear(); // Release virtual keys
        LOGI(Debug, "[Replay] Stopped Ghost Playback.");
    }

    /*
    @brief Record given event
    */
    void Replay::RecordEvent(Tick tick, Key key, bool isPressed) {
        if (isRecording) {
            int code = static_cast<int>(key);
            if (code >= 290 && code <= 314) return; // Ignore F-keys

            if (s_FirstRecordTick) {
                s_RecordStartTick = tick;
                s_FirstRecordTick = false;
            }
            Tick relativeTick = tick - s_RecordStartTick;

            // Save to RECORD buffer
            recordData.push_back({ relativeTick, key, isPressed });
        }
    }

    /*
    @brief Updates the state and send input based on current input event and current tick "time"
    */
    void Replay::UpdateFixedTick(Tick currentTick) {

        if (!isPlaying) return;

        // --- RELATIVE TIME CONVERSION ---
        if (s_FirstPlaybackTick) {
            s_PlaybackStartTick = currentTick;
            s_FirstPlaybackTick = false;
        }

        Tick relativeCurrentTick = currentTick - s_PlaybackStartTick;

        // Process all events scheduled for the current tick
        while (playbackIndex < playbackData.size() &&
            playbackData[playbackIndex].fixedTick == relativeCurrentTick)
        {
            InputEvent& ev = playbackData[playbackIndex];
            int code = static_cast<int>(ev.key);

            std::string stateStr = ev.isPressed ? "Pressed" : "Released";
            std::string logMsg = "[Replay] Injecting KeyCode: " + std::to_string(code) +
                " | State: " + stateStr +
                " | RelTick: " + std::to_string(relativeCurrentTick);
            LOGI(Debug, logMsg);

            //// Inject based on whether it's a mouse button (0-7) or keyboard key
            //if (code >= 0 && code <= GLFW_MOUSE_BUTTON_LAST) {
            //    InputManager::InjectMouseButton(ev.key, ev.isPressed);
            //}
            //else {
            //    InputManager::InjectKey(ev.key, ev.isPressed);
            //}

            // --- UPDATE VIRTUAL STATE INSTEAD OF GLOBAL INPUT ---
            virtualInputState[code] = ev.isPressed;

            playbackIndex++;
        }

        // Auto-stop when we reach the end of the recording
        if (playbackIndex >= playbackData.size()) {
            LOGI(Debug, "[Replay] Playback finished.");
            StopPlayback();
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
        if (!out.is_open()) return;

        // Save the RECORD data!
        size_t count = recordData.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(recordData.data()), count * sizeof(InputEvent));
        out.close();

        LOGI(Core, "[Replay] Ghost Saved!");
    }

    /*
    @brief Load a replay buffer from a binary file
    */
    bool Replay::LoadRecording(const std::string& filename) {
        // --- SAFE PATH RESOLUTION ---
#if USE_IMGUI_EDITOR
        std::filesystem::path dirPath = std::filesystem::path(GetGameRoot()) / "assets" / "replays";
#else
        std::filesystem::path dirPath = std::filesystem::path(GetLocalAppData()) / "replays";
#endif

        std::filesystem::path fullPath = dirPath / (filename + ".bin");
        std::ifstream in(fullPath, std::ios::binary);
        if (!in.is_open()) return false;

        // Load into the PLAYBACK data!
        size_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(size_t));
        playbackData.resize(count);
        in.read(reinterpret_cast<char*>(playbackData.data()), count * sizeof(InputEvent));
        in.close();

        return true;
    }

}