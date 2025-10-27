
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
static int gDisplayMode = 1;                // 0:bg, 1:duck, 2:instanced, 3:imgui-only
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
static const float DUCK_ROT_SPEED   = 90.0f;  // degrees per second
static float gDuckT = 0.0f;

// Instanced squares state
static bool gSquaresInitialized = false;
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

// Rectangles (Mode 4)
static std::vector<CEVec3> gM4RectPos;
static std::vector<CEVec3> gM4RectColor;    // 0..1 colors
static std::vector<float>  gM4RectRot;
static std::vector<float>  gM4RectW;
static std::vector<float>  gM4RectH;

// Triangles (Mode 4)
static std::vector<CEVec3> gM4TriPos;
static std::vector<CEVec3> gM4TriColor;     // 0..1 colors
static std::vector<float>  gM4TriRot;
static std::vector<float>  gM4TriScale;     // use 1.0f
static std::vector<float>  gM4TriSide;      // equilateral side length

// Circles (Mode 4)
static std::vector<CEVec3> gM4CircPos;
static std::vector<CEVec3> gM4CircColor;    // 0..1 colors
static std::vector<float>  gM4CircRot;
static std::vector<float>  gM4CircScale;    // use 1.0f
static std::vector<float>  gM4CircRadius;
static int gM4CircSegments = 24;

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

// Circles (Mode 5) — not in ECS; generate directly
// Circles (Mode 5) — not in ECS; generate directly
static std::vector<CEVec3> gM5CircPos;
static std::vector<CEVec3> gM5CircColor;
static std::vector<float>  gM5CircRot;
static std::vector<float>  gM5CircScale;
static std::vector<float>  gM5CircRadius;
static int gM5CircSegments = 24;


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

//static void InitializeInstancedSquaresData(int count) {
//    gInstPositions.clear();
//    gInstColors.clear();
//    gInstRotations.clear();
//    gInstScales.clear();
//    gInstPositions.reserve(count);
//    gInstColors.reserve(count);
//    gInstRotations.reserve(count);
//    gInstScales.reserve(count);
//
//    float worldW, worldH;
//    ComputeWorldDimensions(worldW, worldH);
//
//    std::random_device rd;
//    std::mt19937 gen(rd());
//    std::uniform_real_distribution<float> distX(-worldW / 2.0f, worldW / 2.0f);
//    std::uniform_real_distribution<float> distY(-worldH / 2.0f, worldH / 2.0f);
//    std::uniform_real_distribution<float> distC(0.2f, 1.0f);
//    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
//    std::uniform_real_distribution<float> distScale(0.5f, 2.0f);
//
//    for (int i = 0; i < count; ++i) {
//        gInstPositions.emplace_back(distX(gen), distY(gen), 0.0f);
//        gInstColors.emplace_back(distC(gen), distC(gen), distC(gen));
//        gInstRotations.emplace_back(distRot(gen));
//        gInstScales.emplace_back(distScale(gen));
//    }
//    gSquaresInitialized = true;
//}

static void InitializeInstancedRectanglesData(int count) {
    gRndRectPos.clear();
    gRndRectColor.clear();
    gRndRectRot.clear();
    gRndRectW.clear();
    gRndRectH.clear();
    gRndRectPos.reserve(count);
    gRndRectColor.reserve(count);
    gRndRectRot.reserve(count);
    gRndRectW.reserve(count);
    gRndRectH.reserve(count);

    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(-worldW / 2.0f, worldW / 2.0f);
    std::uniform_real_distribution<float> distY(-worldH / 2.0f, worldH / 2.0f);
    std::uniform_real_distribution<float> distC(0.2f, 1.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
    std::uniform_real_distribution<float> distW(1.0f, 6.0f);
    std::uniform_real_distribution<float> distH(1.0f, 6.0f);

    for (int i = 0; i < count; ++i) {
        gRndRectPos.emplace_back(distX(gen), distY(gen), 0.0f);
        gRndRectColor.emplace_back(distC(gen), distC(gen), distC(gen));
        gRndRectRot.emplace_back(distRot(gen));
        gRndRectW.emplace_back(distW(gen));
        gRndRectH.emplace_back(distH(gen));
    }
    gRectsInitialized = true;
}

static void InitializeEcsRectanglesDataRandom(int count) {
    //if (!globalECS) return;

    //float worldW = 0.0f, worldH = 0.0f;
    //ComputeWorldDimensions(worldW, worldH);

    //std::random_device rd;
    //std::mt19937 rng(rd());

    //std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    //std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
    //std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);

    //// Sizes scaled to world dimensions, similar to non-ECS rectangles
    //std::uniform_real_distribution<float> widthDist(worldW * 0.01f, worldW * 0.05f);
    //std::uniform_real_distribution<float> heightDist(worldH * 0.01f, worldH * 0.05f);

    //// 0–255 range; renderer multiplies by 1/255.f later
    //std::uniform_int_distribution<int> colorDist(70, 255);

    //for (int i = 0; i < count; ++i) {
    //    auto e = globalECS->CreateEntity();

    //    CEVec3 pos{ posX(rng), posY(rng), 0.0f };
    //    float rotation = rotDeg(rng);
    //    float w = widthDist(rng);
    //    float h = heightDist(rng);

    //    // Name + Transform
    //    globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Rect_" + std::to_string(e) });
    //    globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
    //        rotation, CEVec2{1.0f, 1.0f}, pos
    //    });

    //    // Shape
    //    Component::ShapeComponent shape{};
    //    shape.type = Component::ShapeType::Rectangle;
    //    shape.width = w;
    //    shape.height = h;
    //    shape.color = CEVec3{
    //        static_cast<float>(colorDist(rng)),
    //        static_cast<float>(colorDist(rng)),
    //        static_cast<float>(colorDist(rng))
    //    };

    //    shape.enableTexture = gEcsTexturesEnabled;
    //    shape.textureName = textureDucky; // "ducky.png"
    //    shape.uvSize = CEVec2{ 1.0f, 1.0f };

    //    globalECS->AddComponent<Component::ShapeComponent>(e, shape);
    //}

    //gEcsRectsInitialized = true;
//{
    if (!globalECS) return;

    float worldW = 0.0f, worldH = 0.0f;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
    std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);

    // Sizes scaled to world dimensions, similar to non-ECS rectangles
    std::uniform_real_distribution<float> widthDist(worldW * 0.01f, worldW * 0.05f);
    std::uniform_real_distribution<float> heightDist(worldH * 0.01f, worldH * 0.05f);

    // 0–255 range; renderer multiplies by 1/255.f later
    std::uniform_int_distribution<int> colorDist(70, 255);

    for (int i = 0; i < count; ++i) {
        auto e = globalECS->CreateEntity();

        CEVec3 pos{ posX(rng), posY(rng), 0.0f };
        float rotation = rotDeg(rng);
        float w = widthDist(rng);
        float h = heightDist(rng);

        // Name + Transform
        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Rect_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
            rotation, CEVec2{1.0f, 1.0f}, pos
        });

        // Shape
        Component::ShapeComponent shape{};
        shape.type = Component::ShapeType::Rectangle;
        shape.width = w;
        shape.height = h;
        shape.color = CEVec3{
            static_cast<float>(colorDist(rng)),
            static_cast<float>(colorDist(rng)),
            static_cast<float>(colorDist(rng))
        };

        shape.enableTexture = gEcsTexturesEnabled;
        shape.textureName = textureDucky; // "ducky.png"
        shape.uvSize = CEVec2{ 1.0f, 1.0f };

        globalECS->AddComponent<Component::ShapeComponent>(e, shape);
    }

    gEcsRectsInitialized = true;
}

static void EnsureDuckInitialized() {
    if (gSpriteInitializedInScene) return;
    Component::Sprite::m_sprite.init("../Descent/assets/images/ducky_2_spritesheet.png", 4, 6, 0.3f);
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

static void InitializeMode4RandomShapes(int perTypeCount) {
    gMode4Initialized = false;

    // Clear all
    gM4RectPos.clear(); gM4RectColor.clear(); gM4RectRot.clear(); gM4RectW.clear(); gM4RectH.clear();
    gM4TriPos.clear();  gM4TriColor.clear();  gM4TriRot.clear();  gM4TriScale.clear(); gM4TriSide.clear();
    gM4CircPos.clear(); gM4CircColor.clear(); gM4CircRot.clear(); gM4CircScale.clear(); gM4CircRadius.clear();

    // Reserve
    gM4RectPos.reserve(perTypeCount); gM4RectColor.reserve(perTypeCount);
    gM4RectRot.reserve(perTypeCount); gM4RectW.reserve(perTypeCount); gM4RectH.reserve(perTypeCount);
    gM4TriPos.reserve(perTypeCount);  gM4TriColor.reserve(perTypeCount);
    gM4TriRot.reserve(perTypeCount);  gM4TriScale.reserve(perTypeCount); gM4TriSide.reserve(perTypeCount);
    gM4CircPos.reserve(perTypeCount); gM4CircColor.reserve(perTypeCount);
    gM4CircRot.reserve(perTypeCount); gM4CircScale.reserve(perTypeCount); gM4CircRadius.reserve(perTypeCount);

    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
    std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);

    std::uniform_real_distribution<float> col01(0.2f, 1.0f);
    std::uniform_real_distribution<float> rectW(1.0f, 6.0f);
    std::uniform_real_distribution<float> rectH(1.0f, 6.0f);
    std::uniform_real_distribution<float> triSide(1.0f, 6.0f);
    std::uniform_real_distribution<float> circRadius(1.0f, 4.0f);

    // Rectangles
    for (int i = 0; i < perTypeCount; ++i) {
        gM4RectPos.emplace_back(posX(rng), posY(rng), 0.0f);
        gM4RectColor.emplace_back(col01(rng), col01(rng), col01(rng));
        gM4RectRot.emplace_back(rotDeg(rng));
        gM4RectW.emplace_back(rectW(rng));
        gM4RectH.emplace_back(rectH(rng));
    }

    // Triangles (equilateral side length)
    for (int i = 0; i < perTypeCount; ++i) {
        gM4TriPos.emplace_back(posX(rng), posY(rng), 0.0f);
        gM4TriColor.emplace_back(col01(rng), col01(rng), col01(rng));
        gM4TriRot.emplace_back(rotDeg(rng));
        gM4TriScale.emplace_back(1.0f);
        gM4TriSide.emplace_back(triSide(rng));
    }

    // Circles
    for (int i = 0; i < perTypeCount; ++i) {
        gM4CircPos.emplace_back(posX(rng), posY(rng), 0.0f);
        gM4CircColor.emplace_back(col01(rng), col01(rng), col01(rng));
        gM4CircRot.emplace_back(rotDeg(rng));
        gM4CircScale.emplace_back(1.0f);
        gM4CircRadius.emplace_back(circRadius(rng));
    }

    gMode4Initialized = true;
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
        float w = rectW(rng), h = rectH(rng);

        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "M5Rect_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ rotation, CEVec2{1.0f, 1.0f}, pos });

        Component::ShapeComponent sc{};
        sc.type = Component::ShapeType::Rectangle;
        sc.width = w;
        sc.height = h;
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
        float s = triSide(rng);
        // Equilateral triangle points scaled by side length
        // Height = sqrt(3)/2 * s; center at origin
        float h = 0.866025403784f * s;
        Component::ShapeComponent sc{};
        sc.type = Component::ShapeType::Triangle;
        sc.t1 = CEVec3(0.0f, 2.0f / 3.0f * h, 0.0f);            // top
        sc.t2 = CEVec3(-0.5f * s, -1.0f / 3.0f * h, 0.0f);      // bottom-left
        sc.t3 = CEVec3(0.5f * s, -1.0f / 3.0f * h, 0.0f);      // bottom-right
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
            gM5RectW.push_back(sc->width * tfScale.x);
            gM5RectH.push_back(sc->height * tfScale.y);
        } break;
        case Component::ShapeType::Triangle: {
            gM5TriPos.push_back(pos);
            gM5TriColor.push_back(c01);
            gM5TriRot.push_back(rot);
            gM5TriScale.push_back(1.0f);

            // Derive side length from t1..t3 (use edge t2-t3)
            auto dx = sc->t3.x - sc->t2.x;
            auto dy = sc->t3.y - sc->t2.y;
            float side = std::sqrt(dx * dx + dy * dy);
            gM5TriSide.push_back(side);
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
    gPrevMPressed = false;
    gSquaresInitialized = false;
    gRectsInitialized = false;
    gSpriteInitializedInScene = false;  // allow EnsureDuckInitialized() to run
}

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

void UpdateRan()
{
    // Toggle display mode with G 
    int gState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_G);
    bool gPressed = (gState == GLFW_PRESS);
    if (gPressed && !gPrevGPressed) {
        int prevMode = gDisplayMode;
        int nextMode = (gDisplayMode + 1) % 6; 
        gDisplayMode = nextMode;

        if (gDisplayMode == 1) {
            EnsureDuckInitialized();
        } else {
            EnsureDuckCleaned();
        }

        // Prepare data when entering instanced mode
        if (gDisplayMode == 2) {
            gSquaresInitialized = false;
            gRectsInitialized = false;
        }

        // Initialize randomized ECS rectangles when entering ECS mode (3)
        if (gDisplayMode == 3 && !gEcsRectsInitialized) {
            gEcsTexturesEnabled = false;        
            InitializeEcsRectanglesDataRandom(2500);
        }

        // Initialize Mode 4 random shapes (non-ECS)
        if (gDisplayMode == 4 && !gMode4Initialized) {
            gMode4TexturesEnabled = false;
            InitializeMode4RandomShapes(500);   // 500 rects, 500 tris, 500 circles
        }

        // Initialize Mode 5 ECS random shapes
        if (gDisplayMode == 5 && !gMode5Initialized) {
            gMode5TexturesEnabled = false;
            InitializeMode5EcsRandomShapes(500); // 500 rects, 500 tris + 500 circles
        }

        
        if (prevMode == 3 && nextMode != 3) {

            // gEcsRectsInitialized = false;
        }
    }

    
    int mState = glfwGetKey(GLApp::ptr_window, GLFW_KEY_M);
    bool mPressed = (mState == GLFW_PRESS);
    if (mPressed && !gPrevMPressed) {
        if (gDisplayMode == 2) {
            gInstancingType = (gInstancingType == 0) ? 1 : 0;
        } else if (gDisplayMode == 3) {
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
        } else if (gDisplayMode == 4) {

            gMode4TexturesEnabled = !gMode4TexturesEnabled;

        } else if (gDisplayMode == 5) {

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
            Component::Sprite::m_sprite.setAnimationState("walk_normal");
        } else {
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
    gPrevMPressed = mPressed;
}

void RenderRan()
{
    // framebuffer is cleared every frame
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
        case 2: //Mode M toggles texture
            //if (!gSquaresInitialized) {
            //    InitializeInstancedSquaresData(25000);
            //}
            if (!gRectsInitialized) {
                InitializeInstancedRectanglesData(2500);
            }
            // Squares (existing)
            //RenderInstancedSquares();

            // Rectangles (instanced) using InstancingSystem directly
            if (!gRndRectPos.empty()) {
                std::vector<float> rectScales(gRndRectPos.size(), 1.0f);

                if (gInstancingType == 0) {
                    InstancingSystem::CreateInstancedRectangles(
                        gRndRectPos.data(),
                        gRndRectColor.data(),
                        gRndRectRot.data(),
                        rectScales.data(),
                        gRndRectW.data(),
                        gRndRectH.data(),
                        static_cast<int>(gRndRectPos.size())
                    );
                } else {
                    //const std::string textureDucky = "../Descent/assets/images/ducky_2_spritesheet.png";
                    const CEVec3 tint{ 1.f, 1.f, 1.f };
                    const CEVec2 uvSize{ 1.f, 1.f };
                    InstancingSystem::CreateInstancedRectanglesTextured(
                        textureDucky,
                        gRndRectPos.data(),
                        gRndRectColor.data(),
                        gRndRectRot.data(),
                        rectScales.data(),
                        gRndRectW.data(),
                        gRndRectH.data(),
                        static_cast<int>(gRndRectPos.size()),
                        tint,
                        uvSize
                    );
                }
            }
            break;
        case 3:
            Scene::RenderScene(*globalECS);
            break;
        case 4: { // Non-ECS instanced random triangles, rectangles, circles with texture toggle
            if (!gMode4Initialized) {
                InitializeMode4RandomShapes(500);
            }
            const CEVec3 tint{ 1.f, 1.f, 1.f };
            const CEVec2 uv{ 1.f, 1.f };

            // Rectangles
            if (!gM4RectPos.empty()) {
                std::vector<float> rectScales(gM4RectPos.size(), 1.0f);
                if (!gMode4TexturesEnabled) {
                    InstancingSystem::CreateInstancedRectangles(
                        gM4RectPos.data(), gM4RectColor.data(),
                        gM4RectRot.data(), rectScales.data(),
                        gM4RectW.data(), gM4RectH.data(),
                        static_cast<int>(gM4RectPos.size())
                    );
                } else {
                    InstancingSystem::CreateInstancedRectanglesTextured(
                        textureDucky,
                        gM4RectPos.data(), gM4RectColor.data(),
                        gM4RectRot.data(), rectScales.data(),
                        gM4RectW.data(), gM4RectH.data(),
                        static_cast<int>(gM4RectPos.size()),
                        tint, uv
                    );
                }
            }

            // Triangles
            if (!gM4TriPos.empty()) {
                if (!gMode4TexturesEnabled) {
                    InstancingSystem::CreateInstancedTriangles(
                        gM4TriPos.data(), gM4TriColor.data(),
                        gM4TriRot.data(), gM4TriScale.data(),
                        gM4TriSide.data(),
                        static_cast<int>(gM4TriPos.size())
                    );
                } else {
                    InstancingSystem::CreateInstancedTrianglesTextured(
                        textureDucky,
                        gM4TriPos.data(), gM4TriColor.data(),
                        gM4TriRot.data(), gM4TriScale.data(),
                        gM4TriSide.data(),
                        static_cast<int>(gM4TriPos.size()),
                        tint, uv
                    );
                }
            }

            // Circles
            if (!gM4CircPos.empty()) {
                if (!gMode4TexturesEnabled) {
                    InstancingSystem::CreateInstancedCircles(
                        gM4CircPos.data(), gM4CircColor.data(),
                        gM4CircRot.data(), gM4CircScale.data(),
                        gM4CircRadius.data(),
                        static_cast<int>(gM4CircPos.size()),
                        gM4CircSegments
                    );
                } else {
                    InstancingSystem::CreateInstancedCirclesTextured(
                        textureDucky,
                        gM4CircPos.data(), gM4CircColor.data(),
                        gM4CircRot.data(), gM4CircScale.data(),
                        gM4CircRadius.data(),
                        static_cast<int>(gM4CircPos.size()),
                        gM4CircSegments,
                        tint, uv
                    );
                }
            }
        } break;
        case 5: { // ECS-driven instanced random shapes (rectangles + triangles), circles separate; 'M' toggles textures
            if (!gMode5Initialized) {
                InitializeMode5EcsRandomShapes(500);
            }
            const CEVec3 tint{ 1.f, 1.f, 1.f };
            const CEVec2 uv{ 1.f, 1.f };

            // Rectangles from ECS
            if (!gM5RectPos.empty()) {
                if (!gMode5TexturesEnabled) {
                    InstancingSystem::CreateInstancedRectangles(
                        gM5RectPos.data(), gM5RectColor.data(),
                        gM5RectRot.data(), gM5RectScale.data(),
                        gM5RectW.data(), gM5RectH.data(),
                        static_cast<int>(gM5RectPos.size())
                    );
                } else {
                    InstancingSystem::CreateInstancedRectanglesTextured(
                        textureDucky,
                        gM5RectPos.data(), gM5RectColor.data(),
                        gM5RectRot.data(), gM5RectScale.data(),
                        gM5RectW.data(), gM5RectH.data(),
                        static_cast<int>(gM5RectPos.size()),
                        tint, uv
                    );
                }
            }

            // Triangles from ECS
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
                        textureDucky,
                        gM5TriPos.data(), gM5TriColor.data(),
                        gM5TriRot.data(), gM5TriScale.data(),
                        gM5TriSide.data(),
                        static_cast<int>(gM5TriPos.size()),
                        tint, uv
                    );
                }
            }

            // Circles (not ECS)
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
                        textureDucky,
                        gM5CircPos.data(), gM5CircColor.data(),
                        gM5CircRot.data(), gM5CircScale.data(),
                        gM5CircRadius.data(),
                        static_cast<int>(gM5CircPos.size()),
                        gM5CircSegments,
                        tint, uv
                    );
                }
            }
        } break;
    }
}

void FreeRan()
{
    CrashoutsEngine::Serialization::SaveScenes(sceneName);
    EnsureDuckCleaned();

    gSquaresInitialized = false;
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
