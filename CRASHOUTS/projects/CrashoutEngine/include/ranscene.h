/*!
@file       ran.h
@author     ranvitha@digipen.edu
@date       05/09/2025

@brief      Scene management and rendering functions for Ran's test scene.
@details    This header defines the interface for Ran's test scene, which demonstrates
            various rendering capabilities of the Crashout Engine including:
            - Basic shape rendering (squares, rectangles, triangles, lines)
            - Textured background rendering
            - Instanced rendering for multiple objects
            - Sprite animation system with duck character
            - Input handling for scene interaction
            - Multiple display modes for testing different rendering features

            The scene supports cycling through different display modes:
            - Mode 0: Clear background with basic shapes
            - Mode 1: JPG background rendering
            - Mode 2: Instanced rendering demonstration
            - Mode 3: Sprite animation showcase

            This scene serves as a comprehensive test bed for the engine's
            2D rendering capabilities and animation systems.
//__________________________________________________________________________*/

#pragma once

#include "rendering.h"
#include "shapes.h"
#include "spriteAnimation.h"
#include "CEVec3.h"
#include <vector>
#include <array>

/*                                                      function declarations
----------------------------------------------------------------------------- */

/*!
@brief Load resources and initialize ECS for Ran's scene.
@details
Initializes the Entity Component System (ECS) and resets it for the scene.
This function should be called when transitioning to Ran's scene.
*/
void LoadRan();

/*!
@brief Initialize Ran's scene components and systems.
@details
Sets up instancing system, sprite animations, and other scene-specific
components. Called after LoadRan() to prepare the scene for rendering.
*/
void InitRan();

/*!
@brief Update Ran's scene logic per frame.
@details
Updates sprite animations, handles movement logic, and processes any
time-dependent scene elements. Called every frame during the game loop.
*/
void UpdateRan();

/*!
@brief Render Ran's scene based on current display mode.
@details
Renders the scene according to the current display mode setting.
Handles different rendering paths for shapes, backgrounds, instancing,
and sprite animations.
*/
void RenderRan();

/*!
@brief Free scene-specific resources.
@details
Releases any dynamically allocated resources used by the scene.
Called when cleaning up the scene.
*/
void FreeRan();

/*!
@brief Unload scene resources and cleanup.
@details
Performs final cleanup and resource deallocation for the scene.
Called when transitioning away from Ran's scene.
*/
void UnloadRan();

/*!
@brief Cycle through different display modes.
@details
Switches between the four available display modes:
0 - Clear background with shapes, 1 - JPG background,
2 - Instanced rendering, 3 - Sprite animation.
*/
void CycleDisplayMode();

/*!
@brief Initialize the instanced rendering system.
@details
Sets up data structures and buffers for efficient rendering of multiple
objects using OpenGL instancing. Prepares position, color, rotation,
and scale arrays for batch rendering.
*/
void InitializeInstancing();

/*                                                      render functions
----------------------------------------------------------------------------- */

/*!
@brief Render basic geometric shapes.
@details
Renders various geometric primitives including squares, rectangles,
triangles, and lines using the shapes rendering system.
Used in display mode 0.
*/
void RenderShapes();

/*!
@brief Render textured background.
@details
Renders a textured background using JPG image assets.
Used in display mode 1 to demonstrate texture rendering capabilities.
*/
void RenderBackground();

/*!
@brief Render multiple objects using instancing.
@details
Demonstrates efficient rendering of multiple similar objects using
OpenGL instancing techniques. Renders many squares with varying
positions, colors, rotations, and scales.
Used in display mode 2.
*/
void RenderInstancing();

/*!
@brief Render sprite animation.
@details
Renders the animated duck sprite with current animation state.
Handles sprite positioning, scaling, rotation, and frame updates.
Used in display mode 3.
*/
void RenderSpriteAnimation();

/*                                                      sprite functions
----------------------------------------------------------------------------- */

/*!
@brief Initialize sprite animation system.
@details
Sets up the duck sprite with sprite sheet data, animation states,
and initial positioning. Configures different animation sequences
including idle, walk, and bounce animations.
*/
void InitializeSpriteAnimation();

/*!
@brief Update sprite animation logic.
@details
Updates sprite position based on movement input, manages animation
state transitions, and processes sprite transformations including
horizontal flipping based on movement direction.
*/
void UpdateSpriteAnimation();

/*!
@brief Handle keyboard input for sprite control.
@param key The keyboard key that was pressed/released
@param action The action performed (press, release, repeat)
@details
Processes keyboard input for sprite movement and animation control.
Handles movement keys and updates movement state flags accordingly.
*/
void HandleSpriteKeyInput(int key, int action);

// Extern declarations for global variables 
extern Component::Rendering model;
extern Component::Shapes shapes;
extern Component::Sprite m_sprite;

extern std::array<CEVec3, 4> clr_vtx;
extern std::array<CEVec3, 4> InitVtx;
extern std::vector<CEVec3> current_positions;
extern std::vector<CEVec3> current_colors;

// Transformation and movement variables
extern float TestRotation;
extern float RightTest;
extern float TestScale;
extern bool isMovingLeft;
extern bool isMovingRight;
extern float duckFacingDirection;
extern bool isJump;