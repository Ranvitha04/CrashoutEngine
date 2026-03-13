/*!
@file		replaymanager.h
@author     Renfred Lee
@date       03/03/2026
@brief		Maintain code used for tracking gameplay and performing replay
            of past gameplay funtionalities.
*/
#pragma once

#pragma once
#include <vector>
#include <string>

namespace ReplayManager {

    typedef uint32_t Tick;

    // --- Records physical location instead of keys ---
    struct TransformEvent {
        Tick relativeTick;
        float x, y, z;
        float rotation;
    };

    /*
    @class Replay
    @brief Store replay function callbacks for record and playback
    */
    class Replay {
    private:
        std::vector<TransformEvent> recordData;     // Buffer A
        std::vector<TransformEvent> playbackData;   // Buffer B

        bool isRecording = false;
        bool isPlaying = false;
        size_t playbackIndex = 0;

        Replay() = default;

    public:
        bool isRecordingState() const { return isRecording; } // Helpful getter

        static Replay& GetInstance() { static Replay instance; return instance; }

        void StartRecording();
        void StopRecording();

        void StartPlayback();
        void StopPlayback();

        // --- NEW TRANSFORM METHODS ---
        void RecordCurrentTransform(float x, float y, float z, float rot);
        bool GetGhostTransform(float& outX, float& outY, float& outZ, float& outRot);

        void SaveRecording(const std::string& filename);
        bool LoadRecording(const std::string& filename);
    };

}