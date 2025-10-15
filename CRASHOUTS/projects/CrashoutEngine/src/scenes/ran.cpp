/*!
@file       ran.cpp
@author     ranvitha@digipen.edu
@date       05/09/2025

@brief      Implementation of Ran's test scene for the Crashout Engine.
@details    This file implements a comprehensive test scene that demonstrates
            various rendering and animation capabilities of the Crashout Engine:

            **Display Modes:**
            - Mode 0: Basic shape rendering with clear background
            - Mode 1: Textured background rendering
            - Mode 2: Instanced rendering for multiple objects
            - Mode 3: Sprite animation with duck character

            **Key Features:**
            - Entity Component System (ECS) integration
            - Multiple geometric primitive rendering
            - Sprite animation system with state management
            - Input handling for character movement
            - Instanced rendering for performance testing
            - Dynamic display mode switching

            **Animation System:**
            The scene includes a duck character with multiple animation states:
            - idle_normal: Basic idle animation
            - walk_normal: Walking animation
            - idle_bounce: Bouncing idle animation
            - walk_bounce: Bouncing walk animation

            **Input Controls:**
            - Arrow keys or WASD for movement
            - G key to cycle display modes
            - Various transformation controls for testing

            This scene serves as both a feature demonstration and a testing
            ground for the engine's 2D rendering capabilities.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "scenes/ran.h"
#include "log.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "managers/entitymanager.h"
#include "shapes.h"
#include "spriteAnimation.h"
#include "glapp.h"
#include "rendering.h"
#include "sceneutils.h"
#include "managers/assetmanager.h"

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


/*!
@brief Load resources and initialize ECS for Ran's scene.
@details
Obtains the singleton ECS instance and resets it to prepare for the scene.
This ensures a clean state when entering Ran's test scene.
*/

static void SetupECSGraphicsForMode()
{
    if (!globalECS) return;

    globalECS->Reset();

    using namespace CrashoutsEngine;
    using namespace Component;

    // Default-off toggle: shapes will not be created unless set to true
    const bool renderShapesByDefault = false;

    switch (displayMode) {
    case 0: {
        if (renderShapesByDefault) {
            Entity e1 = globalECS->CreateEntity();
            globalECS->AddComponent<NameComponent>(e1, "Square");
            TransformComponent t1;
            t1.position = CEVec3(-20.f, 0.f, 0.f);
            t1.size = CEVec2(1.f, 1.f);
            t1.rotation = 0.f;
            globalECS->AddComponent<TransformComponent>(e1, t1);
            ShapeComponent s1;
            s1.type = ShapeType::Square;
            s1.size = 12.f;
            s1.color = CEVec3(255.f, 0.f, 0.f);
            globalECS->AddComponent<ShapeComponent>(e1, s1);

            Entity e2 = globalECS->CreateEntity();
            globalECS->AddComponent<NameComponent>(e2, "Rect");
            TransformComponent t2;
            t2.position = CEVec3(20.f, 0.f, 0.f);
            t2.size = CEVec2(1.f, 1.f);
            t2.rotation = 0.f;
            globalECS->AddComponent<TransformComponent>(e2, t2);
            ShapeComponent s2;
            s2.type = ShapeType::Rectangle;
            s2.width = 30.f;
            s2.height = 15.f;
            s2.color = CEVec3(0.f, 128.f, 255.f);
            globalECS->AddComponent<ShapeComponent>(e2, s2);
        }
        break;
    }
    case 1: {
        float aspect = static_cast<float>(GLApp::width) / GLApp::height;
        float worldWidth, worldHeight;
        if (aspect >= 1.0f) {
            worldHeight = 100.0f;
            worldWidth = worldHeight * aspect;
        }
        else {
            worldWidth = 100.0f;
            worldHeight = worldWidth / aspect;
        }

        Entity bg = globalECS->CreateEntity();
        globalECS->AddComponent<NameComponent>(bg, "Background");
        TransformComponent t;
        t.position = CEVec3(0.0f, 0.0f, -0.9f);
        t.size = CEVec2(1.f, 1.f);
        t.rotation = 0.f;
        globalECS->AddComponent<TransformComponent>(bg, t);
        ShapeComponent sc;
        sc.type = ShapeType::Rectangle;
        sc.width = worldWidth;
        sc.height = worldHeight;
        sc.enableTexture = true;
        sc.textureName = "duck";
        sc.color = CEVec3(255.f, 255.f, 255.f);
        globalECS->AddComponent<ShapeComponent>(bg, sc);
        break;
    }
    default:
        break;
    }
}

void LoadRan()
{
    globalECS = CrashoutsEngine::ECS::getInstance();
    globalECS->Reset();

    CrashoutsEngine::Assets::GenerateAssetManifest();
    CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();

    SetupECSGraphicsForMode();
}

/*!
@brief Initialize Ran's scene components and systems.
@details
Performs scene-specific initialization including:
- Logging scene initialization
- Setting up instanced rendering system
- Initializing sprite animation system
Called after LoadRan() to prepare the scene for rendering.
*/
void InitRan()
{
    LOGI(Core, "Ran Initialized");
    InitializeInstancing();
    InitializeSpriteAnimation();
}

/*!
@brief Initialize the sprite animation system with duck character.
@details
Sets up the duck sprite with a 4x6 sprite sheet containing 24 frames:
- Row 0 (frames 0-5): idle_normal animation (2 frames used)
- Row 1 (frames 6-11): walk_normal animation (6 frames used)
- Row 2 (frames 12-17): idle_bounce animation (4 frames used)
- Row 3 (frames 18-23): walk_bounce animation (2 frames used)

Configures initial sprite properties including position, scale, rotation,
and sets the default animation state to "walk_normal".
*/
void InitializeSpriteAnimation()
{
    // Init duck sprite - 4 rows, 6 columns (24 total frames)
    Component::Sprite::m_sprite.init("ducky_2_spritesheet", 4, 6, 0.3f);

    // Setup duck animations for 4x6 sprite sheet layout:
    // Row 0 (frames 0-5): idle_normal animation (use first 2 frames)
    // Row 1 (frames 6-11): walk_normal animation (use all 6 frames)  
    // Row 2 (frames 12-17): idle_bounce animation (use first 4 frames)
    // Row 3 (frames 18-23): walk_bounce animation (use first 2 frames)
    
    Component::Sprite::m_sprite.addAnimationState("idle_normal", Component::AnimationState(0, 2, 0.3f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_normal", Component::AnimationState(6, 6, 0.08f, true));  // Faster walking animation
    Component::Sprite::m_sprite.addAnimationState("idle_bounce", Component::AnimationState(12, 4, 0.2f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_bounce", Component::AnimationState(18, 2, 0.12f, true));

    // Set position and scale for the duck
    Component::Sprite::m_sprite.setPosition(0.0f, 0.0f);
    Component::Sprite::m_sprite.setScale(TestScale * 8.0f); // Increase scale for better visibility
    Component::Sprite::m_sprite.setRotation(TestRotation);

    Component::Sprite::m_sprite.setAnimationState("walk_normal");
}

/*!
@brief Update Ran's scene logic per frame.
@details
Calls the sprite animation update function to handle per-frame
animation logic, movement, and state transitions.
*/
void UpdateRan()
{
    UpdateSpriteAnimation();
}

/*!
@brief Update sprite animation logic and movement.
@details
Handles duck character movement and animation state management:
- Calculates movement based on input flags and delta time
- Determines appropriate animation state (walk vs idle)
- Updates sprite position, facing direction, scale, and rotation
- Manages animation state transitions
- Updates sprite animation frame progression

Movement speed is set to 15.0 world units per second. Animation states
are switched between "walk_normal" when moving and "idle_bounce" when stationary.
*/
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
    // Ensure ECS is initialized before rendering
    if (!globalECS) {
        globalECS = CrashoutsEngine::ECS::getInstance();
        SetupECSGraphicsForMode();
    }

    // Clear the screen
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    switch (displayMode) {
    case 0: // Clear background with shapes (ECS-driven)
        Scene::RenderScene(*globalECS);
        break;
    case 1: // JPG background (ECS-driven textured rectangle)
        Scene::RenderScene(*globalECS);
        break;
    case 2: // Instancing on clear background
        RenderInstancing();
        break;
    case 3: // Sprite animation
        RenderSpriteAnimation();
        break;
    }
}

void RenderShapes()
{
    // Render basic shapes - this can be expanded based on needs
    // For now, just clear background
}

void RenderBackground()
{
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

    // Display JPG background
    Component::Shapes::CreateTexturedRectangle(
        "duck",
        CEVec3(0.0f, 0.0f, -0.9f),  // Position at center, behind other objects
        worldWidth,                  // Full world width
        worldHeight,                 // Full world height
        0.0f,                       // No rotation
        1.0f,                       // Normal scale
        CEVec3(1.0f, 1.0f, 1.0f)    // No tint
    );
}

void RenderInstancing()
{
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

void RenderSpriteAnimation()
{
    // Render the sprite animation
    Component::Sprite::m_sprite.render();
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
    // Reconfigure ECS content to match the new display mode
    SetupECSGraphicsForMode();
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


