/*!
@file		videoloader.h
@author     m.biniswandi@digipen.edu
@date       01/02/2026
@brief		declaration of Video Loader using FFmpeg and FMOD.

All content ? 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <string>
#include <GL/glew.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;
namespace FMOD { class Channel; class Sound; }

namespace CrashoutsEngine {

    class VideoLoader {
    public:
        VideoLoader(const std::string& filepath);
        ~VideoLoader();

        bool Load();
        void Update(float dt);
        GLuint GetTextureHandle() const { return textureHandle; }
        bool IsValid() const { return isValid; }
        bool IsPlaying() const { return isPlaying; }
        void Play() { isPlaying = true; }
        void Pause() { isPlaying = false; }
        void Stop();
        void SetLoop(bool loop) { isLooping = loop; }
        void SetVolume(float vol);
        double GetDuration() const { return duration; }
        double GetCurrentTime() const { return currentTime; }
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }

    private:
        void Cleanup();
        bool ReadFrame();

        std::string filepath;
        GLuint textureHandle{ 0 };
        int width{ 0 };
        int height{ 0 };
        bool isValid{ false };
        bool isPlaying{ false };
        bool isLooping{ false };
        
        double currentTime{ 0.0 };
        double duration{ 0.0 };
        double frameTimer{ 0.0 };
        double frameDelay{ 0.033 };
        
        FMOD::Channel* audioChannel{ nullptr };
        FMOD::Sound* audioSound{ nullptr };
        bool hasAudio{ false };

#ifdef USE_FFMPEG
        AVFormatContext* formatCtx{ nullptr };
        AVCodecContext* codecCtx{ nullptr };
        AVCodecContext* audioCodecCtx{ nullptr };
        AVFrame* frame{ nullptr };
        AVFrame* frameRGB{ nullptr };
        AVFrame* audioFrame{ nullptr };
        AVPacket* packet{ nullptr };
        SwsContext* swsCtx{ nullptr };
        SwrContext* swrCtx{ nullptr };
        int videoStreamIndex{ -1 };
        int audioStreamIndex{ -1 };
        uint8_t* buffer{ nullptr };
#endif
    };
}