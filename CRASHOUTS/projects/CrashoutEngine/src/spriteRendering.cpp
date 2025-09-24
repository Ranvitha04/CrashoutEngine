

#include "glapp.h"
#include "rendering.h"
#include "managers/entitymanager.h"
#include "shapes.h"
#include "resources/textures.h"
#include "resources/texture.cpp"
#include "resources/spriteanimation.h"
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>



namespace Component {
    namespace DuckSprite {

        // Duck sprite sheet configuration
        const int SHEET_COLUMNS = 6;
        const int SHEET_ROWS = 3;
        const int FRAME_WIDTH = 32;   // Adjust based on your actual frame size
        const int FRAME_HEIGHT = 32;

        // Frame coordinate arrays (x, y positions in the grid)
        const std::array<std::pair<int, int>, 2> IDLE_NORMAL_FRAMES = { {
            {0, 0}, {1, 0}  // Row 0, columns 0-1
        } };

        const std::array<std::pair<int, int>, 6> WALK_NORMAL_FRAMES = { {
            {2, 0}, {3, 0}, {4, 0}, {5, 0}, {0, 1}, {1, 1}  // Row 0 cols 2-5, Row 1 cols 0-1
        } };

        const std::array<std::pair<int, int>, 4> IDLE_BOUNCE_FRAMES = { {
            {2, 1}, {3, 1}, {4, 1}, {5, 1}  // Row 1, columns 2-5
        } };

        const std::array<std::pair<int, int>, 6> WALK_BOUNCE_FRAMES = { {
            {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2}  // Row 2, columns 0-5
        } };

        // Animation timing
        const float IDLE_FRAME_DURATION = 0.5f;
        const float WALK_FRAME_DURATION = 0.15f;

        class DuckAnimator {
        private:
            Component::SpriteSheetAnimator animator;
            std::string spriteSheetPath;

        public:
            DuckAnimator(const std::string& sheetPath) : spriteSheetPath(sheetPath) {
                SetupAnimations();
            }

            void SetupAnimations() {
                CreateAnimation("idle_normal", IDLE_NORMAL_FRAMES, IDLE_FRAME_DURATION, true, false);
                CreateAnimation("walk_normal", WALK_NORMAL_FRAMES, WALK_FRAME_DURATION, true, false);
                CreateAnimation("idle_bounce", IDLE_BOUNCE_FRAMES, IDLE_FRAME_DURATION, true, false);
                CreateAnimation("walk_bounce", WALK_BOUNCE_FRAMES, WALK_FRAME_DURATION, true, false);
            }

            template<size_t N>
            void CreateAnimation(const std::string& animName,
                const std::array<std::pair<int, int>, N>& frameCoords,
                float frameDuration, bool loop, bool pingPong) {
                Component::SpriteSheetAnimationClip clip(animName, loop, pingPong);

                for (const auto& coord : frameCoords) {
                    Component::SpriteSheetFrame frame(
                        spriteSheetPath, frameDuration,
                        coord.first, coord.second,  // x, y coordinates
                        FRAME_WIDTH, FRAME_HEIGHT,
                        SHEET_COLUMNS, SHEET_ROWS
                    );
                    clip.AddFrame(frame);
                }

                animator.AddAnimation(clip);
            }

            // Animation control methods
            void PlayIdleNormal() { animator.Play("idle_normal"); }
            void PlayWalkNormal() { animator.Play("walk_normal"); }
            void PlayIdleBounce() { animator.Play("idle_bounce"); }
            void PlayWalkBounce() { animator.Play("walk_bounce"); }

            void Pause() { animator.Pause(); }
            void Stop() { animator.Stop(); }

            // Transform methods
            void SetPosition(const CEVec3& pos) { animator.SetPosition(pos); }
            void SetSize(const CEVec2& size) { animator.SetSize(size); }
            void SetScale(float scale) { animator.SetScale(scale); }

            // Update and render
            void Update(float deltaTime) { animator.Update(deltaTime); }
            void Render() { animator.Render(); }

            // State queries
            bool IsPlaying() const { return animator.IsPlaying(); }
            std::string GetCurrentAnimation() const { return animator.GetCurrentClipName(); }
            float GetProgress() const { return animator.GetProgress(); }
        };
    }

    void SetupDuckWithSpriteSheet() {
        DuckSprite::DuckAnimator duck("sprite_sheet/ducky2_spritesheet.png");

        duck.SetPosition(CEVec3(100.0f, 100.0f, 0.0f));
        duck.SetSize(CEVec2(64.0f, 64.0f));
        duck.SetScale(2.0f);

        duck.PlayIdleNormal();
    }
}