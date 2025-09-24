
/*!
//@file        spriteanimation.h
//@author      ranvitha@digipen.edu
//@date        12/09/2025
//
//This file implements sprite animation functionality using the existing
//TextureRenderer and Rendering systems. Provides frame-based animation
//with configurable timing, looping, and playback controls.

/*__________________________________________________________________________*/

#ifndef SPRITE_ANIMATION_H
#define SPRITE_ANIMATION_H

#include "math/CEVec3.h"
#include "math/CEVec2.h"
#include "resources/textures.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Component {

    /*!
    @brief Represents a single frame in a sprite animation sequence.
    */
    struct SpriteFrame {
        std::string texturePath;    // Path to the texture file
        float duration;             // How long to display this frame (in seconds)
        CEVec2 uvOffset;           // UV offset for sprite sheet (optional)
        CEVec2 uvSize;             // UV size for sprite sheet (optional, default 1,1)

        SpriteFrame(const std::string& path, float dur = 0.1f,
            const CEVec2& offset = CEVec2(0, 0),
            const CEVec2& size = CEVec2(1, 1))
            : texturePath(path), duration(dur), uvOffset(offset), uvSize(size) {
        }
    };

    /*!
    @brief Defines a complete animation sequence with multiple frames.
    */
    struct AnimationClip {
        std::string name;
        std::vector<SpriteFrame> frames;
        bool loop;
        bool pingPong;              // Play forward then backward
        float totalDuration;        // Calculated automatically

        AnimationClip(const std::string& clipName = "default", bool shouldLoop = true, bool shouldPingPong = false)
            : name(clipName), loop(shouldLoop), pingPong(shouldPingPong), totalDuration(0.0f) {
        }

        void AddFrame(const SpriteFrame& frame) {
            frames.push_back(frame);
            totalDuration += frame.duration;
        }

        void AddFrame(const std::string& texturePath, float duration = 0.1f) {
            AddFrame(SpriteFrame(texturePath, duration));
        }
    };

    /*!
    @brief Manages sprite animation playback and rendering.
    */
    class SpriteAnimator {
    private:
        std::unordered_map<std::string, AnimationClip> animations;
        std::string currentClipName;
        float currentTime;
        int currentFrameIndex;
        bool isPlaying;
        bool isReversed;           // For ping-pong animations
        float playbackSpeed;

        // Transform properties
        CEVec3 position;
        CEVec2 size;
        float rotation;
        float scale;
        CEVec3 tint;
        bool modulate;

        // Find current frame based on time
        void UpdateCurrentFrame();

    public:
        SpriteAnimator();
        ~SpriteAnimator() = default;

        // Animation management
        void AddAnimation(const AnimationClip& clip);
        void AddAnimation(const std::string& name, const std::vector<std::string>& texturePaths,
            float frameDuration = 0.1f, bool loop = true, bool pingPong = false);

        // Playback controls
        void Play(const std::string& clipName = "");
        void Pause();
        void Stop();
        void Reset();
        void SetPlaybackSpeed(float speed) { playbackSpeed = speed; }

        // Transform setters
        void SetPosition(const CEVec3& pos) { position = pos; }
        void SetSize(const CEVec2& sz) { size = sz; }
        void SetRotation(float rot) { rotation = rot; }
        void SetScale(float scl) { scale = scl; }
        void SetTint(const CEVec3& tintColor) { tint = tintColor; }
        void SetModulate(bool mod) { modulate = mod; }

        // Getters
        bool IsPlaying() const { return isPlaying; }
        const std::string& GetCurrentClip() const { return currentClipName; }
        int GetCurrentFrame() const { return currentFrameIndex; }
        float GetProgress() const; // Returns 0.0 to 1.0

        // Update and render
        void Update(float deltaTime);
        void Render();

        // Utility functions
        void PreloadAllTextures(); // Preload all animation textures
        static SpriteAnimator CreateFromSpriteSheet(const std::string& sheetPath,
            int frameWidth, int frameHeight,
            int columns, int rows,
            float frameDuration = 0.1f,
            bool loop = true);
    };

    /*!
    @brief Static utility functions for common sprite animation tasks.
    */
    struct SpriteAnimation {
        // Create simple animation from numbered sequence (e.g., "walk_01.png", "walk_02.png", etc.)
        static AnimationClip CreateFromSequence(const std::string& basePath,
            const std::string& prefix,
            const std::string& extension,
            int startFrame, int endFrame,
            float frameDuration = 0.1f,
            bool loop = true);

        // Create animation from sprite sheet
        static AnimationClip CreateFromSpriteSheet(const std::string& name,
            const std::string& sheetPath,
            int frameWidth, int frameHeight,
            int columns, int rows,
            float frameDuration = 0.1f,
            bool loop = true);

        // Create simple animated sprite at position
        static void CreateAnimatedSprite(const std::string& clipName,
            const std::vector<std::string>& texturePaths,
            const CEVec3& position,
            const CEVec2& size = CEVec2(10.0f, 10.0f),
            float frameDuration = 0.1f,
            bool loop = true);
    };

} // namespace Component

#endif // SPRITE_ANIMATION_H