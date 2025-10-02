#include "scenes/ran.h"
#include "log.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "managers/entitymanager.h"
#include "shapes.h"
#include "spriteAnimation.h"
#include "glapp.h"
#include "rendering.h"

static CrashoutsEngine::ECS* globalECS;


Component::Rendering model;
Component::Shapes shapes;
Component::Sprite m_sprite;


std::array<CEVec3, 4> clr_vtx{
    CEVec3(1.f, 1.f, 1.f), // Center - white
    CEVec3(1.f, 0.f, 0.f), // Bottom-right - red
    CEVec3(0.f, 1.f, 0.f), // Top-right - green
    CEVec3(0.f, 0.f, 1.f)  // Top-left - blue
};

std::array<CEVec3, 4> InitVtx = clr_vtx;


std::vector<CEVec3> current_positions;
std::vector<CEVec3> current_colors;


float TestRotation = 0.0f; // global rotation angle
float RightTest = 0.0f;
float TestScale = 1.0f;


bool isMovingLeft = false;
bool isMovingRight = false;
float duckFacingDirection = 1.0f; // 1.0f = right, -1.0f = left
bool isJump = false;

// Display mode for G key cycling
static int displayMode = 0;
//0  is clear background, 1 is JPG background, 2 is instancing on clear, 3 is sprite animation

// Instancing variables
static std::vector<CEVec3> g_positions;
static std::vector<CEVec3> g_colors;
static std::vector<float> g_rotations;
static std::vector<float> g_scales;
static bool g_instancesInitialized = false;
const int maxObjectCount = 2500;


void LoadRan()
{
    
    globalECS = CrashoutsEngine::ECS::getInstance();
    globalECS->Reset();
}

void InitRan()
{
    LOGI(Core, "Ran Initialized");
    InitializeInstancing();
    InitializeSpriteAnimation();
}


void InitializeSpriteAnimation()
{
    // Init duck sprite 
    Component::Sprite::m_sprite.init("../../../projects/CrashoutEngine/sprite_sheet/ducky_2_spritesheet.png", 4, 6, 0.3f);

    // Setup duck animations
    Component::Sprite::m_sprite.addAnimationState("idle_normal", Component::AnimationState(0, 2, 0.05f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_normal", Component::AnimationState(6, 6, 0.02f, true));
    Component::Sprite::m_sprite.addAnimationState("idle_bounce", Component::AnimationState(12, 4, 0.04f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_bounce", Component::AnimationState(16, 2, 0.015f, true));

    // Set position and scale for the duck
    Component::Sprite::m_sprite.setPosition(0.0f, 0.0f);
    Component::Sprite::m_sprite.setScale(TestScale);
    Component::Sprite::m_sprite.setRotation(TestRotation);

    Component::Sprite::m_sprite.setAnimationState("walk_normal");
}

void UpdateRan()
{
    UpdateSpriteAnimation();
}

// Function to update sprite animation (moved from glapp.cpp)
void UpdateSpriteAnimation()
{
    // Duck movement and animation logic
    float moveSpeed = 15.0f; // world units per second
    static float duckPosX = 0.0f;
    static float duckPosY = 0.0f;
    static std::string currentAnimationState = "walk_normal";

    // Determine what animation should be playing
    std::string targetAnimationState;
    if (isMovingLeft || isMovingRight) {
        float movement = (isMovingRight ? moveSpeed : -moveSpeed) * static_cast<float>(GLApp::delta_time);
        duckPosX += movement;
        targetAnimationState = "walk_normal";
    }
    else {
        targetAnimationState = "idle_bounce";
    }

    // Only change animation state if it's different from current
    if (targetAnimationState != currentAnimationState) {
        Component::Sprite::m_sprite.setAnimationState(targetAnimationState);
        currentAnimationState = targetAnimationState;
    }

    Component::Sprite::m_sprite.setPosition(duckPosX, duckPosY);
    Component::Sprite::m_sprite.setFlipHorizontal(duckFacingDirection < 0.0f);
    Component::Sprite::m_sprite.setScale(TestScale);
    Component::Sprite::m_sprite.setRotation(TestRotation);
    Component::Sprite::m_sprite.update(static_cast<float>(GLApp::delta_time));
}

void RenderRan()
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate world dimensions for background
    float aspect = static_cast<float>(GLApp::width) / GLApp::height;
    float worldWidth, worldHeight;

    if (aspect >= 1.0f) {
        worldHeight = 100.0f;  // BASE_SIZE
        worldWidth = worldHeight * aspect;
    }
    else {
        worldWidth = 100.0f;   // BASE_SIZE
        worldHeight = worldWidth / aspect;
    }



    // Display Mode 1 JPG background
    if (displayMode == 1 ) {
        /*g_instancesInitialized = true;*/
        Component::Shapes::CreateTexturedRectangle(
            "../../../projects/CrashoutEngine/images/duck.jpg",
            CEVec3(0.0f, 0.0f, -0.9f),  // Position at center, behind other objects
            worldWidth,                  // Full world width
            worldHeight,                 // Full world height
            0.0f,                       // No rotation
            1.0f,                       // Normal scale
            CEVec3(1.0f, 1.0f, 1.0f)    // No tint
        );


    }

    // Display Mode 2 Instancing on clear background
    else if (displayMode == 2) {

    
        // Use instanced rendering to draw all squares in one call
        Component::Shapes::CreateInstancedSquares(
            g_positions.data(),
            g_colors.data(),
            g_rotations.data(),
            g_scales.data(),
            maxObjectCount,
            0.5f  // base size
        );
    }

    // Display Mode 3 Sprite animation
    else if (displayMode == 3) {
        Component::Sprite::m_sprite.render();
    }
}

void FreeRan()
{
    // Clean up sprite animation resources
    Component::Sprite::m_sprite.cleanup();
}

void UnloadRan()
{
}


void HandleSpriteKeyInput(int key, int action)
{
    // Duck movement controls
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            isMovingLeft = true;
            isMovingRight = false;
            duckFacingDirection = -1.0f; // Face left 
        }
        else if (action == GLFW_RELEASE) {
            isMovingLeft = false;
        }
    }

    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            isMovingRight = true;
            isMovingLeft = false;
            duckFacingDirection = 1.0f; // Face right
        }
        else if (action == GLFW_RELEASE) {
            isMovingRight = false;
        }
    }

    // Test controls
    if (GLFW_PRESS == action) {
        if (key == GLFW_KEY_R) { // Rotate with 'R'
            TestRotation += 10.0f;  // rotate 10 degrees each press
            if (TestRotation >= 360.0f) TestRotation -= 360.0f;
        }

        if (key == GLFW_KEY_I) {
            RightTest += 25.0f;
            std::cout << RightTest << std::endl;
        }

        if (key == GLFW_KEY_S) { // Scale up with S
            TestScale += 2.f;
            std::cout << "Scale: " << TestScale << std::endl;
        }
        if (key == GLFW_KEY_Q) { // Scale down with q
            TestScale -= 2.f;
            if (TestScale < 0.1f) TestScale = 0.1f;
            std::cout << "Scale: " << TestScale << std::endl;
        }

        if (key == GLFW_KEY_G) {
            CycleDisplayMode();
        }
    }
}


void CycleDisplayMode()
{
    displayMode = (displayMode + 1) % 4; //print out graphic state
    switch (displayMode) {
    case 0:
        std::cout << "Display Mode: Clear background" << std::endl;
        break;
    case 1:
        std::cout << "Display Mode: JPG background" << std::endl;
        break;
    case 2:
        std::cout << "Display Mode: Instancing on clear background" << std::endl;
        break;
    case 3:
        std::cout << "Display Mode: Sprite animation" << std::endl;
        break;
    }
}

// Function to initialize instancing data
void InitializeInstancing()
{
    if (!g_instancesInitialized) {
        g_positions.resize(maxObjectCount);
        g_colors.resize(maxObjectCount);
        g_rotations.resize(maxObjectCount);
        g_scales.resize(maxObjectCount);

        // Initialize with random values
        for (int i = 0; i < maxObjectCount; ++i) {           

            g_positions[i] = CEVec3(
                static_cast<float>(rand() % 200 - 100),  // Random x range from -100 to 100
                static_cast<float>(rand() % 200 - 100),  // Random y range from -100 to 100
                0.0f
            );
            g_colors[i] = CEVec3(
                static_cast<float>(rand()) / RAND_MAX,   // Random red
                static_cast<float>(rand()) / RAND_MAX,   // Random green
                static_cast<float>(rand()) / RAND_MAX    // Random blue
            );
            g_rotations[i] = static_cast<float>(rand() % 360);  // Random rotation
            g_scales[i] = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f;  // Random scale 0.5-2.0
        }

        g_instancesInitialized = true;
        std::cout << "Instancing initialized with " << maxObjectCount << " objects" << std::endl;
    }
}