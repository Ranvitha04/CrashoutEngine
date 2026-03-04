/*!
@file		replaymanager.cpp
@author     lee.r@digipen.edu
@date       03/03/2026
@brief		Maintain code used for tracking gameplay and performing replay
            of past gameplay funtionalities.
*/

#include "replaymanager.h"
#include "log.h"
#include <string>
#include <iostream>

namespace ReplayManager {

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
        LOGI(Core, "[Replay] Started Recording.");
    }

    /*
    @brief "Replay" the events by sending recorded input events
    */
    void Replay::StartPlayback() {
        if (state == ReplayState::PLAYING) return;   // Already playing
        if (state == ReplayState::RECORDING) Stop(); // Stop recording first

        if (replayData.empty()) {
            LOGI(Core, "[Replay] No data to playback!");
            return;
        }
        state = ReplayState::PLAYING;
        playbackIndex = 0;
        InputManager::SetReplayMode(true); // Lock out real hardware!
        LOGI(Core, "[Replay] Started Playback.");
    }

    /*
    @brief Reset the state and stop all playback / recording
    */
    void Replay::Stop() {
        if (state == ReplayState::IDLE) return; // Already stopped

        state = ReplayState::IDLE;
        InputManager::SetReplayMode(false); // Return control to player
        LOGI(Core, "[Replay] Stopped.");
    }

    /*
    @brief Record given event
    */
    void Replay::RecordEvent(Tick tick, Key key, bool isPressed) {
        if (state == ReplayState::RECORDING) {
            replayData.push_back({ tick, key, isPressed });

            int code = static_cast<int>(key);
            std::string stateStr = isPressed ? "Pressed" : "Released";
            std::string logMsg = "[Replay] RECORDED KeyCode: " + std::to_string(code) +
                " | State: " + stateStr +
                " | Tick: " + std::to_string(tick);
            LOGI(Core, logMsg);
        }
    }

    /*
    @brief Updates the state and send input based on current input event and current tick "time"
    */
    void Replay::UpdateFixedTick(Tick currentTick) {
        if (state != ReplayState::PLAYING) return;

        // Process all events scheduled for the current tick
        while (playbackIndex < replayData.size() &&
            replayData[playbackIndex].fixedTick == currentTick)
        {
            InputEvent& ev = replayData[playbackIndex];
            int code = static_cast<int>(ev.key);

            std::string stateStr = ev.isPressed ? "Pressed" : "Released";
            std::string logMsg = "[Replay] Injecting KeyCode: " + std::to_string(code) +
                " | State: " + stateStr +
                " | Tick: " + std::to_string(currentTick);
            LOGI(Core, logMsg);

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
            LOGI(Core, "[Replay] Playback finished.");
            Stop();
        }
    }
}