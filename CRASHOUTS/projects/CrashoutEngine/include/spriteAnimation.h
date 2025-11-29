#pragma once

/*!
@file   spriteAnimation.h
@author ranvitha@digipen.edu
@date   30/09/2025

@brief  This file contains the declaration of the `Animator` class and the
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

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
__________________________________________________________________________*/





/*                                                                   includes
----------------------------------------------------------------------------- */

#include <string>
#include <unordered_map>
#include <GL/glew.h>
#include "entitymanager.h"

#include "CEVec2.h" 

namespace Component {
    
    // Data structure for one animation (Idle, Walk, Jump)
    struct SpriteClip {
        std::string name;
        int startFrame;
        int frameCount;
        float fps; // or frameTime
        bool loop;
    };

    // The Component attached to the Entity
    struct SpriteAnimatorComponent {
        // A library of animations for this entity
        std::vector<SpriteClip> clips;

        // Playback state
        std::string currentClipName;
        int currentFrameIndex = 0;
        float timer = 0.0f;
        bool playing = true;
        bool flipH = false;

        // Helper to add a clip
        void AddClip(const std::string& name, int start, int count, float fps, bool loop) {
            // Remove if exists, or just push back
            clips.push_back({ name, start, count, fps, loop });
        }

        // Helper to find a clip
        const SpriteClip* GetClip(const std::string& name) const {
            for (const auto& c : clips) {
                if (c.name == name) return &c;
            }
            return nullptr;
        }
    };

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
    @class Animator
    @brief Handles 2D sprite sheet animations and rendering.

    Provides methods for initializing sprite sheets, managing multiple
    animation states, updating animations per frame, and rendering sprites
    with transformations such as position, scale, rotation, and horizontal
    flipping.
    */

    class Animator {
    public:

        /*!
        @brief Construct a new Sprite instance.
        */
        Animator();

        /*!
        @brief Destructor that cleans up sprite resources.
        */
    	~Animator();

        void Play(const std::string& stateName);
        void SetBool(const std::string& key, bool value);
        bool GetBool(const std::string& key) const;
        void SetTrigger(const std::string& key);
        void SetFloat(const std::string& key, float value);
        void AddState(const std::string& name, const AnimationState& state);
        const std::string& GetCurrentState() const { return currentState; }

        void SetFlip(bool flip);

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

        static Animator m_blinkSprite;
        static Animator m_walkSprite;
        static Animator m_sprite;
        static Animator* m_currentSprite;
        static bool m_spriteMode;

    private:
        GLuint textureId;

        int framesPerRow, framePerColumn;
        int animationFrame;
        int currentFrame;
        float elapsedTime;
        float scale, rotation;
        bool initialised;
        bool flipHorizontal;


        int numFrames;
        float frameTime, posX, posY;
        bool initialized;
        
        // Animation state system
        std::unordered_map<std::string, AnimationState> states;
        std::string currentAnimationState;
        std::string currentState;
        AnimationState* stateRef = nullptr;
        //int animationFrame;  // Current frame within the animation (0 to frameCount-1)

        std::unordered_map<std::string, bool> boolParams;
        std::unordered_map<std::string, float> floatParams;
        std::unordered_map<std::string, bool> triggerParams;
    };
    void SetSpriteSheet(CrashoutsEngine::Entity e,
        const std::string& assetName,
        int cols, int rows,
        CEVec2 worldSize);

    void UpdateSpriteAnimationsECS();
    void RenderSpriteAnimationsECS();

    void AnimatorPlayOnSprite(CrashoutsEngine::Entity e,
        const std::string& stateName);

    void AnimatorSetFlipOnSprite(CrashoutsEngine::Entity e, bool flip);
}
