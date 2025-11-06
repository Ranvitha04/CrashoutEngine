/*!
@file       ranscene.h
@author     ranvitha@digipen.edu
@date       05/09/2025

@brief      Scene management and rendering functions for Ran's test scene.
@details    This header defines the interface for Ran's test scene, which demonstrates
            various rendering capabilities of the Crashout Engine including:
            - Textured background rendering
            - Sprite animation system with duck character
            - Instanced rendering for rectangles and random shapes
            - ECS-driven random shapes
            - ECS-driven animated ducks
            - Input handling for scene interaction

            The scene supports cycling through multiple display modes (0..6):
            - Mode 0: Background image
            - Mode 1: Duck sprite animation (single duck)
            - Mode 2: Non-ECS instanced rectangles (texture toggle via 'M')
            - Mode 3: Reserved / ECS scene (internal)
            - Mode 4: Non-ECS instanced random shapes (texture toggle)
            - Mode 5: ECS-driven random shapes (circles separate; texture toggle)
            - Mode 6: ECS duck sprites (press 'I' to cycle animation state)

            Press 'G' to cycle display modes and reinitialize mode-specific data
            as needed. In Mode 6, pressing 'I' cycles duck animation states.
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
Initializes scene state and references used by Ran's scene (e.g., ECS handles),
preparing it for setup in InitRan(). Call when transitioning into this scene.
*/
void LoadRan();

/*!
@brief Initialize Ran's scene components and systems.
@details
Sets up instancing parameters, sprite animation resources, and any mode-specific
initialization required before rendering. Called after LoadRan().
*/
void InitRan();

/*!
@brief Update Ran's scene logic per frame.
@details
Runs per-frame updates for active display mode (0..6), processes input, and
dispatches mode-specific logic:
- 'G' cycles display modes (0..6)
- 'M' toggles texture usage where applicable (e.g., modes 2, 4, 5)
- In mode 6 (ECS ducks), 'I' cycles duck animation states
Also updates sprite animation in mode 1 and ECS-driven updates in applicable modes.
*/
void UpdateRan();

/*!
@brief Render Ran's scene based on current display mode.
@details
Renders the scene according to the current display mode (0..6),
including background (0), single duck sprite (1), instanced rectangles (2),
non-ECS random instanced shapes (4), ECS random shapes (5), and ECS ducks (6).
*/
void RenderRan();

/*!
@brief Free scene-specific resources.
@details
Releases temporary or mode-specific data structures and clears instancing arrays.
Ensures sprite resources and ECS references are cleaned before unloading.
*/
void FreeRan();

/*!
@brief Unload scene resources and cleanup.
@details
Performs final cleanup and resource deallocation when transitioning away
from Ran's scene. Safe to call after FreeRan().
*/
void UnloadRan();

/*!
@brief Cycle through different display modes.
@details
Switches between the seven available display modes:
0 - Background, 1 - Single duck sprite,
2 - Instanced rectangles, 3 - Reserved/ECS scene,
4 - Non-ECS random shapes, 5 - ECS random shapes,
6 - ECS duck sprites (supports 'I' for animation cycling).
*/
void CycleDisplayMode();

/*!
@brief Initialize the instanced rendering system.
@details
Configures buffers and arrays supporting batched instanced rendering,
including position, color, rotation, and scale streams. Used by modes
that render multiple objects efficiently via instancing.
*/
void InitializeInstancing();

/*                                                      render functions
----------------------------------------------------------------------------- */

/*!
@brief Render basic geometric shapes.
@details
Renders various geometric primitives including squares, rectangles,
triangles, and lines using the shapes rendering system.
Typically used as a utility in shape-centric modes.
*/
void RenderShapes();

/*!
@brief Render textured background.
@details
Renders a textured background using image assets.
Used in display mode 0.
*/
void RenderBackground();

/*!
@brief Render multiple objects using instancing.
@details
Demonstrates efficient rendering of multiple similar objects using
instancing techniques. Typically used in display mode 2.
*/
void RenderInstancing();

/*!
@brief Render sprite animation.
@details
Renders the animated duck sprite with the current animation state and
transforms. Used in display mode 1 for a single duck sprite.
*/
void RenderSpriteAnimation();

/*                                                      sprite functions
----------------------------------------------------------------------------- */

/*!
@brief Initialize sprite animation system.
@details
Sets up the single duck sprite, sprite sheet data, and animation states
for mode 1. Configures idle, walk, and bounce sequences and initial transform.
*/
void InitializeSpriteAnimation();

/*!
@brief Update sprite animation logic.
@details
Updates sprite transform and animation state for the single duck in mode 1,
including movement responses and optional flipping based on input.
*/
void UpdateSpriteAnimation();

/*!
@brief Handle keyboard input for sprite control.
@param key The keyboard key that was pressed/released.
@param action The action performed (press, release, repeat).
@details
Processes keyboard input for sprite movement and orientation changes in mode 1
(e.g., horizontal flip on A/D or arrow keys). The 'I' animation toggle for ECS
ducks is handled in UpdateRan() while in mode 6.
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