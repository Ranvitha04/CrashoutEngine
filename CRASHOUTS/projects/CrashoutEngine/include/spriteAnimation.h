#pragma once

/*!
@file   spriteAnimation.h
@author ranvitha@digipen.edu
@date   30/09/2025

@brief  This file contains the declaration of the `Sprite` class and the
        `AnimationState` structure within the `Component` namespace. These
        provide the interface for handling 2D sprite animations and rendering
        in a game engine.

        The file defines:
        - **AnimationState**: Represents an animation sequence with
          configurable start frame, frame count, frame timing, and looping
          behavior.
        - **Sprite**: A class responsible for initializing sprite sheets,
          managing animation states, updating animations, and rendering frames
          with transformations such as position, rotation, scale, and
          horizontal flipping.

        Additionally, predefined static sprite instances
        (`m_blinkSprite`, `m_walkSprite`, `m_sprite`) and management
        variables (`m_currentSprite`, `m_spriteMode`) are provided for
        quick access and sprite state switching.

        This module integrates with the engine�s rendering system and
        dependencies (`entitymanager`, `rendering`, and
        `textures`) to support a component-based entity system
        with animated 2D graphics.
__________________________________________________________________________*/



/*                                                                   includes
----------------------------------------------------------------------------- */

#include <string>
#include <unordered_map>
#include <GL/glew.h>
#include "entitymanager.h"



namespace Component {
    
    /*!
    @struct AnimationState
    @brief Represents a single animation sequence within a sprite sheet.

    Each state contains a starting frame, frame count, frame duration, and
    whether the animation should loop or stop after finishing.
    */
    struct AnimationState {
        int startFrame;      
        int frameCount;      
        float frameTime;     
        bool loop; 

        /*!
        @brief Construct an AnimationState with optional parameters.
        @param start Starting frame index (default = 0)
        @param count Number of frames in the animation (default = 1)
        @param time Duration per frame in seconds (default = 0.1f)
        @param shouldLoop Whether the animation loops (default = true)
        */
        
        AnimationState(int start = 0, int count = 1, float time = 0.1f, bool shouldLoop = true)
            : startFrame(start), frameCount(count), frameTime(time), loop(shouldLoop) {}
    };

    /*!
    @class Sprite
    @brief Handles 2D sprite sheet animations and rendering.

    Provides methods for initializing sprite sheets, managing multiple
    animation states, updating animations per frame, and rendering sprites
    with transformations such as position, scale, rotation, and horizontal
    flipping.
    */

    class Sprite {
    public:

        /*!
        @brief Construct a new Sprite instance.
        */
        Sprite();

        /*!
        @brief Destructor that cleans up sprite resources.
        */
         ~Sprite();

        /*!
        @brief Initialize a sprite with a sprite sheet.
        @param spriteSheetPath File path to the sprite sheet texture
        @param rows Number of rows in the sprite sheet
        @param cols Number of columns in the sprite sheet
        @param frameTimeSeconds Duration of each frame in seconds
        */
        void init(const std::string& spriteSheetPath, int rows, int cols, float frameTimeSeconds);

        /*!
        @brief Update the current animation state.
        @param deltaTime Time elapsed since the last update (in seconds)
        */
        void update(float deltaTime);

        /*!
        @brief Render the current frame of the sprite.
        */
        void render();

        /*!
        @brief Release OpenGL texture and cleanup resources.
        */
        void cleanup();
        
        /*!
        @brief Add a new animation state to the sprite.
        @param stateName Name of the animation state (e.g., "walk", "idle")
        @param state The AnimationState object containing animation data
        */
        void addAnimationState(const std::string& stateName, const AnimationState& state);

        /*!
        @brief Switch to a specified animation state by name.
        @param stateName The name of the animation state to activate
        */
        void setAnimationState(const std::string& stateName);


        /*!
        @brief Set the sprite's world position.
        @param x X-coordinate
        @param y Y-coordinate
        */
        void setPosition(float x, float y);

        /*!
        @brief Scale the sprite uniformly.
        @param newScale The scaling factor (1.0f = original size)
        */
        void setScale(float newScale);

        /*!
        @brief Set the rotation angle of the sprite.
        @param newRotation The rotation angle in degrees
        */
        void setRotation(float newRotation);

        /*!
        @brief Enable or disable horizontal flipping.
        @param flip True to flip horizontally, false otherwise
        */
        void setFlipHorizontal(bool flip);
        
        /*!
        @brief Configure duck-specific animations (predefined setup).
        */
        void setupDuckAnimations();

        static Sprite m_blinkSprite;
        static Sprite m_walkSprite;
        static Sprite m_sprite;
        static Sprite* m_currentSprite;
        static bool m_spriteMode;

    private:
        GLuint textureId;
        int numFrames, currentFrame, framesPerRow, framePerColumn;
        float frameTime, elapsedTime, scale, rotation, posX, posY;
        bool initialized, flipHorizontal;
        
        // Animation state system
        std::unordered_map<std::string, AnimationState> animationStates;
        std::string currentAnimationState;
        AnimationState* currentState;
        int animationFrame;  // Current frame within the animation (0 to frameCount-1)
    };
}
