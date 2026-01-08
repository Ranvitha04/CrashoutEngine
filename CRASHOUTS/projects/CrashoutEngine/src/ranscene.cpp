/*!
@file        ranvitha.cpp
@author      ranvitha@digipen.edu
@date        18/09/2025
@brief
This source file implements the **Ran game state** logic for the CrashoutsEngine-based
project. It contains functions responsible for initializing, updating, and rendering
game elements within the "Ran" state, including ECS-driven random shape generation,
background rendering, and state transitions.

The file primarily handles:
- **Mode 5 ECS shape initialization**: Randomized creation of rectangles, triangles,
  and circles, including color, position, and rotation attributes.
- **Resource management**: Loading textures, cleaning up sprites, and resetting ECS data.
- **Rendering**: Drawing the background and instanced geometry based on the current mode.
- **Input handling**: Responding to key presses (e.g., switching display modes with the G key).

This module interacts closely with the `InstancingSystem` and the global ECS manager to
maintain a flexible and data-driven rendering flow for the Ran state. It serves as the
entry point for Ran's lifecycle functions — including initialization (`InitRan`),
rendering (`RenderBackground`), and updating (`UpdateRan`).

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
#include "spritecomponent.h"
#include "particleSystem.h"
#include <random>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_set>

#include "namecomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"


static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "ran";

// Display/control state
static int gDisplayMode = 0;
static bool gPrevGPressed = false;
static bool gPrevMPressed = false;          // toggle instancing type (texture/plain)
static bool gSpriteInitializedInScene = false;
static Component::ParticleEmitter* gParticleEmitter = nullptr;
static Component::FireEmitter* gFireEmitter = nullptr;
static Component::FireballEmitter* gFireballEmitter = nullptr;
static Component::ExplosionEmitter* gExplosionEmitter = nullptr;
static Component::TrajectoryEmitter* gTrajectoryEmitter = nullptr;


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
static std::vector<CEColor> gM5RectColor;    // 0..1 colors
static std::vector<float>  gM5RectRot;
static std::vector<float>  gM5RectScale;    // keep 1.0f and bake into width/height
static std::vector<float>  gM5RectW;
static std::vector<float>  gM5RectH;
static std::vector<CEVec2>  gM5RectOffset;

static std::vector<CEVec3> gM5TriPos;
static std::vector<CEColor> gM5TriColor;     // 0..1 colors
static std::vector<float>  gM5TriRot;
static std::vector<float>  gM5TriScale;     // use 1.0f
static std::vector<float>  gM5TriSide;      // derived from t1..t3
static std::vector<CEVec2>  gM5TriOffset;

static std::vector<CEVec3> gM5CircPos;
static std::vector<CEColor> gM5CircColor;
static std::vector<float>  gM5CircRot;
static std::vector<float>  gM5CircScale;
static std::vector<float>  gM5CircRadius;
static std::vector<CEVec2>  gM5CircOffset;
static int gM5CircSegments = 24;

// Mode 6 (ECS ducks)
static bool gMode6Initialized = false;
static bool gPrevIPressed = false;

// Mode 10 (All 3 particle systems together)
static bool gMode10Initialized = false;

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
    Component::Animator::m_sprite.init("assests/images/backgrounds/8k.png", 4, 6, 0.3f);
    Component::Animator::m_sprite.addAnimationState("idle_normal", Component::AnimationState(0, 2, 0.22f, true));
    Component::Animator::m_sprite.addAnimationState("walk_normal", Component::AnimationState(6, 6, 0.09f, true));
    Component::Animator::m_sprite.addAnimationState("idle_bounce", Component::AnimationState(12, 4, 0.20f, true));
    Component::Animator::m_sprite.addAnimationState("walk_bounce", Component::AnimationState(16, 2, 0.10f, true));
    Component::Animator::m_sprite.setAnimationState("walk_normal");
    Component::Animator::m_sprite.setPosition(gDuckX, gDuckY);
    Component::Animator::m_sprite.setScale(gDuckScale);
    Component::Animator::m_sprite.setRotation(gDuckRotationDeg);

    InstancingSystem::InitializeSingleEcsDuck(gDuckX, gDuckY, gDuckRotationDeg, gDuckScale);
    InstancingSystem::InitializeSecondEcsDuck(gDuckX + 10.f, gDuckY, gDuckRotationDeg, gDuckScale);
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
        sc.color = CEColor{ static_cast<float>(color255(rng)), static_cast<float>(color255(rng)), static_cast<float>(color255(rng)) };
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
        sc.color = CEColor{ static_cast<float>(color255(rng)), static_cast<float>(color255(rng)), static_cast<float>(color255(rng)) };
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
        CEColor c01{ sc->color.r * (1.f / 255.f), sc->color.g * (1.f / 255.f), sc->color.b * (1.f / 255.f) };

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
        gM5CircOffset.emplace_back(0.0f, 0.0f);
    }

    gMode5Initialized = true;
}

/*!
@brief Cleanup the single duck sprite state and GPU resources.
@details Ensures sprite resources are released when leaving mode 1.
*/
static void EnsureDuckCleaned() {
    if (!gSpriteInitializedInScene) return;
    Component::Animator::m_sprite.cleanup();
    CrashoutsEngine::Entity e1 = InstancingSystem::GetSingleDuckEntity();
    CrashoutsEngine::Entity e2 = InstancingSystem::GetSecondDuckEntity();
    if (e1) globalECS->DestroyEntity(e1);
    if (e2) globalECS->DestroyEntity(e2);
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

    //CrashoutsEngine::Assets::GenerateAssetManifest();
    //CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
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
        //"assets/images/bg 1 base.png",  // Use an existing texture file
        "assests/images/backgrounds/8k.png",
        CEVec3(0.0f, 0.0f, 0.0f),
        0.0f,
        CEColor(1.0f, 1.0f, 1.0f)
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
- Press `G` to cycle display modes (0..10) and reinitialize mode-specific data.
- Press `M` to toggle textures in modes 2/3/5.
- Mode 1: Update duck movement, scale, and rotation.
- Mode 6: Update ECS duck motion and handle `I` key to cycle manual animation.
- Mode 10: render all 3 particle systems together.
*/
void UpdateRan()
{
    // Toggle display mode with G 
    int gState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_G);
    bool gPressed = (gState == GLFW_PRESS);
    if (gPressed && !gPrevGPressed) {
        int prevMode = gDisplayMode;
        int nextMode = (gDisplayMode + 1) % 12;  // Changed from 10 to 11 to include mode 10
        gDisplayMode = nextMode;
        if (prevMode == 6) {
            gMode6Initialized = false;
        }
        if (gDisplayMode != 3) {
            // Protect emitter-owned entities from being destroyed during the global sweep.
            const CrashoutsEngine::ECS::ComponentArray<Component::NameComponent>& nameArray =
                globalECS->GetComponentArray<Component::NameComponent>();
            if (!nameArray.indexToEntity.empty()) {
                std::unordered_set<CrashoutsEngine::Entity> protectedIds;
                // collect entities that belong to emitters so we don't destroy them
                if (gParticleEmitter) {
                    const auto& pe = gParticleEmitter->GetEntities();
                    for (auto e : pe) protectedIds.insert(e);
                }
                if (gFireEmitter) {
                    const auto& fe = gFireEmitter->GetEntities();
                    for (auto e : fe) protectedIds.insert(e);
                }
                if (gFireballEmitter) {
                    const auto& t = gFireballEmitter->GetTrailEntities();
                    for (auto e : t) protectedIds.insert(e);
                    CrashoutsEngine::Entity core = gFireballEmitter->GetCoreEntity();
                    if (core) protectedIds.insert(core);
                }
                if (gExplosionEmitter) {
                    const auto& ee = gExplosionEmitter->GetEntities();
                    for (auto e : ee) protectedIds.insert(e);
                }
                if (gTrajectoryEmitter) {
                    const auto& te = gTrajectoryEmitter->GetEntities();
                    for (auto e : te) protectedIds.insert(e);
                }

                for (auto id : nameArray.indexToEntity) {
                    if (protectedIds.find(id) == protectedIds.end()) {
                        globalECS->DestroyEntity(id);
                    }
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

        // Initialize particle system when entering mode 7
        if (gDisplayMode == 7) {
            if (!gParticleEmitter) {
                gParticleEmitter = new Component::ParticleEmitter(5000);
                float worldW, worldH; ComputeWorldDimensions(worldW, worldH);
                gParticleEmitter->Initialize(CEVec3(0.f, 0.f, 0.f));
                gParticleEmitter->SetEmissionRate(1500.f);
                gParticleEmitter->SetLifeRange(0.6f, 1.2f);
                gParticleEmitter->SetSpeedRange(12.f, 22.f);
                gParticleEmitter->SetGravity(CEVec3(0.f, -30.f, 0.f));
                gParticleEmitter->SetSizeRange(0.35f, 0.0f);
                gParticleEmitter->SetColorRange(CEColor(1.f, 0.8f, 0.2f), CEColor(0.1f, 0.1f, 0.1f));
            }
        }

        // Initialize fire system when entering mode 8
        if (gDisplayMode == 8) {
            if (!gFireEmitter) {
                gFireEmitter = new Component::FireEmitter(4000);
                gFireEmitter->Initialize(CEVec3(0.f, -20.f, 0.f));
                gFireEmitter->SetEmissionRate(2000.f);
                gFireEmitter->SetLifeRange(0.4f, 0.9f);
                gFireEmitter->SetSpeedRange(6.f, 14.f);
                gFireEmitter->SetGravity(CEVec3(0.f, 20.f, 0.f));
                gFireEmitter->SetSizeRange(0.6f, 0.0f);
                gFireEmitter->SetColorRange(CEColor(1.f,0.9f,0.4f), CEColor(0.3f,0.05f,0.0f));
                gFireEmitter->SetSpread(0.6f);
            }
        }

        // Initialize fireball system when entering mode 9
        if (gDisplayMode == 9) {
            if (!gFireballEmitter) {
                float worldW, worldH; ComputeWorldDimensions(worldW, worldH);
                gFireballEmitter = new Component::FireballEmitter(5000);
                gFireballEmitter->Initialize(CEVec3(0.f, -worldH * 0.45f, 0.f));
                gFireballEmitter->SetBounds(worldW, worldH);
                gFireballEmitter->SetVelocity(CEVec3(0.f, 0.f, 0.f));
                gFireballEmitter->SetRadius(4.f);
                gFireballEmitter->SetEmissionRate(1600.f);
                gFireballEmitter->SetLifeRange(0.35f, 0.85f);
                gFireballEmitter->SetSpeedRange(8.f, 18.f);
                gFireballEmitter->SetGravity(CEVec3(0.f, 0.f, 0.f));
                gFireballEmitter->SetColorRange(CEColor(1.f,0.8f,0.3f), CEColor(0.4f,0.05f,0.0f));
                gFireballEmitter->SetSpread(0.5f);
            }
        }

        // Initialize all 3 particle systems when entering mode 10
        if (gDisplayMode == 10) {
            // Ensure world dims available once
            float worldW, worldH; ComputeWorldDimensions(worldW, worldH);

            // ParticleEmitter: create if missing, then always Initialize to rebuild ECS entities
            if (!gParticleEmitter) gParticleEmitter = new Component::ParticleEmitter(5000);
            gParticleEmitter->Initialize(CEVec3(-worldW * 0.3f, 0.f, 0.f));
            gParticleEmitter->SetEmissionRate(1500.f);
            gParticleEmitter->SetLifeRange(0.6f, 1.2f);
            gParticleEmitter->SetSpeedRange(12.f, 22.f);
            gParticleEmitter->SetGravity(CEVec3(0.f, -30.f, 0.f));
            gParticleEmitter->SetSizeRange(0.35f, 0.0f);
            gParticleEmitter->SetColorRange(CEColor(1.f, 0.8f, 0.2f), CEColor(0.1f, 0.1f, 0.1f));

            // FireEmitter: create if missing, then Initialize (rebuilds ECS entities)
            if (!gFireEmitter) gFireEmitter = new Component::FireEmitter(4000);
            gFireEmitter->Initialize(CEVec3(0.f, -20.f, 0.f));
            gFireEmitter->SetEmissionRate(2000.f);
            gFireEmitter->SetLifeRange(0.4f, 0.9f);
            gFireEmitter->SetSpeedRange(6.f, 14.f);
            gFireEmitter->SetGravity(CEVec3(0.f, 20.f, 0.f));
            gFireEmitter->SetSizeRange(0.6f, 0.0f);
            gFireEmitter->SetColorRange(CEColor(1.f,0.9f,0.4f), CEColor(0.3f,0.05f,0.0f));
            gFireEmitter->SetSpread(0.6f);

            // FireballEmitter: create if missing, then Initialize (rebuilds ECS entities)
            if (!gFireballEmitter) gFireballEmitter = new Component::FireballEmitter(5000);
            gFireballEmitter->Initialize(CEVec3(worldW * 0.3f, -worldH * 0.45f, 0.f));
            gFireballEmitter->SetBounds(worldW, worldH);
            gFireballEmitter->SetVelocity(CEVec3(0.f, 0.35f, 0.f));
            gFireballEmitter->SetRadius(4.f);
            gFireballEmitter->SetEmissionRate(1600.f);
            gFireballEmitter->SetLifeRange(0.35f, 0.85f);
            gFireballEmitter->SetSpeedRange(8.f, 18.f);
            gFireballEmitter->SetGravity(CEVec3(0.f, 0.f, 0.f));
            gFireballEmitter->SetColorRange(CEColor(1.f,0.8f,0.3f), CEColor(0.4f,0.05f,0.0f));
            gFireballEmitter->SetSpread(0.5f);  

            if (!gExplosionEmitter) {
                gExplosionEmitter = new Component::ExplosionEmitter(5000);
                gExplosionEmitter->Initialize(CEVec3(0.f, 20.f, 0.f));
                gExplosionEmitter->SetEmissionRate(100);
                gExplosionEmitter->SetLifeRange(0.25f, 0.6f);
                gExplosionEmitter->SetSpeedRange(12.f, 28.f);
                gExplosionEmitter->SetGravity(CEVec3(0.f, -10.f, 0.f));
                gExplosionEmitter->SetColorRange(CEColor(1.f, 0.7f, 0.2f), CEColor(0.25f, 0.25f, 0.25f));
                gExplosionEmitter->SetSpread(1.0f);
                gExplosionEmitter->SetBurst(1200, 1.8f);
                gExplosionEmitter->TriggerBurst();
            }

            gMode10Initialized = true;
        }

        if (gDisplayMode == 11) {
            if (!gExplosionEmitter) {
                gExplosionEmitter = new Component::ExplosionEmitter(5000);
                gExplosionEmitter->Initialize(CEVec3(0.f, 0.f, 0.f));
                gExplosionEmitter->SetEmissionRate(100);
                gExplosionEmitter->SetLifeRange(0.25f, 0.6f);
                gExplosionEmitter->SetSpeedRange(12.f, 28.f);
                gExplosionEmitter->SetGravity(CEVec3(0.f, -10.f, 0.f));
                gExplosionEmitter->SetColorRange(CEColor(1.f, 0.7f, 0.2f), CEColor(0.25f, 0.25f, 0.25f));
                gExplosionEmitter->SetSpread(1.0f);
                gExplosionEmitter->SetBurst(1200, 1.8f);
                gExplosionEmitter->TriggerBurst();
            }
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

            // Seed rectangles via InstancingSystem's existing ECS helper
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

        if (prevMode == 4 && nextMode != 4) {

            gMode4Initialized = false;

        }

        // Cleanup particle system when leaving mode 7
        if (prevMode == 7 && nextMode != 7 && nextMode != 10) {
            if (gParticleEmitter) { gParticleEmitter->Cleanup(); delete gParticleEmitter; gParticleEmitter = nullptr; }
        }
        // Cleanup fire system when leaving mode 8
        if (prevMode == 8 && nextMode != 8 && nextMode != 10) {
            if (gFireEmitter) { gFireEmitter->Cleanup(); delete gFireEmitter; gFireEmitter = nullptr; }
        }
        // Cleanup fireball system when leaving mode 9
        if (prevMode == 9 && nextMode != 9 && nextMode != 10) {
            if (gFireballEmitter) { gFireballEmitter->Cleanup(); delete gFireballEmitter; gFireballEmitter = nullptr; }
        }
        // Cleanup all particle systems when leaving mode 10
        if (prevMode == 10 && nextMode != 10) {
            gMode10Initialized = false;
            if (gParticleEmitter) { gParticleEmitter->Cleanup(); delete gParticleEmitter; gParticleEmitter = nullptr; }
            if (gFireEmitter) { gFireEmitter->Cleanup(); delete gFireEmitter; gFireEmitter = nullptr; }
            if (gFireballEmitter) { gFireballEmitter->Cleanup(); delete gFireballEmitter; gFireballEmitter = nullptr; }
        }

        
        if (prevMode == 11 && nextMode != 11) {
            if (gExplosionEmitter) { gExplosionEmitter->Cleanup(); delete gExplosionEmitter; gExplosionEmitter = nullptr; }
        }

        if (prevMode == 1 && nextMode != 1) {
            if (gTrajectoryEmitter) { gTrajectoryEmitter->Cleanup(); delete gTrajectoryEmitter; gTrajectoryEmitter = nullptr; }
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
            bool flip = ((move * dirX) < 0.0f);

            // Write to ECS
            auto e = InstancingSystem::GetSingleDuckEntity();
            if (auto* T = globalECS->GetComponent<Component::TransformComponent>(e)) {
                T->position.x = gDuckX; T->position.y = gDuckY;
            }
            if (auto* DM = globalECS->GetComponent<Component::DuckMotionComponent>(e)) {
                DM->animationMode = 2; // walk_normal
            }
            if (auto* A = globalECS->GetComponent<Component::SpriteAnimComponent>(e)) {
                A->flip = flip;
            }
        }
        else {
            auto e = InstancingSystem::GetSingleDuckEntity();
            if (auto* DM = globalECS->GetComponent<Component::DuckMotionComponent>(e)) {
                DM->animationMode = 3; // idle_bounce
            }
        }

        int sState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_S);
        int qState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_Q);
        bool scaleUp = (sState == GLFW_PRESS) || (sState == GLFW_REPEAT);
        bool scaleDown = (qState == GLFW_PRESS) || (qState == GLFW_REPEAT);
        if (scaleUp || scaleDown) {
            float delta = DUCK_SCALE_SPEED * dt * (scaleUp ? 1.0f : -1.0f);
            gDuckScale = std::max(0.1f, gDuckScale + delta);
            auto e = InstancingSystem::GetSingleDuckEntity();
            if (auto* T = globalECS->GetComponent<Component::TransformComponent>(e)) {
                T->size.x = gDuckScale; T->size.y = gDuckScale;
            }
            auto e2 = InstancingSystem::GetSecondDuckEntity();
            if (auto* T2 = globalECS->GetComponent<Component::TransformComponent>(e2)) {
                T2->size.x = gDuckScale; T2->size.y = gDuckScale;
            }
        }

        int rState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_R);
        int tState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_T);
        bool rotLeft = (rState == GLFW_PRESS) || (rState == GLFW_REPEAT);
        bool rotRight = (tState == GLFW_PRESS) || (tState == GLFW_REPEAT);
        if (rotLeft || rotRight) {
            float deltaDeg = DUCK_ROT_SPEED * dt * (rotRight ? 1.0f : -1.0f);
            gDuckRotationDeg += deltaDeg;
            if (gDuckRotationDeg > 360.0f) gDuckRotationDeg -= 360.0f;
            if (gDuckRotationDeg < -360.0f) gDuckRotationDeg += 360.0f;

            auto e = InstancingSystem::GetSingleDuckEntity();
            if (auto* T = globalECS->GetComponent<Component::TransformComponent>(e)) {
                T->rotation = gDuckRotationDeg;
            }
            auto e2 = InstancingSystem::GetSecondDuckEntity();
            if (auto* T2 = globalECS->GetComponent<Component::TransformComponent>(e2)) {
                T2->rotation = gDuckRotationDeg;
            }
        }

        int leftState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_LEFT);
        int rightState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_RIGHT);
        int upState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_UP);
        int downState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_DOWN);
        bool left = (leftState == GLFW_PRESS) || (leftState == GLFW_REPEAT);
        bool right = (rightState == GLFW_PRESS) || (rightState == GLFW_REPEAT);
        bool up = (upState == GLFW_PRESS) || (upState == GLFW_REPEAT);
        bool down = (downState == GLFW_PRESS) || (downState == GLFW_REPEAT);
        float dx = 0.f; float dy = 0.f;
        if (left && !right) dx = -DUCK_SPEED * dt;
        else if (right && !left) dx = DUCK_SPEED * dt;
        if (up && !down) dy = DUCK_SPEED * dt;
        else if (down && !up) dy = -DUCK_SPEED * dt;
        auto e2 = InstancingSystem::GetSecondDuckEntity();
        if (auto* T2 = globalECS->GetComponent<Component::TransformComponent>(e2)) {
            T2->position.x += dx;
            T2->position.y += dy;
        }
        if (auto* DM2 = globalECS->GetComponent<Component::DuckMotionComponent>(e2)) {
            if (dx != 0.f || dy != 0.f) DM2->animationMode = 2; else DM2->animationMode = 3;
        }
        if (auto* A2 = globalECS->GetComponent<Component::SpriteAnimComponent>(e2)) {
            if (left && !right) A2->flip = true; else if (right && !left) A2->flip = false;
        }
        if (!gTrajectoryEmitter) {
            gTrajectoryEmitter = new Component::TrajectoryEmitter(64);
            CEVec3 a = CEVec3(0.f,0.f,0.f);
            CEVec3 b = CEVec3(0.f,0.f,0.f);
            auto e1 = InstancingSystem::GetSingleDuckEntity();
            e2 = InstancingSystem::GetSecondDuckEntity();
            if (auto* t1 = globalECS->GetComponent<Component::TransformComponent>(e1)) {
                a = t1->position;
            }
            if (auto* t2 = globalECS->GetComponent<Component::TransformComponent>(e2)) {
                b = t2->position;
            }
            gTrajectoryEmitter->Initialize(a,b);
            gTrajectoryEmitter->SetMass(1.f);
            gTrajectoryEmitter->SetSpeed(22.f);
            gTrajectoryEmitter->SetGravity(9.8f);
        }
        gTrajectoryEmitter->Update(dt);
        InstancingSystem::UpdateEcsDuckSprites();
    }

    // ECS ducks Update for mode 6
    if (gDisplayMode == 6 && gMode6Initialized) {
        InstancingSystem::UpdateEcsDuckSprites();
    }

    if (gDisplayMode == 7 && gParticleEmitter) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        gParticleEmitter->Update(dt);
    }
    if (gDisplayMode == 8 && gFireEmitter) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        gFireEmitter->Update(dt);
    }
    if (gDisplayMode == 9 && gFireballEmitter) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        gFireballEmitter->Update(dt);
    }
    
    // Update all 3 particle systems in mode 10
    if (gDisplayMode == 10 && gMode10Initialized) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        if (gParticleEmitter) gParticleEmitter->Update(dt);
        if (gFireEmitter) gFireEmitter->Update(dt);
        if (gFireballEmitter) gFireballEmitter->Update(dt);
		if (gExplosionEmitter) gExplosionEmitter->Update(dt);
    }

    // Update explosion system in mode 11
    if (gDisplayMode == 11 && gExplosionEmitter) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        gExplosionEmitter->Update(dt);
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
    case 1:
        InstancingSystem::RenderEcsDuckSprites();
        if (gTrajectoryEmitter) {
            gTrajectoryEmitter->Render(false);
        }
        break;
    case 2: //Mode M toggles texture

        if (!gRectsInitialized) {
            InstancingSystem::InitializeInstancedRectanglesData(2500);
            gRectsInitialized = true;
        }

        // Render instanced rectangles from InstancingSystem internal arrays
        {
            InstancingSystem::InstancedParams params{};
            if (gInstancingType == 0) {
                params.textured = false;
            }
            else {
                params.textured = true;
                params.textureName = textureDucky;
                params.tint = CEColor(1.f, 1.f, 1.f);
                params.uvSize = CEVec2(1.f, 1.f);
            }
            InstancingSystem::RenderInstancedRectangles(params);
        }
        break;
    case 3:

        break;
    case 4: { // Non-ECS instanced random triangles, rectangles, circles with texture toggl

        // No re-init here; seed in UpdateRan or when entering mode
        InstancingSystem::InstancedParams params{};
        params.textured = gMode4TexturesEnabled;
        params.textureName = textureDucky;
        params.tint = CEColor(1.f, 1.f, 1.f);
        params.uvSize = CEVec2(1.f, 1.f);

        // Draw order: circles -> triangles -> rectangles
        InstancingSystem::RenderMode4Circles(params);
        InstancingSystem::RenderMode4Triangles(params);
        InstancingSystem::RenderMode4Rectangles(params);
    } break;

    case 5: { // ECS-driven random shapes

        if (!gMode5Initialized) {
            InitializeMode5EcsRandomShapes(500);
        }
        InstancingSystem::InstancedParams params{};
        params.textured = gMode5TexturesEnabled;
        params.textureName = textureDucky;
        params.tint = CEColor(1.f, 1.f, 1.f);
        params.uvSize = CEVec2(1.f, 1.f);

        // Circles
        if (!gM5CircPos.empty()) {
            if (!gMode5TexturesEnabled) {
                InstancingSystem::CreateInstancedCircles(
                    gM5CircPos.data(), gM5CircColor.data(),
                    gM5CircRot.data(), gM5CircScale.data(),
                    gM5CircRadius.data(),
                    gM5CircOffset.data(), // CHANGED: Passed Offset
                    static_cast<int>(gM5CircPos.size()),
                    gM5CircSegments
                );
            }
            else {
                InstancingSystem::CreateInstancedCirclesTextured(
                    params.textureName,
                    gM5CircPos.data(), gM5CircColor.data(),
                    gM5CircRot.data(), gM5CircScale.data(),
                    gM5CircRadius.data(),
                    gM5CircOffset.data(), // CHANGED: Passed Offset
                    static_cast<int>(gM5CircPos.size()),
                    gM5CircSegments,
                    params.tint, params.uvSize
                );
            }
        }

        // Rectangles and Triangles
        if (!gM5RectPos.empty()) {
            std::vector<float> rectScales(gM5RectPos.size(), 1.0f);
            if (!gMode5TexturesEnabled) {
                InstancingSystem::CreateInstancedRectangles(
                    gM5RectPos.data(), gM5RectColor.data(),
                    gM5RectRot.data(), rectScales.data(),
                    gM5RectW.data(), gM5RectH.data(),
                    gM5RectOffset.data(), // CHANGED: Passed Offset
                    static_cast<int>(gM5RectPos.size())
                );
            }
            else {
                InstancingSystem::CreateInstancedRectanglesTextured(
                    params.textureName,
                    gM5RectPos.data(), gM5RectColor.data(),
                    gM5RectRot.data(), rectScales.data(),
                    gM5RectW.data(), gM5RectH.data(),
                    gM5RectOffset.data(), // CHANGED: Passed Offset
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
                    gM5TriSide.data(),
                    gM5TriOffset.data(), // CHANGED: Passed Offset
                    static_cast<int>(gM5TriPos.size())
                );
            }
            else {
                InstancingSystem::CreateInstancedTrianglesTextured(
                    params.textureName,
                    gM5TriPos.data(), gM5TriColor.data(),
                    gM5TriRot.data(), gM5TriScale.data(),
                    gM5TriSide.data(),
                    gM5TriSide.data(),
                    gM5TriOffset.data(), // CHANGED: Passed Offset
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
    case 7: { // Particle system
        if (gParticleEmitter) {
            gParticleEmitter->Render(false);
        }
    } break;
    case 8: { // Fire system
        if (gFireEmitter) {
            gFireEmitter->Render(false);
        }
    } break;
    case 9: { // Fireball system
        if (gFireballEmitter) {
            gFireballEmitter->Render(false);
        }
    } break;
    case 10: { // All 3 particle systems together
        if (gParticleEmitter) {
            gParticleEmitter->Render(false);
        }
        if (gFireEmitter) {
            gFireEmitter->Render(false);
        }
        if (gFireballEmitter) {
            gFireballEmitter->Render(false);
        }
        if (gExplosionEmitter) {
            gExplosionEmitter->Render(false);
        }
    } break;
    case 11: { // Explosion system
        if (gExplosionEmitter) {
            gExplosionEmitter->Render(false);
        }
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

    if (gParticleEmitter) { gParticleEmitter->Cleanup(); delete gParticleEmitter; gParticleEmitter = nullptr; }
    if (gFireEmitter) { gFireEmitter->Cleanup(); delete gFireEmitter; gFireEmitter = nullptr; }
    if (gFireballEmitter) { gFireballEmitter->Cleanup(); delete gFireballEmitter; gFireballEmitter = nullptr; }
    if (gExplosionEmitter) { gExplosionEmitter->Cleanup(); delete gExplosionEmitter; gExplosionEmitter = nullptr; }
    if (gTrajectoryEmitter) { gTrajectoryEmitter->Cleanup(); delete gTrajectoryEmitter; gTrajectoryEmitter = nullptr; }
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
    using Component::Animator;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_LEFT:
        case GLFW_KEY_A:
            Animator::m_sprite.setFlipHorizontal(true);
            break;
        case GLFW_KEY_RIGHT:
        case GLFW_KEY_D:
            Animator::m_sprite.setFlipHorizontal(false);
            break;
        default:
            break;
        }
    }

    
}
