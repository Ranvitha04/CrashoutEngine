#include "ranscene.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "sceneutils.h"
#include "assetmanager.h"
#include "sceneasset.h"
#include "glapp.h"
#include "rendering.h"
#include "shapes.h"
#include "spriteAnimation.h"
#include "frameratecontroller.h"
#include <random>
#include <vector>

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "ran";
// file-level state and helpers
//static CrashoutsEngine::ECS* globalECS;

static int gDisplayMode = 1;                // 0:bg, 1:duck, 2:instanced, 3:imgui-only
static bool gPrevGPressed = false;
static bool gSpriteInitializedInScene = false;  

static float gDuckX = 0.0f;
static float gDuckY = 0.0f;
static const float DUCK_SPEED = 20.0f; // world units per second


static float gDuckScale = 1.0f;
static float gDuckRotationDeg = 0.0f;
static const float DUCK_SCALE_SPEED = 1.5f;   // scale units per second
static const float DUCK_ROT_SPEED   = 90.0f;  // degrees per second
static float gDuckT = 0.0f;

static bool gSquaresInitialized = false;
static std::vector<CEVec3> gInstPositions;
static std::vector<CEVec3> gInstColors;
static std::vector<float> gInstRotations;
static std::vector<float> gInstScales;

static void ComputeWorldDimensions(float& worldWidth, float& worldHeight) {
    const float BASE_SIZE = 100.0f;
    float aspect = static_cast<float>(GLApp::width) / static_cast<float>(GLApp::height);
    if (aspect >= 1.0f) {
        worldHeight = BASE_SIZE;
        worldWidth = worldHeight * aspect;
    } else {
        worldWidth = BASE_SIZE;
        worldHeight = worldWidth / aspect;
    }
}

static void InitializeInstancedSquaresData(int count) {
    gInstPositions.clear();
    gInstColors.clear();
    gInstRotations.clear();
    gInstScales.clear();
    gInstPositions.reserve(count);
    gInstColors.reserve(count);
    gInstRotations.reserve(count);
    gInstScales.reserve(count);

    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(-worldW / 2.0f, worldW / 2.0f);
    std::uniform_real_distribution<float> distY(-worldH / 2.0f, worldH / 2.0f);
    std::uniform_real_distribution<float> distC(0.2f, 1.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
    std::uniform_real_distribution<float> distScale(0.5f, 2.0f);

    for (int i = 0; i < count; ++i) {
        gInstPositions.emplace_back(distX(gen), distY(gen), 0.0f);
        gInstColors.emplace_back(distC(gen), distC(gen), distC(gen));
        gInstRotations.emplace_back(distRot(gen));
        gInstScales.emplace_back(distScale(gen));
    }
    gSquaresInitialized = true;
}

// function RenderBackground()
void RenderBackground() {
    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);
    Component::Shapes::CreateTexturedRectangle(
        "assets/images/bg 1 base.png",  // Use an existing texture file
        CEVec3(0.0f, 0.0f, 0.0f),
        worldW, worldH,
        0.0f, 1.0f,
        CEVec3(1.0f, 1.0f, 1.0f)
    );
}

static void RenderInstancedSquares() {
    const int count = static_cast<int>(gInstPositions.size());
    if (count == 0) return;
    Component::Shapes::CreateInstancedSquares(
        gInstPositions.data(),
        gInstColors.data(),
        gInstRotations.data(),
        gInstScales.data(),
        count,
        1.0f
    );
}

static void EnsureDuckInitialized() {
    if (gSpriteInitializedInScene) return;
    Component::Sprite::m_sprite.init("assets/sprite_sheet/ducky_2_spritesheet.png", 4, 6, 0.3f);
    Component::Sprite::m_sprite.addAnimationState("idle_normal",  Component::AnimationState(0,  2, 0.22f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_normal",  Component::AnimationState(6,  6, 0.09f, true));
    Component::Sprite::m_sprite.addAnimationState("idle_bounce",  Component::AnimationState(12, 4, 0.20f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_bounce",  Component::AnimationState(16, 2, 0.10f, true));
    Component::Sprite::m_sprite.setAnimationState("walk_normal");
    Component::Sprite::m_sprite.setPosition(gDuckX, gDuckY);
    Component::Sprite::m_sprite.setScale(gDuckScale);
    Component::Sprite::m_sprite.setRotation(gDuckRotationDeg);
    gSpriteInitializedInScene = true;
}

static void EnsureDuckCleaned() {
    if (!gSpriteInitializedInScene) return;
    Component::Sprite::m_sprite.cleanup();
    gSpriteInitializedInScene = false;
}

void LoadRan()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	CrashoutsEngine::Assets::GenerateAssetManifest();

	CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
}

void InitRan()
{
    LOGI(Core, "Ran Initialized");
    gDisplayMode = 1;
    gPrevGPressed = false;
    gSquaresInitialized = false;
    gSpriteInitializedInScene = false;  // FIX: allow EnsureDuckInitialized() to run
}

// function UpdateRan()
void UpdateRan()
{
    int gState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_G);
    bool gPressed = (gState == GLFW_PRESS);
    if (gPressed && !gPrevGPressed) {
        int prevMode = gDisplayMode;
        int nextMode = (gDisplayMode + 1) % 4;
        gDisplayMode = nextMode;

        if (gDisplayMode == 1) {
            EnsureDuckInitialized();
        } else {
            EnsureDuckCleaned();
        }

        // Prepare data when entering squares mode
        if (gDisplayMode == 2) {
            gSquaresInitialized = false;
        }

        // Hard reset instanced data when leaving squares mode
        if (prevMode == 2 && nextMode != 2) {
            gInstPositions.clear();
            gInstColors.clear();
            gInstRotations.clear();
            gInstScales.clear();
            gSquaresInitialized = false;
        }
    }
   
    
    // Movement, scale, rotation when in duck mode
    if (gDisplayMode == 1 && gSpriteInitializedInScene) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());

        // Orientation-based movement using current rotation
        int aState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_A);
        int dState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_D);
        bool forward  = (dState == GLFW_PRESS) || (dState == GLFW_REPEAT);
        bool backward = (aState == GLFW_PRESS) || (aState == GLFW_REPEAT);

        const float rad = gDuckRotationDeg * 3.14159265f / 180.0f;
        const float dirX = std::cos(rad);
        const float dirY = std::sin(rad);

        float move = 0.0f;
        if (forward && !backward) {
            move = DUCK_SPEED * dt;
        } else if (backward && !forward) {
            move = -DUCK_SPEED * dt;
        }

        if (move != 0.0f) {
            gDuckX += dirX * move;
            gDuckY += dirY * move;
            Component::Sprite::m_sprite.setPosition(gDuckX, gDuckY);
            Component::Sprite::m_sprite.setFlipHorizontal((move * dirX) < 0.0f);

            // Walking: use walk animation
            Component::Sprite::m_sprite.setAnimationState("walk_normal");
        } else {
            // Not walking: switch to idle-bounce
            Component::Sprite::m_sprite.setAnimationState("idle_bounce");
        }

        // Scale controls (S/Q)
        int sState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_S);
        int qState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_Q);
        bool scaleUp   = (sState == GLFW_PRESS) || (sState == GLFW_REPEAT);
        bool scaleDown = (qState == GLFW_PRESS) || (qState == GLFW_REPEAT);
        if (scaleUp || scaleDown) {
            float delta = DUCK_SCALE_SPEED * dt * (scaleUp ? 1.0f : -1.0f);
            gDuckScale = std::max(0.1f, gDuckScale + delta);
            Component::Sprite::m_sprite.setScale(gDuckScale);
        }

        // Rotation controls (R/T)
        int rState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_R);
        int tState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_T);
        bool rotLeft  = (rState == GLFW_PRESS) || (rState == GLFW_REPEAT);
        bool rotRight = (tState == GLFW_PRESS) || (tState == GLFW_REPEAT);
        if (rotLeft || rotRight) {
            float deltaDeg = DUCK_ROT_SPEED * dt * (rotRight ? 1.0f : -1.0f);
            gDuckRotationDeg += deltaDeg;
            if (gDuckRotationDeg > 360.0f) gDuckRotationDeg -= 360.0f;
            if (gDuckRotationDeg < -360.0f) gDuckRotationDeg += 360.0f;
            Component::Sprite::m_sprite.setRotation(gDuckRotationDeg);
        }
    }

    gPrevGPressed = gPressed;

}

void RenderRan()
{
    //framebuffer is cleared every frame
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    switch (gDisplayMode) {
        case 0: // Background image
            RenderBackground();
            break;
        case 1: // Duck sprite animation
            EnsureDuckInitialized();
            Component::Sprite::m_sprite.update(static_cast<float>(FrameRateController::GetInstance().DT()));
            Component::Sprite::m_sprite.render();
            break;
        case 2: // Instanced squares
            if (!gSquaresInitialized) {
                InitializeInstancedSquaresData(2500);
            }
            RenderInstancedSquares();
            break;
        case 3:
           
            Scene::RenderScene(*globalECS);
            break;
    }

    
    
}

void FreeRan()
{
	CrashoutsEngine::Serialization::SaveScenes(sceneName);
	 EnsureDuckCleaned();
    gSquaresInitialized = false;
    gInstPositions.clear();
    gInstColors.clear();
    gInstRotations.clear();
    gInstScales.clear();
}

void UnloadRan()
{
}

void HandleSpriteKeyInput(int key, int action)
{
    using Component::Sprite;

    
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_LEFT:
            case GLFW_KEY_A:
                Sprite::m_sprite.setFlipHorizontal(true);
                break;
            case GLFW_KEY_RIGHT:
            case GLFW_KEY_D:
                Sprite::m_sprite.setFlipHorizontal(false);
                break;
            default:
                break;
        }
    }
}