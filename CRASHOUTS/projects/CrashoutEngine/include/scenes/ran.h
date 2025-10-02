#pragma once

#include "rendering.h"
#include "shapes.h"
#include "spriteAnimation.h"
#include "math/CEVec3.h"
#include <vector>
#include <array>

// Function declarations
void LoadRan();
void InitRan();
void UpdateRan();
void RenderRan();
void FreeRan();
void UnloadRan();

void CycleDisplayMode();
void InitializeInstancing();

// Sprite animation functions
void InitializeSpriteAnimation();
void UpdateSpriteAnimation();
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