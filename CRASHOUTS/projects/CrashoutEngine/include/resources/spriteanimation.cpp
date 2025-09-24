
#include "spriteanimation.h"
#include <array>

// Extended SpriteFrame to support UV coordinates for sprite sheets
namespace Component {

    struct SpriteSheetFrame {
        std::string texturePath;
        float duration;
        int frameX, frameY;  // Grid coordinates in sprite sheet
        int frameWidth, frameHeight;  // Size of individual frame
        int sheetColumns, sheetRows;  // Total grid dimensions

        SpriteSheetFrame(const std::string& path, float dur, int x, int y,
            int fWidth, int fHeight, int cols, int rows)
            : texturePath(path), duration(dur), frameX(x), frameY(y),
            frameWidth(fWidth), frameHeight(fHeight),
            sheetColumns(cols), sheetRows(rows) {
        }

        // Calculate UV coordinates for this frame
        void GetUVCoordinates(float& uMin, float& vMin, float& uMax, float& vMax) const {
            float frameWidthUV = 1.0f / sheetColumns;
            float frameHeightUV = 1.0f / sheetRows;

            uMin = frameX * frameWidthUV;
            vMin = frameY * frameHeightUV;
            uMax = uMin + frameWidthUV;
            vMax = vMin + frameHeightUV;
        }
    };

    // Extended AnimationClip for sprite sheets
    class SpriteSheetAnimationClip {
    public:
        std::string name;
        std::vector<SpriteSheetFrame> frames;
        float totalDuration;
        bool loop;
        bool pingPong;

        SpriteSheetAnimationClip(const std::string& clipName, bool isLoop = true, bool isPingPong = false)
            : name(clipName), totalDuration(0.0f), loop(isLoop), pingPong(isPingPong) {
        }

        void AddFrame(const SpriteSheetFrame& frame) {
            frames.push_back(frame);
            totalDuration += frame.duration;
        }
    };

    // Enhanced SpriteAnimator for sprite sheets
    class SpriteSheetAnimator {
    private:
        std::map<std::string, SpriteSheetAnimationClip> animations;
        std::string currentClipName;
        float currentTime;
        int currentFrameIndex;
        bool isPlaying;
        bool isReversed;
        float playbackSpeed;

        // Transform properties
        CEVec3 position;
        CEVec2 size;
        float rotation;
        float scale;
        CEVec3 tint;
        bool modulate;

    public:
        SpriteSheetAnimator()
            : currentTime(0.0f), currentFrameIndex(0), isPlaying(false),
            isReversed(false), playbackSpeed(1.0f),
            position(0.0f, 0.0f, 0.0f), size(32.0f, 32.0f),
            rotation(0.0f), scale(1.0f), tint(1.0f, 1.0f, 1.0f), modulate(false) {
        }

        void AddAnimation(const SpriteSheetAnimationClip& clip) {
            animations[clip.name] = clip;
            if (currentClipName.empty()) {
                currentClipName = clip.name;
            }
        }

        void Play(const std::string& clipName) {
            if (!clipName.empty() && animations.find(clipName) != animations.end()) {
                if (currentClipName != clipName) {
                    currentClipName = clipName;
                    Reset();
                }
            }
            isPlaying = true;
        }

        void Pause() { isPlaying = false; }
        void Stop() { isPlaying = false; Reset(); }
        void Reset() { currentTime = 0.0f; currentFrameIndex = 0; isReversed = false; }

        void Update(float deltaTime) {
            if (!isPlaying || animations.find(currentClipName) == animations.end()) {
                return;
            }

            currentTime += deltaTime * playbackSpeed;
            UpdateCurrentFrame();
        }

        void UpdateCurrentFrame() {
            if (animations.find(currentClipName) == animations.end()) return;

            const SpriteSheetAnimationClip& clip = animations.at(currentClipName);
            if (clip.frames.empty()) return;

            // Handle animation timing and looping logic
            if (currentTime >= clip.totalDuration) {
                if (clip.loop) {
                    if (clip.pingPong) {
                        isReversed = !isReversed;
                        currentTime = 0.0f;
                    }
                    else {
                        currentTime = fmod(currentTime, clip.totalDuration);
                    }
                }
                else {
                    currentFrameIndex = static_cast<int>(clip.frames.size()) - 1;
                    isPlaying = false;
                    return;
                }
            }

            // Find current frame based on time
            float accumulatedTime = 0.0f;
            for (size_t i = 0; i < clip.frames.size(); ++i) {
                size_t frameIdx = isReversed ? clip.frames.size() - 1 - i : i;
                if (currentTime >= accumulatedTime &&
                    currentTime < accumulatedTime + clip.frames[frameIdx].duration) {
                    currentFrameIndex = static_cast<int>(frameIdx);
                    return;
                }
                accumulatedTime += clip.frames[frameIdx].duration;
            }
        }

        void Render() {
            if (animations.find(currentClipName) == animations.end()) return;

            const SpriteSheetAnimationClip& clip = animations.at(currentClipName);
            if (clip.frames.empty() || currentFrameIndex < 0 ||
                currentFrameIndex >= static_cast<int>(clip.frames.size())) {
                return;
            }

            const SpriteSheetFrame& frame = clip.frames[currentFrameIndex];

            // Calculate UV coordinates
            float uMin, vMin, uMax, vMax;
            frame.GetUVCoordinates(uMin, vMin, uMax, vMax);

            // Render with UV coordinates (you'll need to extend TextureRenderer for this)
            TextureRenderer::CreateTexturedQuadUV(frame.texturePath, position, size,
                rotation, scale, tint, modulate,
                uMin, vMin, uMax, vMax);
        }

        // Setters
        void SetPosition(const CEVec3& pos) { position = pos; }
        void SetSize(const CEVec2& s) { size = s; }
        void SetRotation(float rot) { rotation = rot; }
        void SetScale(float sc) { scale = sc; }
        void SetTint(const CEVec3& t) { tint = t; }

        // Getters
        bool IsPlaying() const { return isPlaying; }
        std::string GetCurrentClipName() const { return currentClipName; }
        float GetProgress() const {
            if (animations.find(currentClipName) == animations.end()) return 0.0f;
            const SpriteSheetAnimationClip& clip = animations.at(currentClipName);
            return clip.totalDuration > 0.0f ? currentTime / clip.totalDuration : 0.0f;
        }
    };
}