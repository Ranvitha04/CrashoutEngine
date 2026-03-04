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
    private:
        // --- THE DOUBLE BUFFERS ---
        std::vector<InputEvent> recordData;     // Buffer A: The player's current run
        std::vector<InputEvent> playbackData;   // Buffer B: The loaded ghost's run

        std::unordered_map<int, bool> virtualInputState;

        // Independent state flags! They can both be true at the same time.
        bool isRecording = false;
        bool isPlaying = false;

        size_t playbackIndex = 0;

        Replay() = default;

    public:
        static Replay& GetInstance() { static Replay instance; return instance; }

        void StartRecording();
        void StopRecording(); // Replaces generic Stop()

        void StartPlayback();
        void StopPlayback();  // Replaces generic Stop()

        void RecordEvent(Tick tick, Key key, bool isPressed);
        void UpdateFixedTick(Tick currentTick);

        bool IsVirtualKeyPressed(int keycode);

        void SaveRecording(const std::string& filename);
        bool LoadRecording(const std::string& filename); // Still returns bool!
    };

}