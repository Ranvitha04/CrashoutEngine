#pragma once
#include <string>
#include <GL/glew.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

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
        double GetDuration() const { return duration; }
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

#ifdef USE_FFMPEG
        AVFormatContext* formatCtx{ nullptr };
        AVCodecContext* codecCtx{ nullptr };
        AVFrame* frame{ nullptr };
        AVFrame* frameRGB{ nullptr };
        AVPacket* packet{ nullptr };
        SwsContext* swsCtx{ nullptr };
        int videoStreamIndex{ -1 };
        uint8_t* buffer{ nullptr };
#endif
    };
}