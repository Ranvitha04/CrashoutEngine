/*!
@file		replaymanager.h
@author     lee.r@digipen.edu
@date       03/03/2026
@brief		Maintain code used for tracking gameplay and performing replay
            of past gameplay funtionalities.
*/
#pragma once

#include "inputmanager.h"

namespace ReplayManager {

    using Tick = uint32_t;

    /*
    @struct InputEvent
    @brief Handles simple data like time, key, and pressed state
    */
    struct InputEvent {
        Tick fixedTick; // The exact physics frame this happened
        Key key;            // Key Code
        bool isPressed;     // true = pressed, false = released
    };

    // Replay states
    enum class ReplayState { IDLE, RECORDING, PLAYING };

    /*
    @class Replay
    @brief Store replay function callbacks and track current state
    */
    class Replay {
    public:
        // Singleton access
        static Replay& GetInstance() {
            static Replay instance;
            return instance;
        }

        // Track input events
        void StartRecording();
        // Play using stored input events
        void StartPlayback();
        // Stop all recording / playback
        void Stop();

        // Update loop for replay items
        void UpdateFixedTick(Tick currentTick);

        // Method to capture inputs
        void RecordEvent(Tick tick, Key key, bool isPressed);

        // Getter to check state
        ReplayState GetState() const { return state; }

        // Save recording
        void SaveRecording(const std::string& filename);

        // Load recording
        void LoadRecording(const std::string& filename);

    private:
        ReplayState state = ReplayState::IDLE;
        // Tracks curernt playback index / tick
        size_t playbackIndex = 0;
        // Vector to store event data
        std::vector<InputEvent> replayData;
    };

}