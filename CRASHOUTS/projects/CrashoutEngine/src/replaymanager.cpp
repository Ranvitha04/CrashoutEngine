/*!
@file		replaymanager.cpp
@author     lee.r@digipen.edu
@date       03/03/2026
@brief		Maintain code used for tracking gameplay and performing replay
            of past gameplay funtionalities.
*/

#include "engine.h"
#include "replaymanager.h"
#include "frameratecontroller.h"
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
    @brief Record current pos & rot
    */
    void Replay::RecordCurrentTransform(float x, float y, float z, float rot) {
        if (!isRecording) return;

        Tick currentTick = FrameRateController::GetInstance().GetFixedTickCount();

        if (s_FirstRecordTick) {
            s_RecordStartTick = currentTick;
            s_FirstRecordTick = false;
        }

        Tick relativeTick = currentTick - s_RecordStartTick;
        recordData.push_back({ relativeTick, x, y, z, rot });
    }

    /*
    @brief Retrieve playback pos & rot
    */
    bool Replay::GetGhostTransform(float& outX, float& outY, float& outZ, float& outRot) {
        if (!isPlaying || playbackData.empty()) return false;

        Tick currentTick = FrameRateController::GetInstance().GetFixedTickCount();

        if (s_FirstPlaybackTick) {
            s_PlaybackStartTick = currentTick;
            s_FirstPlaybackTick = false;
            playbackIndex = 0;
        }

        Tick relativeTick = currentTick - s_PlaybackStartTick;

        // Fast-forward to the correct frame for the current time
        while (playbackIndex < playbackData.size() - 1 &&
            playbackData[playbackIndex + 1].relativeTick <= relativeTick)
        {
            playbackIndex++;
        }

        // Return the exact position and rotation the player was at during this tick!
        const auto& ev = playbackData[playbackIndex];
        outX = ev.x;
        outY = ev.y;
        outZ = ev.z;
        outRot = ev.rotation;

        return true;
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
        s_FirstPlaybackTick = true;
        LOGI(Debug, "[Replay] Started Ghost Playback.");
    }

    /*
    @brief Stop playback
    */
    void Replay::StopPlayback() {
        isPlaying = false;
        LOGI(Debug, "[Replay] Stopped Ghost Playback.");
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
        out.write(reinterpret_cast<const char*>(recordData.data()), count * sizeof(TransformEvent));
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
        in.read(reinterpret_cast<char*>(playbackData.data()), count * sizeof(TransformEvent));
        in.close();

        return true;
    }

}