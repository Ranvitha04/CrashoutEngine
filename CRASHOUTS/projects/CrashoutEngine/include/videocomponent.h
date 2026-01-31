#pragma once
#include <vector>
#include <string>
#include <memory>
#include <GL/glew.h>

namespace CrashoutsEngine { class VideoLoader; }

namespace Component {
    struct VideoComponent {
        std::vector<GLuint> frames;
        float frameRate = 12.0f; // Default to 12fps
        float timer = 0.0f;
        int currentFrameIndex = 0;
        bool isPlaying = false;
        bool loop = false;
        bool isCutscene = false; 
        bool matchAspectRatio = true;
        std::string videoPath = "";

        // New: FFmpeg Support
        std::shared_ptr<CrashoutsEngine::VideoLoader> loader;

        VideoComponent() = default;
    };
}
