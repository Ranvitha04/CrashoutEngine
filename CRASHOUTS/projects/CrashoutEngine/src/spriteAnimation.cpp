/*!
@file   sprite.cpp
@author ranvitha@digipen.edu
@date   30/09/2025

@brief  This file contains the implementation of the `Sprite` class within
        the `Component` namespace. The `Sprite` class provides functionality
        for handling 2D sprite animations, rendering, and animation state
        management in a game engine.

        The class supports:
        - Initializing sprite sheets and loading textures
        - Managing multiple animation states with frame-based playback
        - Updating animations over time (with loop/non-loop support)
        - Rendering sprites with position, rotation, scale, and horizontal flip
        - Predefined animation setups (e.g., duck character animations)

        Integration with the `TextureRenderer` system allows efficient
        rendering of sprite frames using OpenGL. This system is designed
        to work in conjunction with other engine modules such as
        `entitymanager`, `rendering`, and `resources/textures` to support
        a component-based entity system.
__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "spriteAnimation.h"
#include "rendering.h"
#include "resources/textures.h"
#include "managers/entitymanager.h"
#include "math/CEVec2.h"
#include "math/CEVec3.h"
#include <GL/glew.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Component {

    Sprite Sprite::m_blinkSprite;
    Sprite Sprite::m_walkSprite;
    Sprite Sprite::m_sprite;
    Sprite* Sprite::m_currentSprite = nullptr;
    bool Sprite::m_spriteMode = false;

   /*!
   @brief Default constructor for the Sprite class.
   Initializes all internal state variables to default values and ensures the sprite
   starts in an uninitialized state.
   */

    Sprite::Sprite()
        : textureId(0),
        numFrames(1), currentFrame(0), framesPerRow(1), framePerColumn(1),
        frameTime(0.5f), elapsedTime(0.0f),
        scale(1.0f), rotation(0.0f), posX(0.0f), posY(0.0f),
        initialized(false), flipHorizontal(false), currentState(nullptr), animationFrame(0) {
    }

    /*!
    @brief Destructor for the Sprite class.
    Automatically calls `cleanup()` to release any allocated resources and reset the state.
    */

    Sprite::~Sprite() { cleanup(); }

   /*!
   @brief Initializes the sprite with a sprite sheet texture and animation properties.

   @param spriteSheetPath Path to the sprite sheet image file.
   @param rows Number of rows in the sprite sheet.
   @param cols Number of columns in the sprite sheet.
   @param frameTimeSeconds Duration (in seconds) of each animation frame.

   @details
   This function loads the sprite sheet texture, calculates frame count and layout,
   and sets up the sprite for rendering. If called more than once, it will clean up
   the previous state before reinitializing.
   */

    void Sprite::init(const std::string& spriteSheetPath, int rows, int cols, float frameTimeSeconds) {
        if (initialized) {
            cleanup();
        }

        scale = 10.0f;    // Use normal scale (will be adjusted by setScale)
        posX = 0.0f;     // Center position
        posY = 0.0f;     // Center position
        rotation = 0.0f;

        // Use the TextureRenderer system to load texture
        textureId = TextureRenderer::GetTextureHandle(spriteSheetPath);
        if (textureId == 0) {
            std::cerr << "[ERROR] Failed to load sprite sheet: " << spriteSheetPath << std::endl;
            return;
        }

        framesPerRow = cols;
        framePerColumn = rows;
        numFrames = rows * cols;

        frameTime = frameTimeSeconds;
        currentFrame = 0;
        elapsedTime = 0.0f;

        initialized = true;
    }

   /*!
   @brief Updates the current animation frame based on elapsed time.

   @param deltaTime Time (in seconds) since the last update call.

   @details
   This function advances the animation frame when the accumulated time exceeds
   the frame duration. It respects whether the animation is looping or not and
   updates the `currentFrame` index accordingly.
   */

    void Sprite::update(float deltaTime) {
        if (!initialized) {
            return;
        }
        if (!currentState) {
            return;
        }

        elapsedTime += deltaTime;
        if (elapsedTime >= frameTime) {
            if (currentState->loop) {
                animationFrame = (animationFrame + 1) % currentState->frameCount;
            } else {
                if (animationFrame < currentState->frameCount - 1) {
                    animationFrame++;
                }
            }
            
            // Calculate the actual frame index in the sprite sheet
            currentFrame = currentState->startFrame + animationFrame;
            elapsedTime = 0.0f;
        }
    }

    /*!
    @brief Renders the current animation frame to the screen.

    @details
    Computes the current frame�s position within the sprite sheet and draws it using
    the texture renderer. Supports horizontal flipping, scaling, rotation, and tinting.
    */

    void Sprite::render() {
        if (!initialized) {
            return;
        }

        // Calculate current frame position in sprite sheet
        int col = currentFrame % framesPerRow;
        int row = currentFrame / framesPerRow;

        // Use the new dynamic TextureRenderer to render the sprite frame
        CEVec3 position(posX, posY, 0.0f);
        CEVec3 tint(1.0f, 1.0f, 1.0f); // White tint (no color change)

        // Use the flipped version if horizontal flipping is enabled
        // Use a more reasonable base size to prevent scaling artifacts
        TextureRenderer::CreateSpriteFrameFlipped(
            textureId,           
            position,            
            CEVec2(12.0f, 12.0f), // Larger base size to reduce scaling factor
            col,                 
            row,                 
            framesPerRow,        // Total columns
            framePerColumn,      // Total rows
            flipHorizontal,      // Horizontal flip flag
            rotation,            // Rotation
            scale * 0.5f,        // Reduce scale factor to compensate for larger base size
            tint,                // Tint color
            true                 // Enable modulation for transparency
        );
    }

    /*!
    @brief Cleans up the sprite�s internal state and resets all properties.

    @details
    Does not delete the texture (as it is managed by the texture renderer), but resets
    internal variables to prepare the sprite for reinitialization or destruction.
    */

    void Sprite::cleanup() {
        if (!initialized) return;

        // Texture is managed by TextureRenderer
        textureId = 0;
        initialized = false;
        currentFrame = 0;
        elapsedTime = 0.0f;
    }

    /*!
    @brief Registers a new animation state for the sprite.

    @param stateName Name of the animation state.
    @param state AnimationState object containing frame range, frame count, and looping info.

    @details
    Animation states define how frames are played for a given action (e.g., walk, idle, attack).
    */

    void Sprite::addAnimationState(const std::string& stateName, const AnimationState& state) {
        animationStates[stateName] = state;
    }

    /*!
   @brief Sets the active animation state for playback.

   @param stateName The name of the animation state to switch to.

   @details
   Resets frame counters and elapsed time when switching states. If the state does not exist,
   the function does nothing.
   */

    void Sprite::setAnimationState(const std::string& stateName) {
        auto it = animationStates.find(stateName);
        if (it != animationStates.end() && currentAnimationState != stateName) {
            currentAnimationState = stateName;
            currentState = &it->second;
            animationFrame = 0;
            elapsedTime = 0.0f;
            frameTime = currentState->frameTime;
        }
    }

   /*!
   @brief Sets the sprite�s position on the screen.
   @param x X-coordinate.
   @param y Y-coordinate.
   */

    void Sprite::setPosition(float x, float y) {
        posX = x;
        posY = y;
    }

   /*!
   @brief Sets the scale factor of the sprite.
   @param newScale New scale multiplier.
   */

    void Sprite::setScale(float newScale) {
        scale = newScale;
    }

    /*!
    @brief Sets the rotation angle of the sprite.
    @param newRotation New rotation in radians or degrees (depending on renderer implementation).
    */

    void Sprite::setRotation(float newRotation) {
        rotation = newRotation;
    }

   
   /*!
   @brief Enables or disables horizontal flipping of the sprite.
   @param flip True to flip horizontally, false for normal orientation.
   */

    void Sprite::setFlipHorizontal(bool flip) {
        flipHorizontal = flip;
    }

    /*!
    @brief Configures default animation states for a duck character sprite sheet.

    @details
    Pre-defines idle and walking animations in both normal and "bounce" styles,
    mapping them to specific rows and frames in the sprite sheet.
    */

    void Sprite::setupDuckAnimations() {
        
        // Row 1: idle_normal (2 frames) = frames 0-1
        // Row 2: walk_normal (6 frames) = frames 6-11  
        // Row 3: idle_bounce (4 frames) = frames 12-15
        // Row 4: walk_bounce (6 frames) = frames 18-23 
        
        // idle_normal: 2 frames (row 1, frames 0-1) - Slower for breathing effect
        addAnimationState("idle_normal", AnimationState(0, 2, 0.8f, true));
        
        // walk_normal: 6 frames (row 2, frames 6-11) - Faster for smooth walking
        addAnimationState("walk_normal", AnimationState(6, 6, 0.12f, true));
        
        // idle_bounce: 4 frames (row 3, frames 12-15) - Medium speed for bounce effect
        addAnimationState("idle_bounce", AnimationState(12, 4, 0.15f, true));
        
        // walk_bounce: Use all 6 frames (row 4, frames 18-23) - Smooth bouncy walk
        addAnimationState("walk_bounce", AnimationState(18, 6, 0.1f, true));
        
        // Set default animation
        setAnimationState("idle_normal");
    }
}