/*!
@file        ranvitha.cpp
@author      ranvitha@digipen.edu
@date        18/09/2025
@brief
This source file implements the **Ran game state** logic for the CrashoutsEngine-based
project. It contains functions responsible for initializing, updating, and rendering
game elements within the “Ran” state, including ECS-driven random shape generation,
background rendering, and state transitions.

The file primarily handles:
- **Mode 5 ECS shape initialization**: Randomized creation of rectangles, triangles,
  and circles, including color, position, and rotation attributes.
- **Resource management**: Loading textures, cleaning up sprites, and resetting ECS data.
- **Rendering**: Drawing the background and instanced geometry based on the current mode.
- **Input handling**: Responding to key presses (e.g., switching display modes with the G key).

This module interacts closely with the `InstancingSystem` and the global ECS manager to
maintain a flexible and data-driven rendering flow for the Ran state. It serves as the
entry point for Ran’s lifecycle functions — including initialization (`InitRan`),
rendering (`RenderBackground`), and updating (`UpdateRan`).
*/


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
#include "instanceSystem.h"
#include <random>
#include <vector>
#include <string>
#include <cmath>


static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "ran";

// Display/control state
static int gDisplayMode = 0;                
static bool gPrevGPressed = false;
static bool gPrevMPressed = false;          // toggle instancing type (texture/plain)
static bool gSpriteInitializedInScene = false;

// Duck controls/state
static float gDuckX = 0.0f;
static float gDuckY = 0.0f;
static const float DUCK_SPEED = 20.0f; // world units per second
static float gDuckScale = 1.0f;
static float gDuckRotationDeg = 0.0f;
static const float DUCK_SCALE_SPEED = 1.5f;   // scale units per second
static const float DUCK_ROT_SPEED = 90.0f;  // degrees per second
static float gDuckT = 0.0f;

// Instanced state
//static bool gSquaresInitialized = false;
static std::vector<CEVec3> gInstPositions;
static std::vector<CEVec3> gInstColors;
static std::vector<float>  gInstRotations;
static std::vector<float>  gInstScales;

// Instanced rectangles state
static bool gRectsInitialized = false;
static std::vector<CEVec3> gRndRectPos;
static std::vector<CEVec3> gRndRectColor;
static std::vector<float>  gRndRectRot;
static std::vector<float>  gRndRectW;
static std::vector<float>  gRndRectH;
static int gInstancingType = 0; // 0: plain colored, 1: textured

const std::string textureDucky = "ducky.png";

static bool gEcsRectsInitialized = false;
static bool gEcsTexturesEnabled = false;

// ===== Mode 4 (non-ECS) random shapes =====
static bool gMode4Initialized = false;
static bool gMode4TexturesEnabled = false;


// ===== Mode 5 (ECS-driven) random shapes =====
static bool gMode5Initialized = false;
static bool gMode5TexturesEnabled = false;

// Derived instancing arrays from ECS (rectangles + triangles)
static std::vector<CEVec3> gM5RectPos;
static std::vector<CEVec3> gM5RectColor;    // 0..1 colors
static std::vector<float>  gM5RectRot;
static std::vector<float>  gM5RectScale;    // keep 1.0f and bake into width/height
static std::vector<float>  gM5RectW;
static std::vector<float>  gM5RectH;

static std::vector<CEVec3> gM5TriPos;
static std::vector<CEVec3> gM5TriColor;     // 0..1 colors
static std::vector<float>  gM5TriRot;
static std::vector<float>  gM5TriScale;     // use 1.0f
static std::vector<float>  gM5TriSide;      // derived from t1..t3

static std::vector<CEVec3> gM5CircPos;
static std::vector<CEVec3> gM5CircColor;
static std::vector<float>  gM5CircRot;
static std::vector<float>  gM5CircScale;
static std::vector<float>  gM5CircRadius;
static int gM5CircSegments = 24;

// Mode 6 (ECS ducks)
static bool gMode6Initialized = false;
static bool gPrevIPressed = false;

static void ComputeWorldDimensions(float& worldWidth, float& worldHeight) {
    const float BASE_SIZE = 100.0f;
    float aspect = static_cast<float>(GLApp::width) / static_cast<float>(GLApp::height);
    if (aspect >= 1.0f) {
        worldHeight = BASE_SIZE;
        worldWidth = worldHeight * aspect;
    }
    else {
        worldWidth = BASE_SIZE;
        worldHeight = worldWidth / aspect;
    }
}

/*!
@brief Initialize the single duck sprite for display mode 1.
@details
- Loads the duck spritesheet and registers animation states.
- Applies initial position, scale, and rotation.
- Guards against double-initialization via `gSpriteInitializedInScene`.
*/
static void EnsureDuckInitialized() {
    Component::Sprite::m_sprite.init("assets/images/ducky_2_spritesheet.png", 4, 6, 0.3f);
    Component::Sprite::m_sprite.addAnimationState("idle_normal", Component::AnimationState(0, 2, 0.22f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_normal", Component::AnimationState(6, 6, 0.09f, true));
    Component::Sprite::m_sprite.addAnimationState("idle_bounce", Component::AnimationState(12, 4, 0.20f, true));
    Component::Sprite::m_sprite.addAnimationState("walk_bounce", Component::AnimationState(16, 2, 0.10f, true));
    Component::Sprite::m_sprite.setAnimationState("walk_normal");
    Component::Sprite::m_sprite.setPosition(gDuckX, gDuckY);
    Component::Sprite::m_sprite.setScale(gDuckScale);
    Component::Sprite::m_sprite.setRotation(gDuckRotationDeg);
    gSpriteInitializedInScene = true;
}


// Initialize Mode 5 random shapes via ECS (rectangles + triangles)
// Also build the derived instancing arrays for rectangles/triangles from ECS.
// Circles are generated separately (not ECS).
static void InitializeMode5EcsRandomShapes(int perTypeCount) {
    gMode5Initialized = false;

    if (!globalECS) return;

    // Seed ECS with rectangles and triangles
    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
    std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);
    std::uniform_real_distribution<float> rectW(1.0f, 6.0f);
    std::uniform_real_distribution<float> rectH(1.0f, 6.0f);
    std::uniform_real_distribution<float> triSide(1.0f, 6.0f);
    std::uniform_int_distribution<int> color255(70, 255);

    // Rectangles
    for (int i = 0; i < perTypeCount; ++i) {
        auto e = globalECS->CreateEntity();
        CEVec3 pos{ posX(rng), posY(rng), 0.0f };
        float rotation = rotDeg(rng);

        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "M5Rect_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ rotation, CEVec2{1.0f, 1.0f}, pos });

        Component::ShapeComponent sc{};
        sc.type = Component::ShapeType::Rectangle;
        sc.color = CEVec3{ static_cast<float>(color255(rng)), static_cast<float>(color255(rng)), static_cast<float>(color255(rng)) };
        sc.enableTexture = gMode5TexturesEnabled;
        sc.textureName = textureDucky;
        sc.uvSize = CEVec2{ 1.0f, 1.0f };
        globalECS->AddComponent<Component::ShapeComponent>(e, sc);
    }

    // Triangles (equilateral base vertices, sized by 'size' param)
    // t1,t2,t3 are base vertices centered at origin; rotation/pos from TransformComponent
    for (int i = 0; i < perTypeCount; ++i) {
        auto e = globalECS->CreateEntity();
        CEVec3 pos{ posX(rng), posY(rng), 0.0f };
        float rotation = rotDeg(rng);
        // float s = triSide(rng);
        // Equilateral triangle points scaled by side length
        // Height = sqrt(3)/2 * s; center at origin
        // float h = 0.866025403784f * s;
        Component::ShapeComponent sc{};
        sc.type = Component::ShapeType::Triangle;
        //sc.t1 = CEVec3(0.0f, 2.0f / 3.0f * h, 0.0f);            // top
        //sc.t2 = CEVec3(-0.5f * s, -1.0f / 3.0f * h, 0.0f);      // bottom-left
        //sc.t3 = CEVec3(0.5f * s, -1.0f / 3.0f * h, 0.0f);      // bottom-right
        sc.color = CEVec3{ static_cast<float>(color255(rng)), static_cast<float>(color255(rng)), static_cast<float>(color255(rng)) };
        sc.enableTexture = gMode5TexturesEnabled;
        sc.textureName = textureDucky;
        sc.uvSize = CEVec2{ 1.0f, 1.0f };

        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "M5Tri_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ rotation, CEVec2{1.0f, 1.0f}, pos });
        globalECS->AddComponent<Component::ShapeComponent>(e, sc);
    }

    // Build instancing arrays from ECS (rects + tris)
    gM5RectPos.clear(); gM5RectColor.clear(); gM5RectRot.clear(); gM5RectScale.clear(); gM5RectW.clear(); gM5RectH.clear();
    gM5TriPos.clear();  gM5TriColor.clear();  gM5TriRot.clear();  gM5TriScale.clear();  gM5TriSide.clear();

    const auto& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
    for (const auto& id : shapeArray.indexToEntity) {
        auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
        if (!sc) continue;

        CEVec3 pos{ 0,0,0 }; float rot = 0.0f; CEVec2 tfScale{ 1.f, 1.f };
        if (globalECS->HasComponent<Component::TransformComponent>(id)) {
            auto* tf = globalECS->GetComponent<Component::TransformComponent>(id);
            pos = tf->position;
            rot = tf->rotation;
            tfScale = tf->size;
        }

        // Normalize color
        CEVec3 c01{ sc->color.x * (1.f / 255.f), sc->color.y * (1.f / 255.f), sc->color.z * (1.f / 255.f) };

        switch (sc->type) {
        case Component::ShapeType::Rectangle: {
            gM5RectPos.push_back(pos);
            gM5RectColor.push_back(c01);
            gM5RectRot.push_back(rot);
            gM5RectScale.push_back(1.0f); // bake scale in width/height
        } break;
        case Component::ShapeType::Triangle: {
            gM5TriPos.push_back(pos);
            gM5TriColor.push_back(c01);
            gM5TriRot.push_back(rot);
            gM5TriScale.push_back(1.0f);

            // Derive side length from t1..t3 (use edge t2-t3)
            //auto dx = sc->t3.x - sc->t2.x;
            //auto dy = sc->t3.y - sc->t2.y;
            //float side = std::sqrt(dx * dx + dy * dy);
            //gM5TriSide.push_back(side);
        } break;
        default: break;
        }
    }

    // Circles for Mode 5 (not ECS)
    gM5CircPos.clear(); gM5CircColor.clear(); gM5CircRot.clear(); gM5CircScale.clear(); gM5CircRadius.clear();
    gM5CircPos.reserve(perTypeCount); gM5CircColor.reserve(perTypeCount);
    gM5CircRot.reserve(perTypeCount); gM5CircScale.reserve(perTypeCount); gM5CircRadius.reserve(perTypeCount);

    std::uniform_real_distribution<float> col01(0.2f, 1.0f);
    std::uniform_real_distribution<float> circRadius(1.0f, 4.0f);

    for (int i = 0; i < perTypeCount; ++i) {
        gM5CircPos.emplace_back(posX(rng), posY(rng), 0.0f);
        gM5CircColor.emplace_back(col01(rng), col01(rng), col01(rng));
        gM5CircRot.emplace_back(rotDeg(rng));
        gM5CircScale.emplace_back(1.0f);
        gM5CircRadius.emplace_back(circRadius(rng));
    }

    gMode5Initialized = true;
}

/*!
@brief Cleanup the single duck sprite state and GPU resources.
@details Ensures sprite resources are released when leaving mode 1.
*/
static void EnsureDuckCleaned() {
    if (!gSpriteInitializedInScene) return;
    Component::Sprite::m_sprite.cleanup();
    gSpriteInitializedInScene = false;
}

/*!
@brief Load Ran scene resources and reset ECS state.
@details
- Gets the global ECS instance and resets it.
- Generates and loads the asset manifest so textures/shaders are available.
*/
void LoadRan()
{
    globalECS = CrashoutsEngine::ECS::getInstance();
    globalECS->Reset();

    CrashoutsEngine::Assets::GenerateAssetManifest();
    CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
}

void InitRan()
{
    LOGI(Core, "Ran Initialized");
    gDisplayMode = 0;
    gPrevGPressed = false;
    gPrevMPressed = false;

    gRectsInitialized = false;
    gSpriteInitializedInScene = false;  // allow EnsureDuckInitialized() to run
}

void InitializeInstancing()
{
}

void RenderShapes()
{
}

/*!
@brief Render and scale the background image to fit the world.
@details Uses aspect-aware world dimensions to size the background quad.
*/
void RenderBackground()
{
    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);
    CEVec2 size{ 1.f, 1.f };
    Component::Shapes::CreateTexturedRectangle(
        size,
        "assets/images/bg 1 base.png",  // Use an existing texture file
        CEVec3(0.0f, 0.0f, 0.0f),
        0.0f,
        CEVec3(1.0f, 1.0f, 1.0f)
    );
}

void RenderInstancing()
{
}

void RenderSpriteAnimation()
{
}

/*!
@brief Update loop for Ran scene handling input and per-mode logic.
@details
- Press `G` to cycle display modes (0..6) and reinitialize mode-specific data.
- Press `M` to toggle textures in modes 2/3/5.
- Mode 1: update duck movement, scale, and rotation.
- Mode 6: update ECS duck motion and handle `I` key to cycle manual animation.
*/
void UpdateRan()
{
    // Toggle display mode with G 
    int gState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_G);
    bool gPressed = (gState == GLFW_PRESS);
    if (gPressed && !gPrevGPressed) {
        int prevMode = gDisplayMode;
        int nextMode = (gDisplayMode + 1) % 7; // include mode 6
        gDisplayMode = nextMode;

        if (gDisplayMode != 3) {
            const CrashoutsEngine::ECS::ComponentArray<CrashoutsEngine::NameComponent>& nameArray = 
                globalECS->GetComponentArray<CrashoutsEngine::NameComponent>();
            if (nameArray.indexToEntity.size() > 0) {
                for (auto id : nameArray.indexToEntity) {
                    globalECS->DestroyEntity(id);
                }
            }
        }

        if (gDisplayMode == 1) {
            EnsureDuckInitialized();
        }
        else {
            EnsureDuckCleaned();
        }

        // Prepare data when entering instanced mode
        if (gDisplayMode == 2) {

            gRectsInitialized = false;
        }

        // Initialize randomized ECS rectangles when entering ECS mode (3)
        if (gDisplayMode == 3 && !gEcsRectsInitialized) {
            gEcsTexturesEnabled = false;

            // Remove triangles (injected by other modes like 5) to keep mode 3 rectangle-only
            std::vector<CrashoutsEngine::Entity> toRemove;
            auto& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
            for (const auto& id : shapeArray.indexToEntity) {
                auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
                if (sc && sc->type == Component::ShapeType::Triangle) {
                    toRemove.push_back(id);
                }
            }
            for (auto id : toRemove) {
                globalECS->DestroyEntity(id);
            }

            // Seed rectangles via InstancingSystem’s existing ECS helper
            InstancingSystem::InitializeEcsRectanglesDataRandom(2500);
            gEcsRectsInitialized = true;
        }

        // Initialize Mode 4 random shapes (non-ECS)
        if (gDisplayMode == 4 && !gMode4Initialized) {
            gMode4TexturesEnabled = false;
            InstancingSystem::InitializeMode4RandomShapes(500);   // 500 rects, 500 tris, 500 circles
        }

        // Initialize Mode 5 ECS random shapes
        if (gDisplayMode == 5 && !gMode5Initialized) {
            gMode5TexturesEnabled = false;
            //InitializeMode5EcsRandomShapes(500); // 500 rects, 500 tris , 500 circles
            gMode5TexturesEnabled = false;
            InstancingSystem::InitializeMode5EcsRandomShapes(
                CrashoutsEngine::ECS::getInstance(),
                500,
                gMode5TexturesEnabled,
                textureDucky,
                // Rectangles out
                gM5RectPos, gM5RectColor,
                gM5RectRot, gM5RectScale,
                gM5RectW, gM5RectH,
                // Triangles out
                gM5TriPos, gM5TriColor,
                gM5TriRot, gM5TriScale, gM5TriSide,
                // Circles out (not ECS)
                gM5CircPos, gM5CircColor,
                gM5CircRot, gM5CircScale, gM5CircRadius,
                gM5CircSegments
            );
            gMode5Initialized = true;
        }

        //to clear every change
        if (prevMode == 2 && nextMode != 2) {

            gRectsInitialized = false;
        }

        if (prevMode == 3 && nextMode != 3) {

            gEcsRectsInitialized = false;
        }

        if (prevMode == 4 && nextMode != 4){
        
            gMode4Initialized = false;
        
        }

    }


    int mState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_M);
    bool mPressed = (mState == GLFW_PRESS);
    if (mPressed && !gPrevMPressed) {
        if (gDisplayMode == 2) {
            gInstancingType = (gInstancingType == 0) ? 1 : 0;
        }
        else if (gDisplayMode == 3) {
            // Toggle textures for ECS rectangles
            gEcsTexturesEnabled = !gEcsTexturesEnabled;

            auto& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
            for (const auto& id : shapeArray.indexToEntity) {
                auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
                if (sc && sc->type == Component::ShapeType::Rectangle) {
                    sc->enableTexture = gEcsTexturesEnabled;
                    if (gEcsTexturesEnabled) {
                        sc->textureName = textureDucky;
                        sc->uvSize = CEVec2{ 1.0f, 1.0f };
                    }
                }
            }
        }
        else if (gDisplayMode == 4) {

            gMode4TexturesEnabled = !gMode4TexturesEnabled;

        }
        else if (gDisplayMode == 5) {

            gMode5TexturesEnabled = !gMode5TexturesEnabled;


            auto& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
            for (const auto& id : shapeArray.indexToEntity) {
                auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
                if (sc && (sc->type == Component::ShapeType::Rectangle || sc->type == Component::ShapeType::Triangle)) {
                    sc->enableTexture = gMode5TexturesEnabled;
                    sc->textureName = textureDucky;
                    sc->uvSize = CEVec2{ 1.0f, 1.0f };
                }
            }
        }
    }

    // Movement, scale, rotation when in duck mode
    if (gDisplayMode == 1 && gSpriteInitializedInScene) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());

        // Orientation-based movement using current rotation
        int aState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_A);
        int dState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_D);
        bool forward = (dState == GLFW_PRESS) || (dState == GLFW_REPEAT);
        bool backward = (aState == GLFW_PRESS) || (aState == GLFW_REPEAT);

        const float rad = gDuckRotationDeg * 3.14159265f / 180.0f;
        const float dirX = std::cos(rad);
        const float dirY = std::sin(rad);

        float move = 0.0f;
        if (forward && !backward) {
            move = DUCK_SPEED * dt;
        }
        else if (backward && !forward) {
            move = -DUCK_SPEED * dt;
        }

        if (move != 0.0f) {
            gDuckX += dirX * move;
            gDuckY += dirY * move;
            Component::Sprite::m_sprite.setPosition(gDuckX, gDuckY);
            Component::Sprite::m_sprite.setFlipHorizontal((move * dirX) < 0.0f);
            Component::Sprite::m_sprite.setAnimationState("walk_normal");
        }
        else {
            Component::Sprite::m_sprite.setAnimationState("idle_bounce");
        }

        // Scale controls (S/Q)
        int sState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_S);
        int qState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_Q);
        bool scaleUp = (sState == GLFW_PRESS) || (sState == GLFW_REPEAT);
        bool scaleDown = (qState == GLFW_PRESS) || (qState == GLFW_REPEAT);
        if (scaleUp || scaleDown) {
            float delta = DUCK_SCALE_SPEED * dt * (scaleUp ? 1.0f : -1.0f);
            gDuckScale = std::max(0.1f, gDuckScale + delta);
            Component::Sprite::m_sprite.setScale(gDuckScale);
        }

        // Rotation controls (R/T)
        int rState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_R);
        int tState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_T);
        bool rotLeft = (rState == GLFW_PRESS) || (rState == GLFW_REPEAT);
        bool rotRight = (tState == GLFW_PRESS) || (tState == GLFW_REPEAT);
        if (rotLeft || rotRight) {
            float deltaDeg = DUCK_ROT_SPEED * dt * (rotRight ? 1.0f : -1.0f);
            gDuckRotationDeg += deltaDeg;
            if (gDuckRotationDeg > 360.0f) gDuckRotationDeg -= 360.0f;
            if (gDuckRotationDeg < -360.0f) gDuckRotationDeg += 360.0f;
            Component::Sprite::m_sprite.setRotation(gDuckRotationDeg);
        }
    }

    // ECS ducks update
    if (gDisplayMode == 6 && gMode6Initialized) {
        InstancingSystem::UpdateEcsDuckSprites();
    }

    // Handle "I" toggle for duck animation state in mode 6
    int iState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_I);
    bool iPressed = (iState == GLFW_PRESS);
    if (iPressed && !gPrevIPressed && gDisplayMode == 6 && gMode6Initialized) {
        InstancingSystem::CycleEcsDuckAnimationState();
    }
    gPrevIPressed = iPressed;

    gPrevGPressed = gPressed;
    gPrevMPressed = mPressed;
}

void RenderRan()
{
    // framebuffer is cleared every frame
#if USE_IMGUI_EDITOR
#else
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    switch (gDisplayMode) {
    case 0: // Background image
        RenderBackground();
        break;
    case 1: // Duck sprite animation
        //EnsureDuckInitialized();
        Component::Sprite::m_sprite.update(static_cast<float>(FrameRateController::GetInstance().DT()));
        Component::Sprite::m_sprite.render();
        break;
    case 2: //Mode M toggles texture
        //if (!gSquaresInitialized) {
        //    InitializeInstancedSquaresData(25000);
        //}
        std::cout << "non ecs" << std::endl;
        if (!gRectsInitialized) {
            InstancingSystem::InitializeInstancedRectanglesData(2500);
            gRectsInitialized = true;
        }

        // Render instanced rectangles from InstancingSystem internal arrays
        {
            InstancingSystem::InstancedParams params{};
            if (gInstancingType == 0) {
                params.textured = false;
            } else {
                params.textured = true;
                params.textureName = textureDucky;
                params.tint = CEVec3(1.f, 1.f, 1.f);
                params.uvSize = CEVec2(1.f, 1.f);
            }
            InstancingSystem::RenderInstancedRectangles(params);
        }
        break;
    case 3:
        std::cout << "ec" << std::endl;
        //Scene::RenderScene(*globalECS);
        break;
    case 4: { // Non-ECS instanced random triangles, rectangles, circles with texture toggle
        std::cout << "rand shap" << std::endl;

        // No re-init here; seed in UpdateRan or when entering mode
        InstancingSystem::InstancedParams params{};
        params.textured = gMode4TexturesEnabled;
        params.textureName = textureDucky;
        params.tint = CEVec3(1.f, 1.f, 1.f);
        params.uvSize = CEVec2(1.f, 1.f);

        // Draw order: circles -> triangles -> rectangles
        InstancingSystem::RenderMode4Circles(params);
        InstancingSystem::RenderMode4Triangles(params);
        InstancingSystem::RenderMode4Rectangles(params);
    } break;

    case 5: { // ECS-driven random shapes; circles separate
        //std::cout << "ecs rando " << std::endl;
        if (!gMode5Initialized) {
            InitializeMode5EcsRandomShapes(500);
        }
        InstancingSystem::InstancedParams params{};
        params.textured = gMode5TexturesEnabled;
        params.textureName = textureDucky;
        params.tint = CEVec3(1.f, 1.f, 1.f);
        params.uvSize = CEVec2(1.f, 1.f);

        // Circles FIRST to avoid covering ECS rectangles/triangles
        if (!gM5CircPos.empty()) {
            if (!gMode5TexturesEnabled) {
                InstancingSystem::CreateInstancedCircles(
                    gM5CircPos.data(), gM5CircColor.data(),
                    gM5CircRot.data(), gM5CircScale.data(),
                    gM5CircRadius.data(),
                    static_cast<int>(gM5CircPos.size()),
                    gM5CircSegments
                );
            } else {
                InstancingSystem::CreateInstancedCirclesTextured(
                    params.textureName,
                    gM5CircPos.data(), gM5CircColor.data(),
                    gM5CircRot.data(), gM5CircScale.data(),
                    gM5CircRadius.data(),
                    static_cast<int>(gM5CircPos.size()),
                    gM5CircSegments,
                    params.tint, params.uvSize
                );
            }
        }

        // Rectangles and Triangles after circles
        if (!gM5RectPos.empty()) {
            std::vector<float> rectScales(gM5RectPos.size(), 1.0f);
            if (!gMode5TexturesEnabled) {
                InstancingSystem::CreateInstancedRectangles(
                    gM5RectPos.data(), gM5RectColor.data(),
                    gM5RectRot.data(), rectScales.data(),
                    gM5RectW.data(), gM5RectH.data(),
                    static_cast<int>(gM5RectPos.size())
                );
            } else {
                InstancingSystem::CreateInstancedRectanglesTextured(
                    params.textureName,
                    gM5RectPos.data(), gM5RectColor.data(),
                    gM5RectRot.data(), rectScales.data(),
                    gM5RectW.data(), gM5RectH.data(),
                    static_cast<int>(gM5RectPos.size()),
                    params.tint, params.uvSize
                );
            }
        }
        if (!gM5TriPos.empty()) {
            if (!gMode5TexturesEnabled) {
                InstancingSystem::CreateInstancedTriangles(
                    gM5TriPos.data(), gM5TriColor.data(),
                    gM5TriRot.data(), gM5TriScale.data(),
                    gM5TriSide.data(),
                    static_cast<int>(gM5TriPos.size())
                );
            } else {
                InstancingSystem::CreateInstancedTrianglesTextured(
                    params.textureName,
                    gM5TriPos.data(), gM5TriColor.data(),
                    gM5TriRot.data(), gM5TriScale.data(),
                    gM5TriSide.data(),
                    static_cast<int>(gM5TriPos.size()),
                    params.tint, params.uvSize
                );
            }
        }
    } break;
    case 6: { // ECS ducks
        if (!gMode6Initialized) {
            InstancingSystem::InitializeEcsDuckSpritesRandom(1000);
            gMode6Initialized = true;
        }
        InstancingSystem::RenderEcsDuckSprites();
    } break;
    }
}

void FreeRan()
{
    CrashoutsEngine::Serialization::SaveScenes(sceneName);
    EnsureDuckCleaned();


    gRectsInitialized = false;

    gInstPositions.clear();
    gInstColors.clear();
    gInstRotations.clear();
    gInstScales.clear();

    gRndRectPos.clear();
    gRndRectColor.clear();
    gRndRectRot.clear();
    gRndRectW.clear();
    gRndRectH.clear();
}

void UnloadRan()
{
}

void CycleDisplayMode()
{
}

void UpdateSpriteAnimation()
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
