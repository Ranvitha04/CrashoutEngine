/*!
@file        instanceSystem.cpp
@author      ranvitha@digipen.edu
@date        6/11/2025
@brief
This header file declares the interfaces and data structures for the **Instancing System**,
a subsystem responsible for efficiently rendering large numbers of geometric shapes (circles,
rectangles, and triangles) using OpenGL instanced drawing.

The Instancing System provides both textured and non-textured variants for each shape type,
allowing for batch rendering of multiple instances with unique transformations, colors, and
scales. It supports integration with the CrashoutsEngine ECS (Entity Component System) to
enable entity-based rendering and data-driven initialization of instance buffers.

Key functionality includes:
- Instanced rendering setup for circles, rectangles, and triangles.
- Textured instancing with tint and UV scaling support.
- Randomized ECS entity initialization for procedural scenes.
- Shared GPU buffer management for efficient rendering performance.

This file contains only declarations; implementations are provided in the corresponding
`instancing_system.cpp` file.
*/

#include <cmath>
#include "instanceSystem.h"
#include "shapes.h"
#include "CEMath.h"
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <unordered_map>
#include "spritecomponent.h"
#include "frameratecontroller.h"
#include "glapp.h"
#include "textureasset.h"
#include "rendering.h"

#include "transformcomponent.h"
#include "namecomponent.h"
#include "shapescomponent.h"

// Global variables needed for ECS and texturing
static CrashoutsEngine::ECS* globalECS = nullptr;
const std::string textureDucky = "ducky.png";
static bool gEcsRectsInitialized = false;
static bool gEcsTexturesEnabled = false;

// Function to initialize the globalECS variable
static void EnsureGlobalECSInitialized() {
    if (!globalECS) {
        globalECS = CrashoutsEngine::ECS::getInstance();
    }
}

namespace InstancingSystem {

    //Circle Instancing buffers
    static GLuint s_circInstanceVAO = 0;
    static GLuint s_circInstanceVBO = 0;
    static GLuint s_circInstanceEBO = 0;
    static GLuint s_circInstanceDataVBO = 0;
    static bool   s_circInstanceBuffersInitialized = false;
    static int    s_circSegmentsCurrent = 0;

    // Rectangle instancing buffers
    static GLuint s_rectInstanceVAO = 0;
    static GLuint s_rectInstanceVBO = 0;  
    static GLuint s_rectInstanceEBO = 0;  
    static GLuint s_rectInstanceDataVBO = 0;  
    static bool s_rectInstanceBuffersInitialized = false;

    // Triangle instancing buffers
    static GLuint s_triInstanceVAO = 0;
    static GLuint s_triInstanceVBO = 0;
    static GLuint s_triInstanceEBO = 0;
    static GLuint s_triInstanceDataVBO = 0;
    static bool   s_triInstanceBuffersInitialized = false;

 

    // ECS duck sprites state
    static bool s_duckSpritesInitialized = false;
    static GLuint s_duckSheetHandle = 0;
    static const char* DUCK_SPRITESHEET_ASSET = "ducky_2_spritesheet";
    static const char* DUCK_SPRITESHEET_PATH  = "assets/images/ducky_2_spritesheet.png";
    static const int DUCK_ROWS = 4;
    static const int DUCK_COLS = 6;
    static const CEVec2 DUCK_SIZE = CEVec2(3.0f, 3.0f);

    struct DuckAnimState {
        int   startFrame{0};
        int   frameCount{1};
        float frameTime{0.1f};
        float timer{0.0f};
        int   frameIndex{0};
        bool  flip{false};
    };

    static CrashoutsEngine::Entity s_singleDuckId = 0;
    static CrashoutsEngine::Entity s_secondDuckId = 0;

    static DuckAnimState MakeAnimByMode(int mode) {
        DuckAnimState s{};
        switch (mode) {
            case 1: // idle_normal
                s.startFrame = 0;  s.frameCount = 2; s.frameTime = 0.8f; break;
            case 2: // walk_normal
                s.startFrame = 6;  s.frameCount = 6; s.frameTime = 0.12f; break;
            case 3: // idle_bounce
                s.startFrame = 12; s.frameCount = 4; s.frameTime = 0.15f; break;
            case 4: // walk_bounce
                s.startFrame = 16; s.frameCount = 2; s.frameTime = 0.10f; break;
            default:
                s.startFrame = 0;  s.frameCount = 2; s.frameTime = 0.8f; break;
        }
        return s;
    }

    /*!\n    @brief Compute world dimensions using window aspect.\n    @details Keeps a consistent base dimension and scales the other axis to match aspect ratio.\n    */
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

    /*!\n    @brief Initialize randomized ECS duck entities and per-entity animation state.\n    @param count Number of ducks to spawn\n    @details Preloads spritesheet, seeds randomized motion values, sets world bounds, and creates animation entries.\n    */
    void InitializeEcsDuckSpritesRandom(int count) {
        EnsureGlobalECSInitialized();
        if (!globalECS) return;

        // Preload spritesheet and fetch handle for flipped rendering
        Component::TextureRenderer::PreloadTexture(DUCK_SPRITESHEET_PATH);
        s_duckSheetHandle = Component::TextureRenderer::GetTextureHandle(DUCK_SPRITESHEET_ASSET);

        float worldW = 0.0f, worldH = 0.0f;
        ComputeWorldDimensions(worldW, worldH);

        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
        std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
        std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);
        std::uniform_real_distribution<float> spd(6.0f, 12.0f);
        std::uniform_real_distribution<float> trn(15.0f, 60.0f);
        std::uniform_real_distribution<float> osc(3.0f, 10.0f);
        std::uniform_int_distribution<int> animMode(1, 4);
        std::uniform_real_distribution<float> dir(-1.0f, 1.0f);

        for (int i = 0; i < count; ++i) {
            auto e = globalECS->CreateEntity();

            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);

            // Name + Transform
            globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Duck_" + std::to_string(e) });
            globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
                rotation, CEVec2{1.0f, 1.0f}, pos
            });

            // Motion component
            Component::DuckMotionComponent dm{};
            dm.speed = spd(rng);
            dm.turnRate = trn(rng);
            dm.oscillation = osc(rng);
            dm.noiseTime = 0.0f;

            // Random initial velocity (normalized)
            CEVec3 v{ dir(rng), dir(rng), 0.0f };
            float len = std::sqrt(v.x * v.x + v.y * v.y);
            if (len < 1e-3f) v = CEVec3{1.0f, 0.0f, 0.0f}; else { v.x /= len; v.y /= len; }
            dm.velocity = v;

            // Bounds = world bounds 
            dm.SetBoundsFromWorld(worldW, worldH);
            dm.ExpandBounds(-2.0f);

            dm.animationMode = animMode(rng);
            globalECS->AddComponent<Component::DuckMotionComponent>(e, dm);

            // Sprite sheet + animation components
            Component::SpriteSheetComponent sheet{};
            sheet.textureAsset = DUCK_SPRITESHEET_ASSET;
            sheet.cols = DUCK_COLS;
            sheet.rows = DUCK_ROWS;
            sheet.worldSize = DUCK_SIZE;
            globalECS->AddComponent<Component::SpriteSheetComponent>(e, sheet);

            DuckAnimState defaults = MakeAnimByMode(dm.animationMode);
            Component::SpriteAnimComponent anim{};
            anim.startFrame = defaults.startFrame;
            anim.frameCount = defaults.frameCount;
            anim.frameTime = defaults.frameTime;
            anim.timer = 0.0f;
            anim.frameIndex = 0;
            anim.flip = false;
            globalECS->AddComponent<Component::SpriteAnimComponent>(e, anim);
        }

        s_duckSpritesInitialized = true;
    }

    /*!\n    @brief Update duck motion, bounds, facing rotation, and per-entity animation timers.\n    @details Honors manual animationMode when non-zero, otherwise auto-chooses idle/walk based on speed.\n    */
    void UpdateEcsDuckSprites() {
        if (!globalECS) return;
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());

        auto& arr = globalECS->GetComponentArray<Component::DuckMotionComponent>();
        for (size_t i = 0; i < arr.indexToEntity.size(); ++i) {
            auto id = arr.indexToEntity[i];
            auto* dm = globalECS->GetComponent<Component::DuckMotionComponent>(id);
            auto* tf = globalECS->GetComponent<Component::TransformComponent>(id);
            auto* anim = globalECS->GetComponent<Component::SpriteAnimComponent>(id);
            if (!dm || !tf || !anim) continue;

            // Update noise and gently steer
            dm->noiseTime += dt;
            float steer = std::sin(dm->noiseTime * 1.7f) * (dm->turnRate * 0.0174533f) * dt; // turnRate deg/s -> rad/s
            // Rotate velocity by small angle
            float cs = std::cos(steer), sn = std::sin(steer);
            CEVec3 v = dm->velocity;
            dm->velocity = CEVec3{ v.x * cs - v.y * sn, v.x * sn + v.y * cs, 0.0f };

            // Integrate position
            tf->position.x += dm->velocity.x * dm->speed * dt;
            tf->position.y += dm->velocity.y * dm->speed * dt;

            // Bounce within bounds
            bool clamped = dm->ClampToBounds(tf->position);
            if (clamped) {
                // Invert velocity when hitting bounds
                if (tf->position.x <= dm->boundsMin.x || tf->position.x >= dm->boundsMax.x) dm->velocity.x *= -1.0f;
                if (tf->position.y <= dm->boundsMin.y || tf->position.y >= dm->boundsMax.y) dm->velocity.y *= -1.0f;
            }

            // Face movement direction only when auto-moving (speed > 0)
            if (dm->speed > 0.0f) {
                tf->rotation = std::atan2(dm->velocity.y, dm->velocity.x) * 57.2957795f;
            }

            // Animation update
            float speedMag = std::sqrt(dm->velocity.x * dm->velocity.x + dm->velocity.y * dm->velocity.y) * dm->speed;
            const float moveThresh = 1.5f;
            int desiredMode = dm->animationMode; // 1..4 manual modes; 0 = auto
            if (desiredMode <= 0) {
                desiredMode = (speedMag > moveThresh) ? 2 : 3; // auto: walk_normal vs idle_bounce
            }
            DuckAnimState desired = MakeAnimByMode(desiredMode);
            bool changed = anim->startFrame != desired.startFrame || anim->frameCount != desired.frameCount || anim->frameTime != desired.frameTime;
            if (changed) {
                anim->startFrame = desired.startFrame;
                anim->frameCount = desired.frameCount;
                anim->frameTime = desired.frameTime;
                anim->timer = 0.0f;
                anim->frameIndex = 0;
            }

            // Flip based on velocity only when auto-moving; otherwise preserve external control
            if (dm->speed > 0.0f) {
                anim->flip = (dm->velocity.x < 0.0f);
            }

            anim->timer += dt;
            if (anim->timer >= anim->frameTime) {
                anim->timer -= anim->frameTime;
                anim->frameIndex = (anim->frameIndex + 1) % anim->frameCount;
            }
        }
    }

    /*!\n    @brief Render ECS ducks for the current frame using the duck spritesheet.\n    @details Computes spritesheet frame XY from per-entity animation state; applies rotation, scale, and optional flip.\n    */
    void RenderEcsDuckSprites() {
        if (!globalECS) return;

        if (s_duckSheetHandle == 0) {
            s_duckSheetHandle = Component::TextureRenderer::GetTextureHandle(DUCK_SPRITESHEET_ASSET);
        }

        auto& arr = globalECS->GetComponentArray<Component::SpriteAnimComponent>();
        for (size_t i = 0; i < arr.indexToEntity.size(); ++i) {
            auto id = arr.indexToEntity[i];
            auto* tf    = globalECS->GetComponent<Component::TransformComponent>(id);
            auto* anim  = globalECS->GetComponent<Component::SpriteAnimComponent>(id);
            auto* sheet = globalECS->GetComponent<Component::SpriteSheetComponent>(id);
            if (!tf || !anim || !sheet) continue;

            int globalFrame = anim->startFrame + anim->frameIndex;
            int frameY = globalFrame / sheet->cols;
            int frameX = globalFrame % sheet->cols;

            GLuint handle = Component::TextureRenderer::GetTextureHandle(sheet->textureAsset);
            CEColor tintColor = CEColor(1.0f, 1.0f, 1.0f);
            if (id == s_secondDuckId) {
                tintColor = CEColor(0.85f, 0.4f, 1.0f);
            }
            if (handle != 0) {
                Component::TextureRenderer::CreateSpriteFrameFlipped(
                    handle,
                    tf->position,
                    sheet->worldSize,
                    frameX, frameY,
                    sheet->cols, sheet->rows,
                    anim->flip,
                    tf->rotation,
                    tf->size.x,
                    tintColor,
                    false
                );
            } else {
                Component::TextureRenderer::CreateSpriteFrame(
                    sheet->textureAsset,
                    tf->position,
                    sheet->worldSize,
                    frameX, frameY,
                    sheet->cols, sheet->rows,
                    tf->rotation,
                    tf->size.x,
                    tintColor,
                    false
                );
            }
        }
    }

    /*!\n    @brief Advance ducks' manual animation mode (1..4) and reset animation timers/indices.\n    @details Manual mode prevents auto-switching until set back to 0.\n    */
    void CycleEcsDuckAnimationState() {
        if (!globalECS) return;
        auto& arr = globalECS->GetComponentArray<Component::DuckMotionComponent>();
        for (size_t i = 0; i < arr.indexToEntity.size(); ++i) {
            auto id = arr.indexToEntity[i];
            auto* dm = globalECS->GetComponent<Component::DuckMotionComponent>(id);
            auto* anim = globalECS->GetComponent<Component::SpriteAnimComponent>(id);
            if (!dm || !anim) continue;

            dm->animationMode = (dm->animationMode % 4) + 1;
            DuckAnimState st = MakeAnimByMode(dm->animationMode);
            anim->startFrame = st.startFrame;
            anim->frameCount = st.frameCount;
            anim->frameTime  = st.frameTime;
            anim->timer = 0.0f;
            anim->frameIndex = 0;
        }
    }

    // Initialize a single duck for display mode 1 using ECS
    void InitializeSingleEcsDuck(float x, float y, float rotationDeg, float uniformScale) {
        EnsureGlobalECSInitialized();
        if (!globalECS) return;

        // Clear previous single duck if any
        if (s_singleDuckId != 0) {
            globalECS->DestroyEntity(s_singleDuckId);
            s_singleDuckId = 0;
        }

        // Ensure texture handle is ready
        Component::TextureRenderer::PreloadTexture(DUCK_SPRITESHEET_PATH);
        s_duckSheetHandle = Component::TextureRenderer::GetTextureHandle(DUCK_SPRITESHEET_ASSET);

        auto e = globalECS->CreateEntity();
        s_singleDuckId = e;

        // Name + Transform
        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Duck_Single" });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
            rotationDeg, CEVec2{uniformScale, uniformScale}, CEVec3{x, y, 0.0f}
        });

        // Motion (used for animation mode selection and flip logic if desired)
        Component::DuckMotionComponent dm{};
        dm.speed = 0.0f;                   // keyboard-controlled, not auto-integrated
        dm.turnRate = 0.0f;
        dm.oscillation = 0.0f;
        dm.noiseTime = 0.0f;
        dm.velocity = CEVec3{1.0f, 0.0f, 0.0f}; // default facing +X
        dm.animationMode = 2;              // default walk
        dm.SetBoundsFromWorld(100.0f, 60.0f); // harmless defaults
        globalECS->AddComponent<Component::DuckMotionComponent>(e, dm);

        // Sprite sheet + anim
        Component::SpriteSheetComponent sheet{};
        sheet.textureAsset = DUCK_SPRITESHEET_ASSET;
        sheet.cols = DUCK_COLS;
        sheet.rows = DUCK_ROWS;
        sheet.worldSize = DUCK_SIZE;
        globalECS->AddComponent<Component::SpriteSheetComponent>(e, sheet);

        DuckAnimState defaults = MakeAnimByMode(dm.animationMode);
        Component::SpriteAnimComponent anim{};
        anim.startFrame = defaults.startFrame;
        anim.frameCount = defaults.frameCount;
        anim.frameTime  = defaults.frameTime;
        anim.timer = 0.0f;
        anim.frameIndex = 0;
        anim.flip = false;
        globalECS->AddComponent<Component::SpriteAnimComponent>(e, anim);
    }

    void InitializeSecondEcsDuck(float x, float y, float rotationDeg, float uniformScale) {
        EnsureGlobalECSInitialized();
        if (!globalECS) return;
        if (s_secondDuckId != 0) {
            globalECS->DestroyEntity(s_secondDuckId);
            s_secondDuckId = 0;
        }
        Component::TextureRenderer::PreloadTexture(DUCK_SPRITESHEET_PATH);
        s_duckSheetHandle = Component::TextureRenderer::GetTextureHandle(DUCK_SPRITESHEET_ASSET);
        auto e = globalECS->CreateEntity();
        s_secondDuckId = e;
        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Duck_Purple" });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ rotationDeg, CEVec2{uniformScale, uniformScale}, CEVec3{x, y, 0.0f} });
        Component::DuckMotionComponent dm{};
        dm.speed = 0.0f;
        dm.turnRate = 0.0f;
        dm.oscillation = 0.0f;
        dm.noiseTime = 0.0f;
        dm.velocity = CEVec3{1.0f, 0.0f, 0.0f};
        dm.animationMode = 2;
        dm.SetBoundsFromWorld(100.0f, 60.0f);
        globalECS->AddComponent<Component::DuckMotionComponent>(e, dm);
        Component::SpriteSheetComponent sheet{};
        sheet.textureAsset = DUCK_SPRITESHEET_ASSET;
        sheet.cols = DUCK_COLS;
        sheet.rows = DUCK_ROWS;
        sheet.worldSize = DUCK_SIZE;
        globalECS->AddComponent<Component::SpriteSheetComponent>(e, sheet);
        DuckAnimState defaults = MakeAnimByMode(dm.animationMode);
        Component::SpriteAnimComponent anim{};
        anim.startFrame = defaults.startFrame;
        anim.frameCount = defaults.frameCount;
        anim.frameTime  = defaults.frameTime;
        anim.timer = 0.0f;
        anim.frameIndex = 0;
        anim.flip = false;
        globalECS->AddComponent<Component::SpriteAnimComponent>(e, anim);
    }

    CrashoutsEngine::Entity GetSecondDuckEntity() {
        return s_secondDuckId;
    }

CrashoutsEngine::Entity GetSingleDuckEntity() {
    return s_singleDuckId;
}

void InstancingSystem::InitializeEcsDuckSpritesRandom(int count, const std::string& assetName, int cols, int rows, CEVec2 worldSize) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    Component::TextureRenderer::PreloadTexture(assetName);
    float worldW = 0.0f, worldH = 0.0f;
    ComputeWorldDimensions(worldW, worldH);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
    std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);
    std::uniform_int_distribution<int> animMode(1, 4);
    for (int i = 0; i < count; ++i) {
        auto e = globalECS->CreateEntity();
        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Sprite_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ rotDeg(rng), CEVec2{1.0f, 1.0f}, CEVec3{ posX(rng), posY(rng), 0.0f } });
        Component::SpriteSheetComponent sheet{};
        sheet.textureAsset = assetName;
        sheet.cols = cols;
        sheet.rows = rows;
        sheet.worldSize = worldSize;
        globalECS->AddComponent<Component::SpriteSheetComponent>(e, sheet);
        DuckAnimState defaults = MakeAnimByMode(animMode(rng));
        Component::SpriteAnimComponent anim{};
        anim.startFrame = defaults.startFrame;
        anim.frameCount = defaults.frameCount;
        anim.frameTime = defaults.frameTime;
        anim.timer = 0.0f;
        anim.frameIndex = 0;
        anim.flip = false;
        globalECS->AddComponent<Component::SpriteAnimComponent>(e, anim);
    }
}

void InstancingSystem::InitializeSingleEcsDuck(float x, float y, float rotationDeg, float uniformScale, const std::string& assetName, int cols, int rows, CEVec2 worldSize) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    if (s_singleDuckId != 0) {
        globalECS->DestroyEntity(s_singleDuckId);
        s_singleDuckId = 0;
    }
    Component::TextureRenderer::PreloadTexture(assetName);
    auto e = globalECS->CreateEntity();
    s_singleDuckId = e;
    globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Sprite_Single" });
    globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{ rotationDeg, CEVec2{uniformScale, uniformScale}, CEVec3{x, y, 0.0f} });
    Component::SpriteSheetComponent sheet{};
    sheet.textureAsset = assetName;
    sheet.cols = cols;
    sheet.rows = rows;
    sheet.worldSize = worldSize;
    globalECS->AddComponent<Component::SpriteSheetComponent>(e, sheet);
    DuckAnimState defaults = MakeAnimByMode(2);
    Component::SpriteAnimComponent anim{};
    anim.startFrame = defaults.startFrame;
    anim.frameCount = defaults.frameCount;
    anim.frameTime = defaults.frameTime;
    anim.timer = 0.0f;
    anim.frameIndex = 0;
    anim.flip = false;
    globalECS->AddComponent<Component::SpriteAnimComponent>(e, anim);
}

void InstancingSystem::SetEntitySpriteSheet(CrashoutsEngine::Entity e, const std::string& assetName, int cols, int rows, CEVec2 worldSize) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    Component::TextureRenderer::PreloadTexture(assetName);
    auto* sheet = globalECS->GetComponent<Component::SpriteSheetComponent>(e);
    if (!sheet) {
        Component::SpriteSheetComponent s{};
        s.textureAsset = assetName;
        s.cols = cols;
        s.rows = rows;
        s.worldSize = worldSize;
        globalECS->AddComponent<Component::SpriteSheetComponent>(e, s);
    } else {
        sheet->textureAsset = assetName;
        sheet->cols = cols;
        sheet->rows = rows;
        sheet->worldSize = worldSize;
    }
}

    //.....................CIRCLE..............................................//
    void InstancingSystem::CreateInstancedCircles(const CEVec3* positions, const CEColor* colors,
        const float* rotations, const float* scales,
        const float* radii,
        int count, int segments) {

        if (count <= 0) return;
        if (segments < 3) segments = 3;

        Component::Rendering& renderer = Component::Rendering::Get();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        //glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        if (!s_circInstanceBuffersInitialized || s_circSegmentsCurrent != segments) {
            if (s_circInstanceBuffersInitialized) {
                glDeleteVertexArrays(1, &s_circInstanceVAO);
                glDeleteBuffers(1, &s_circInstanceVBO);
                glDeleteBuffers(1, &s_circInstanceEBO);
                glDeleteBuffers(1, &s_circInstanceDataVBO);
                s_circInstanceBuffersInitialized = false;
            }

            // Generate unit circle triangle fan vertices (center + perimeter)
            std::vector<MeshVertex> base_vertices;
            base_vertices.reserve(static_cast<size_t>(segments) + 1);
            base_vertices.push_back({ 0.0f, 0.0f, 0.0f, 0.5f, 0.5f }); // Center with UV center
            
            for (int i = 0; i <= segments; ++i) {
                float angle = 2.0f * static_cast<float>(CEMath::PI) * static_cast<float>(i) / static_cast<float>(segments);
                float x = std::cos(angle);
                float y = std::sin(angle);
                // Simple planar UV mapping
                base_vertices.push_back({ x, y, 0.0f, (x + 1.0f) * 0.5f, (y + 1.0f) * 0.5f });
            }

            // Triangle indices
            std::vector<GLushort> base_indices;
            base_indices.reserve(static_cast<size_t>(segments) * 3);
            for (int i = 0; i < segments; ++i) {
                base_indices.push_back(0);
                base_indices.push_back(static_cast<GLushort>(i + 1));
                base_indices.push_back(static_cast<GLushort>(i + 2));
            }

            // Acquire pooled buffers (or create new) via renderer
            s_circInstanceVAO = renderer.GetPooledVAO();
            s_circInstanceVBO = renderer.GetPooledVBO();
            s_circInstanceEBO = renderer.GetPooledEBO();
            s_circInstanceDataVBO = renderer.GetPooledVBO();

            glBindVertexArray(s_circInstanceVAO);

            // 1. Base Geometry (MeshVertex)
            glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, base_vertices.size() * sizeof(MeshVertex), base_vertices.data(), GL_STATIC_DRAW);

            // Loc 0: Pos
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, x));
            glEnableVertexAttribArray(0);
            // Loc 2: UV
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, u));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_circInstanceEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, base_indices.size() * sizeof(GLushort), base_indices.data(), GL_STATIC_DRAW);

            // 2. Instance Attributes (CircleInstanceData)
            glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);

            GLsizei stride = sizeof(CircleInstanceData);

            // Loc 3: Position (Vec3)
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CircleInstanceData, position));
            glEnableVertexAttribArray(3);
            glVertexAttribDivisor(3, 1);

            // Loc 4: Color (Vec4/CEColor) -- CHANGED TO 4 FLOATS
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CircleInstanceData, color));
            glEnableVertexAttribArray(4);
            glVertexAttribDivisor(4, 1);

            // Loc 5: Rotation
            glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CircleInstanceData, rotation));
            glEnableVertexAttribArray(5);
            glVertexAttribDivisor(5, 1);

            // Loc 6: Scale
            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CircleInstanceData, scale));
            glEnableVertexAttribArray(6);
            glVertexAttribDivisor(6, 1);

            // Loc 7: Radius (Width)
            glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CircleInstanceData, radius));
            glEnableVertexAttribArray(7);
            glVertexAttribDivisor(7, 1);

            // Loc 8: Padding/Height
            glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CircleInstanceData, padding));
            glEnableVertexAttribArray(8);
            glVertexAttribDivisor(8, 1);

            s_circSegmentsCurrent = segments;
            s_circInstanceBuffersInitialized = true;
            glBindVertexArray(0);
        }

        // --- Update Instance Data using Struct ---
        std::vector<CircleInstanceData> instanceData(count);
        for (int i = 0; i < count; ++i) {
            instanceData[i].position = positions[i];
            instanceData[i].color = colors[i];
            instanceData[i].rotation = rotations[i];
            instanceData[i].scale = scales[i];
            instanceData[i].radius = radii[i];
            instanceData[i].padding = radii[i]; // Reuse radius for height/padding
        }

        glBindVertexArray(s_circInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(CircleInstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

        renderer.shdr_pgm_instanced->Use();
        glm::mat4 model = glm::mat4(1.0f);
        renderer.shdr_pgm_instanced->SetUniform("uModel", model);

        // Setup Uniforms for non-textured circle
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);
        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.0f);

        int elements = segments * 3;
        glDrawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_SHORT, 0, count);

        glBindVertexArray(0);

        //glDepthMask(GL_TRUE); // Re-enable depth writing for next objects
        glDisable(GL_BLEND);  // Disable blending
        glEnable(GL_CULL_FACE);
    }

    void InstancingSystem::CreateInstancedCirclesTextured(const std::string& textureName,
        const CEVec3* positions, const CEColor* colors,
        const float* rotations, const float* scales,
        const float* radii,
        int count, int segments,
        const CEColor& tint,
        const CEVec2& uvSize) {
        if (count <= 0) return;
        if (segments < 3) segments = 3;

        if (count <= 0) return;

        glDisable(GL_CULL_FACE);

        // Reuse the exact same buffer init logic if uninitialized
        if (!s_circInstanceBuffersInitialized || s_circSegmentsCurrent != segments) {
            // Trick: call the non-textured init with dummy data 
            // (Or copy-paste the init block here. Calling the other function with 0 count handles init if refactored slightly, 
            // but safest to just rely on shared state or copy logic. For this response, assuming state is shared.)
            CEVec3 p; CEColor c; float f = 0;
            CreateInstancedCircles(&p, &c, &f, &f, &f, 0, segments); // Triggers init
        }

        // --- Update Instance Data using Struct ---
        std::vector<CircleInstanceData> instanceData(count);
        for (int i = 0; i < count; ++i) {
            instanceData[i].position = positions[i];
            instanceData[i].color = colors[i];
            instanceData[i].rotation = rotations[i];
            instanceData[i].scale = scales[i];
            instanceData[i].radius = radii[i];
            instanceData[i].padding = radii[i];
        }

        glBindVertexArray(s_circInstanceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_circInstanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(CircleInstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

        Component::Rendering& renderer = Component::Rendering::Get();
        GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureName);

        renderer.shdr_pgm_instanced->Use();
        renderer.shdr_pgm_instanced->SetUniform("uModel", glm::mat4(1.0f));

        // Uniforms
        GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_useTexture");
        if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

        GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_tint");
        if (tint_loc != -1) glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);

        GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_uvSize");
        if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);    // No blending
        glDepthMask(GL_TRUE);   // Write depth normally

        // Bind Texture
        glBindTextureUnit(0, textureHandle); // DSA or standard
        int elements = segments * 3;
        glDrawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_SHORT, 0, count);
        glBindTextureUnit(0, 0);

        glBindVertexArray(0);

        // 3. Reset State
        //glDepthMask(GL_TRUE); // Important!
        //glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    void InstancingSystem::RenderInstancedCirclesBatch(
    const CEVec3* positions,
    const CEColor* colors,
    const float* rotations,
    const float* scales,
    const float* radii,
    int count,
    int segments,
    const InstancedParams& params
) {
    if (count <= 0) return;

    if (!s_circInstanceBuffersInitialized || s_circSegmentsCurrent != segments) {
        // This function is a bit of a mess. It seems to be a legacy function that was replaced by CreateInstancedCircles.
        // For now, we will just call the new function.
        //SetupCircleInstanceBuffers(segments);
        s_circSegmentsCurrent = segments;
    }

    if (!params.textured) {
        CreateInstancedCircles(positions, colors, rotations, scales, radii, count, segments);
    }
    else {
        CreateInstancedCirclesTextured(params.textureName, positions, colors, rotations, scales, radii, count, segments, params.tint, params.uvSize);
    }
}



    // Rectangles (Mode 4)
    static std::vector<CEVec3> gM4RectPos;
    static std::vector<CEColor> gM4RectColor;    
    static std::vector<float>  gM4RectRot;
    static std::vector<float>  gM4RectW;
    static std::vector<float>  gM4RectH;

    // Triangles (Mode 4)
    static std::vector<CEVec3> gM4TriPos;
    static std::vector<CEColor> gM4TriColor;
    static std::vector<float>  gM4TriRot;
    static std::vector<float>  gM4TriScale;     
    static std::vector<float>  gM4TriSide;     

    // Circles (Mode 4)
    static std::vector<CEVec3> gM4CircPos;
    static std::vector<CEColor> gM4CircColor;
    static std::vector<float>  gM4CircRot;
    static std::vector<float>  gM4CircScale;    
    static std::vector<float>  gM4CircRadius;
    static int gM4CircSegments = 64;

    void InstancingSystem::InitializeMode4RandomShapes(int perTypeCount) {
        //gMode4Initialized = false;

        //ClearRectangleData();

        // Clear all
        gM4RectPos.clear(); gM4RectColor.clear(); gM4RectRot.clear(); gM4RectW.clear(); gM4RectH.clear();
        gM4TriPos.clear();  gM4TriColor.clear();  gM4TriRot.clear();  gM4TriScale.clear(); gM4TriSide.clear();
        gM4CircPos.clear(); gM4CircColor.clear(); gM4CircRot.clear(); gM4CircScale.clear(); gM4CircRadius.clear();

        // Reserve
        gM4RectPos.reserve(perTypeCount);gM4RectColor.reserve(perTypeCount);
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
            gM4RectColor.emplace_back(col01(rng), col01(rng), col01(rng), col01(rng));
            gM4RectRot.emplace_back(rotDeg(rng));
            gM4RectW.emplace_back(rectW(rng));
            gM4RectH.emplace_back(rectH(rng));
        }

        // Triangles (equilateral side length)
      /*  for (int i = 0; i < perTypeCount; ++i) {
            gM4TriPos.emplace_back(posX(rng), posY(rng), 0.0f);
            gM4TriColor.emplace_back(col01(rng), col01(rng), col01(rng));
            gM4TriRot.emplace_back(rotDeg(rng));
            gM4TriScale.emplace_back(1.0f);
            gM4TriSide.emplace_back(triSide(rng));
        }*/

        // Circles
        for (int i = 0; i < perTypeCount; ++i) {
            gM4CircPos.emplace_back(posX(rng), posY(rng), 0.0f);
            gM4CircColor.emplace_back(col01(rng), col01(rng), col01(rng), col01(rng));
            gM4CircRot.emplace_back(0.0f);
            gM4CircScale.emplace_back(1.0f);
            gM4CircRadius.emplace_back(circRadius(rng));
        }

        //gMode4Initialized = true;
    }

    // Build Mode 5 ECS-driven rectangles + triangles; circles generated separately
    void InitializeMode5EcsRandomShapes(
        CrashoutsEngine::ECS* ecs,
        int perTypeCount,
        bool texturesEnabled, const std::string& textureName,
        // Rectangles out
        std::vector<CEVec3>& rectPos, std::vector<CEColor>& rectColor,
        std::vector<float>& rectRot, std::vector<float>& rectScale,
        std::vector<float>& rectW, std::vector<float>& rectH,
        // Triangles out
        std::vector<CEVec3>& triPos, std::vector<CEColor>& triColor,
        std::vector<float>& triRot, std::vector<float>& triScale, std::vector<float>& triSide,
        // Circles out (not ECS)
        std::vector<CEVec3>& circPos, std::vector<CEColor>& circColor,
        std::vector<float>& circRot, std::vector<float>& circScale, std::vector<float>& circRadius,
        int& circSegments
    ) {
        rectPos.clear(); rectColor.clear(); rectRot.clear(); rectScale.clear(); rectW.clear(); rectH.clear();
        triPos.clear(); triColor.clear(); triRot.clear(); triScale.clear(); triSide.clear();
        circPos.clear(); circColor.clear(); circRot.clear(); circScale.clear(); circRadius.clear();

        rectScale.reserve(perTypeCount);
        rectPos.reserve(perTypeCount); rectColor.reserve(perTypeCount); rectRot.reserve(perTypeCount);
        rectW.reserve(perTypeCount); rectH.reserve(perTypeCount);
        triPos.reserve(perTypeCount); triColor.reserve(perTypeCount); triRot.reserve(perTypeCount);
        triScale.reserve(perTypeCount); triSide.reserve(perTypeCount);
        circPos.reserve(perTypeCount); circColor.reserve(perTypeCount); circRot.reserve(perTypeCount);
        circScale.reserve(perTypeCount); circRadius.reserve(perTypeCount);

        float worldW = 0.0f, worldH = 0.0f;
        ComputeWorldDimensions(worldW, worldH);

        std::random_device rd;
        std::mt19937 rng(rd());

        std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
        std::uniform_real_distribution<float> posY(-worldH * 0.5f, worldH * 0.5f);
        std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);
        std::uniform_real_distribution<float> widthDist(worldW * 0.01f, worldW * 0.05f);
        std::uniform_real_distribution<float> heightDist(worldH * 0.01f, worldH * 0.05f);
        std::uniform_real_distribution<float> triSideDist(worldW * 0.01f, worldW * 0.05f);
        std::uniform_int_distribution<int> colorDist(70, 255);
        std::uniform_real_distribution<float> circRadiusDist(worldW * 0.01f, worldW * 0.04f);

        // Rectangles via ECS + derived instancing arrays
        for (int i = 0; i < perTypeCount; ++i) {
            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);
            // float w = widthDist(rng);
            // float h = heightDist(rng);

            rectPos.emplace_back(pos);
            rectColor.emplace_back(
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f
            );
            rectRot.emplace_back(rotation);
            rectScale.emplace_back(1.0f);
            // rectW.emplace_back(w);
            // rectH.emplace_back(h);

            if (ecs) {
                auto e = ecs->CreateEntity();
                ecs->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Rect_" + std::to_string(e) });
                ecs->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
                    rotation, CEVec2{1.0f, 1.0f}, pos
                });

                Component::ShapeComponent shape{};
                shape.type = Component::ShapeType::Rectangle;
                shape.color = CEColor{
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng))
                };
                shape.enableTexture = texturesEnabled;
                shape.textureName = textureName;
                shape.uvSize = CEVec2{ 1.0f, 1.0f };

                ecs->AddComponent<Component::ShapeComponent>(e, shape);
            }
        }

        // Triangles via ECS + derived instancing arrays
        for (int i = 0; i < perTypeCount; ++i) {
            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);
            float side = triSideDist(rng);

            triPos.emplace_back(pos);
            triColor.emplace_back(
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f
            );
            triRot.emplace_back(rotation);
            triScale.emplace_back(1.0f);
            triSide.emplace_back(side);

            if (ecs) {
                auto e = ecs->CreateEntity();
                ecs->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Tri_" + std::to_string(e) });
                ecs->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
                    rotation, CEVec2{1.0f, 1.0f}, pos
                });

                Component::ShapeComponent shape{};
                shape.type = Component::ShapeType::Triangle;
                shape.color = CEColor{
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng)),
                    static_cast<float>(colorDist(rng))
                };
                shape.enableTexture = texturesEnabled;
                shape.textureName = textureName;
                shape.uvSize = CEVec2{ 1.0f, 1.0f };

                ecs->AddComponent<Component::ShapeComponent>(e, shape);
            }
        }

        // Circles are generated without ECS
        for (int i = 0; i < perTypeCount; ++i) {
            CEVec3 pos{ posX(rng), posY(rng), 0.0f };
            float rotation = rotDeg(rng);
            float radius = circRadiusDist(rng);

            circPos.emplace_back(pos);
            circColor.emplace_back(
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f,
                static_cast<float>(colorDist(rng)) / 255.0f
            );
            circRot.emplace_back(rotation);
            circScale.emplace_back(1.0f);
            circRadius.emplace_back(radius);
        }

        if (circSegments <= 0) circSegments = 24;
    }
} 

//............................Rectangles..........................//

static std::vector<CEVec3> gInstPositions;
static std::vector<CEColor> gInstColors;
static std::vector<float> gInstRotations;
static std::vector<float> gInstScales;
static std::vector<float> gInstWidths;
static std::vector<float> gInstHeights;

void InstancingSystem::ClearRectangleData() {
    gInstPositions.clear();
    gInstColors.clear();
    gInstRotations.clear();
    gInstScales.clear();
    gInstWidths.clear();
    gInstHeights.clear();
}

void InstancingSystem::InitializeEcsRectanglesDataRandom(int count) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;

    float worldW = 0.0f, worldHeight = 0.0f;
    InstancingSystem::ComputeWorldDimensions(worldW, worldHeight);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_real_distribution<float> posX(-worldW * 0.5f, worldW * 0.5f);
    std::uniform_real_distribution<float> posY(-worldHeight * 0.5f, worldHeight * 0.5f);
    std::uniform_real_distribution<float> rotDeg(0.0f, 360.0f);

    // Sizes scaled to world dimensions, similar to non-ECS rectangles
    std::uniform_real_distribution<float> widthDist(worldW * 0.01f, worldW * 0.05f);
    std::uniform_real_distribution<float> heightDist(worldHeight * 0.01f, worldHeight * 0.05f);

    // 0�255 range; renderer multiplies by 1/255.f later
    std::uniform_int_distribution<int> colorDist(70, 255);

    for (int i = 0; i < count; ++i) {
        auto e = globalECS->CreateEntity();

        CEVec3 pos{ posX(rng), posY(rng), 0.0f };
        float rotation = rotDeg(rng);
        // float w = widthDist(rng);
        // float h = heightDist(rng);

        // Name + Transform
        globalECS->AddComponent<Component::NameComponent>(e, Component::NameComponent{ "Rect_" + std::to_string(e) });
        globalECS->AddComponent<Component::TransformComponent>(e, Component::TransformComponent{
            rotation, CEVec2{1.0f, 1.0f}, pos
            });

        // Shape
        Component::ShapeComponent shape{};
        shape.type = Component::ShapeType::Rectangle;
        shape.color = CEColor{
            static_cast<float>(colorDist(rng)),
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

void InstancingSystem::InitializeInstancedRectanglesData(int count) {

    ClearRectangleData();

    gInstPositions.reserve(count);
    gInstColors.reserve(count);
    gInstRotations.reserve(count);
    gInstScales.reserve(count);
    gInstWidths.reserve(count);
    gInstHeights.reserve(count);

    float worldW, worldH;
    ComputeWorldDimensions(worldW, worldH);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(-worldW / 2.0f, worldW / 2.0f);
    std::uniform_real_distribution<float> distY(-worldH / 2.0f, worldH / 2.0f);
    std::uniform_real_distribution<float> distC(0.2f, 1.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
    std::uniform_real_distribution<float> distScale(0.5f, 2.0f);
    std::uniform_real_distribution<float> distWidth(1.0f, 4.0f);
    std::uniform_real_distribution<float> distHeight(1.0f, 4.0f);

    for (int i = 0; i < count; ++i) {
        gInstPositions.emplace_back(distX(gen), distY(gen), 0.0f);
        gInstColors.emplace_back(distC(gen), distC(gen), distC(gen));
        gInstRotations.emplace_back(distRot(gen));
        gInstScales.emplace_back(distScale(gen));
        gInstWidths.emplace_back(distWidth(gen));
        gInstHeights.emplace_back(distHeight(gen));
    }
    //gRectanglesInitialized = true;
}

void InstancingSystem::CreateInstancedRectangles(const CEVec3* positions, const CEColor* colors,
    const float* rotations, const float* scales,
    const float* widths, const float* heights,
    int count) {

    if (count <= 0) return;

    Component::Rendering& renderer = Component::Rendering::Get();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    // Initialize rectangle instancing buffers once
    if (!s_rectInstanceBuffersInitialized) {
        // Base vertices with positions and texture coordinates
        MeshVertex base_vertices[] = {
                { -0.5f, -0.5f, 0.0f,  0.0f, 1.0f }, // BL (UV 0,1)
                {  0.5f, -0.5f, 0.0f,  1.0f, 1.0f }, // BR (UV 1,1)
                {  0.5f,  0.5f, 0.0f,  1.0f, 0.0f }, // TR (UV 1,0)
                { -0.5f,  0.5f, 0.0f,  0.0f, 0.0f }  // TL (UV 0,0)
        };
        std::array<GLushort, 6> base_indices{ 0, 1, 2, 0, 2, 3 };

        s_rectInstanceVAO = renderer.GetPooledVAO();
        s_rectInstanceVBO = renderer.GetPooledVBO();
        s_rectInstanceEBO = renderer.GetPooledEBO();
        s_rectInstanceDataVBO = renderer.GetPooledVBO();

        glBindVertexArray(s_rectInstanceVAO);

        // 1. Base Geometry
        glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices, GL_STATIC_DRAW);

        // Loc 0: Pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, x));
        glEnableVertexAttribArray(0);
        // Loc 2: UV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, u));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_rectInstanceEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

        // 2. Instance Attributes (RectInstanceData)
        glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
        GLsizei stride = sizeof(RectInstanceData);

        // Loc 3: Position
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(RectInstanceData, position));
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);

        // Loc 4: Color (4 floats)
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(RectInstanceData, color));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        // Loc 5: Rotation
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(RectInstanceData, rotation));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        // Loc 6: Scale
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(RectInstanceData, scale));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        // Loc 7: Width
        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(RectInstanceData, width));
        glEnableVertexAttribArray(7);
        glVertexAttribDivisor(7, 1);

        // Loc 8: Height
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(RectInstanceData, height));
        glEnableVertexAttribArray(8);
        glVertexAttribDivisor(8, 1);

        s_rectInstanceBuffersInitialized = true;
        glBindVertexArray(0);
    }

    // --- Update Instance Data using Struct ---
    std::vector<RectInstanceData> instanceData(count);
    for (int i = 0; i < count; ++i) {
        instanceData[i].position = positions[i];
        instanceData[i].color = colors[i];
        instanceData[i].rotation = rotations[i];
        instanceData[i].scale = scales[i];
        instanceData[i].width = widths[i];
        instanceData[i].height = heights[i];
    }

    glBindVertexArray(s_rectInstanceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(RectInstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

    renderer.shdr_pgm_instanced->Use();
    renderer.shdr_pgm_instanced->SetUniform("uModel", glm::mat4(1.0f));

    GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_useTexture");
    if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);

    GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_tint");
    if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.0f);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, count);

    glBindVertexArray(0);

    // --- Cleanup ---
    //glDepthMask(GL_TRUE); // Re-enable depth writing for next objects
    glDisable(GL_BLEND);  // Disable blending
    glEnable(GL_CULL_FACE);
}

void InstancingSystem::CreateInstancedRectanglesTextured(
    const std::string& textureName,
    const CEVec3* positions, const CEColor* colors,
    const float* rotations, const float* scales,
    const float* widths, const float* heights,
    int count,
    const CEColor& tint,
    const CEVec2& uvSize)
{
    if (count <= 0) return;
    glDisable(GL_CULL_FACE);

    // Init buffers if needed using base function logic
    if (!s_rectInstanceBuffersInitialized) {
        CEVec3 p; CEColor c; float f = 0;
        CreateInstancedRectangles(&p, &c, &f, &f, &f, &f, 0);
    }

    std::vector<RectInstanceData> instanceData(count);
    for (int i = 0; i < count; ++i) {
        instanceData[i].position = positions[i];
        instanceData[i].color = colors[i];
        instanceData[i].rotation = rotations[i];
        instanceData[i].scale = scales[i];
        instanceData[i].width = widths[i];
        instanceData[i].height = heights[i];
    }

    glBindVertexArray(s_rectInstanceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_rectInstanceDataVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(RectInstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

    Component::Rendering& renderer = Component::Rendering::Get();
    GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureName);

    renderer.shdr_pgm_instanced->Use();
    renderer.shdr_pgm_instanced->SetUniform("uModel", glm::mat4(1.0f));

    GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_useTexture");
    if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

    GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_tint");
    if (tint_loc != -1) glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);

    GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_uvSize");
    if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

    //// 1. Handle Transparency
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //
    //// 2. Disable Depth Write so transparent textures don't occlusion-cull things behind them
    //glDepthMask(GL_FALSE);

    //glDisable(GL_BLEND);    // No blending
    glDepthMask(GL_TRUE);   // Write depth normally

    glBindTextureUnit(0, textureHandle);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, count);
    glBindTextureUnit(0, 0);

    glBindVertexArray(0);

    // 3. Reset State
    //glDepthMask(GL_TRUE); // Important!
    //glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void InstancingSystem::RenderInstancedRectanglesBatch(
    const CEVec3* positions,
    const CEColor* colors,
    const float* rotations,
    const float* scales,
    const float* widths,
    const float* heights,
    int count,
    const InstancedParams& params
) {
    if (count <= 0) return;
    if (!params.textured) {
        CreateInstancedRectangles(positions, colors, rotations, scales, widths, heights, count);
    } else {
        CreateInstancedRectanglesTextured(params.textureName, positions, colors, rotations, scales, widths, heights, count, params.tint, params.uvSize);
    }
}

void InstancingSystem::RenderInstancedRectangles(const InstancedParams& params) {
    int count = static_cast<int>(gInstPositions.size());
    if (count <= 0) return;

    InstancingSystem::RenderInstancedRectanglesBatch(
        gInstPositions.data(),
        gInstColors.data(),
        gInstRotations.data(),
        gInstScales.data(),
        gInstWidths.data(),
        gInstHeights.data(),
        count,
        params
    );
}

void InstancingSystem::PopulateRectanglesFromECS(bool previewOnly) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    InstancingSystem::ClearRectangleData();

    auto& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
    for (const auto& id : shapeArray.indexToEntity) {
        auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
        auto* tf = globalECS->GetComponent<Component::TransformComponent>(id);
        if (!sc || !tf) continue;
        if (sc->type != Component::ShapeType::Rectangle) continue;
        if (previewOnly && !sc->preview) continue;
        gInstPositions.emplace_back(tf->position);
        gInstColors.emplace_back(CEColor{ sc->color.r * (1.f/255.f), sc->color.g * (1.f/255.f), sc->color.b * (1.f/255.f) });
        gInstRotations.emplace_back(tf->rotation);
        gInstScales.emplace_back(1.f);
        gInstWidths.emplace_back(tf->size.x);
        gInstHeights.emplace_back(tf->size.y);
    }
}

void InstancingSystem::PopulateRectanglesFromEntities(const std::vector<CrashoutsEngine::Entity>& ids, bool previewOnly) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    InstancingSystem::ClearRectangleData();
    gInstPositions.reserve(ids.size());
    gInstColors.reserve(ids.size());
    gInstRotations.reserve(ids.size());
    gInstScales.reserve(ids.size());
    gInstWidths.reserve(ids.size());
    gInstHeights.reserve(ids.size());
    for (auto id : ids) {
        auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
        auto* tf = globalECS->GetComponent<Component::TransformComponent>(id);
        if (!sc || !tf) continue;
        if (sc->type != Component::ShapeType::Rectangle) continue;
        if (previewOnly && !sc->preview) continue;
        if (tf->size.x <= 0.f || tf->size.y <= 0.f) continue;
        gInstPositions.emplace_back(tf->position);
        gInstColors.emplace_back(CEColor{ sc->color.r * (1.f/255.f), sc->color.g * (1.f/255.f), sc->color.b * (1.f/255.f) });
        gInstRotations.emplace_back(tf->rotation);
        gInstScales.emplace_back(1.f);
        gInstWidths.emplace_back(tf->size.x);
        gInstHeights.emplace_back(tf->size.y);
    }
}

void InstancingSystem::RenderInstancedCirclesFromECS(bool previewOnly, int segments, const InstancedParams& params) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    std::vector<CEVec3> pos;
    std::vector<CEColor> col;
    std::vector<float> rot;
    std::vector<float> sca;
    std::vector<float> rad;
    auto& shapeArray = globalECS->GetComponentArray<Component::ShapeComponent>();
    for (const auto& id : shapeArray.indexToEntity) {
        auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
        auto* tf = globalECS->GetComponent<Component::TransformComponent>(id);
        if (!sc || !tf) continue;
        if (sc->type != Component::ShapeType::Circle) continue;
        if (previewOnly && !sc->preview) continue;
        pos.emplace_back(tf->position);
        col.emplace_back(sc->color);
        rot.emplace_back(tf->rotation);
        sca.emplace_back(1.f);
        rad.emplace_back(tf->size.x);
    }
    if (pos.empty()) return;
    InstancingSystem::RenderInstancedCirclesBatch(
        pos.data(),
        col.data(),
        rot.data(),
        sca.data(),
        rad.data(),
        static_cast<int>(pos.size()),
        segments <= 0 ? 24 : segments,
        params
    );
}

void InstancingSystem::RenderInstancedCirclesFromEntities(const std::vector<CrashoutsEngine::Entity>& ids, bool previewOnly, int segments, const InstancedParams& params) {
    EnsureGlobalECSInitialized();
    if (!globalECS) return;
    std::vector<CEVec3> pos;
    std::vector<CEColor> col;
    std::vector<float> rot;
    std::vector<float> sca;
    std::vector<float> rad;
    pos.reserve(ids.size());
    col.reserve(ids.size());
    rot.reserve(ids.size());
    sca.reserve(ids.size());
    rad.reserve(ids.size());
    for (auto id : ids) {
        auto* sc = globalECS->GetComponent<Component::ShapeComponent>(id);
        auto* tf = globalECS->GetComponent<Component::TransformComponent>(id);
        if (!sc || !tf) continue;
        if (sc->type != Component::ShapeType::Circle) continue;
        if (previewOnly && !sc->preview) continue;
        pos.emplace_back(tf->position);
        col.emplace_back(sc->color);
        rot.emplace_back(tf->rotation);
        sca.emplace_back(1.f);
        rad.emplace_back(tf->size.x);
    }
    if (pos.empty()) return;
    InstancingSystem::RenderInstancedCirclesBatch(
        pos.data(),
        col.data(),
        rot.data(),
        sca.data(),
        rad.data(),
        static_cast<int>(pos.size()),
        segments <= 0 ? 24 : segments,
        params
    );
}

void InstancingSystem::CreateInstancedTriangles(
    const CEVec3* positions, const CEColor* colors,
    const float* rotations, const float* scales,
    const float* sideLengths,
    int count
) {
    if (count <= 0) return;
    Component::Rendering& renderer = Component::Rendering::Get();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    if (!s_triInstanceBuffersInitialized) {
        MeshVertex base_vertices[] = {
                { -0.5f, -0.5f, 0.0f,  0.0f, 1.0f }, // BL
                {  0.5f, -0.5f, 0.0f,  1.0f, 1.0f }, // BR
                {  0.0f,  0.5f, 0.0f,  0.5f, 0.0f }  // Top
        };
        std::array<GLushort, 3> base_indices{ 0, 1, 2 };

        s_triInstanceVAO = renderer.GetPooledVAO();
        s_triInstanceVBO = renderer.GetPooledVBO();
        s_triInstanceEBO = renderer.GetPooledEBO();
        s_triInstanceDataVBO = renderer.GetPooledVBO();

        glBindVertexArray(s_triInstanceVAO);

        glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(base_vertices), base_vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, x));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, u));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_triInstanceEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(base_indices), base_indices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);
        GLsizei stride = sizeof(TriInstanceData);

        // Loc 3: Position
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TriInstanceData, position));
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);

        // Loc 4: Color
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TriInstanceData, color));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        // Loc 5: Rotation
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TriInstanceData, rotation));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        // Loc 6: Scale
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TriInstanceData, scale));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        // Loc 7: Side Length
        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TriInstanceData, sideLength));
        glEnableVertexAttribArray(7);
        glVertexAttribDivisor(7, 1);

        s_triInstanceBuffersInitialized = true;
        glBindVertexArray(0);
    }

    std::vector<TriInstanceData> instanceData(count);
    for (int i = 0; i < count; ++i) {
        instanceData[i].position = positions[i];
        instanceData[i].color = colors[i];
        instanceData[i].rotation = rotations[i];
        instanceData[i].scale = scales[i];
        instanceData[i].sideLength = sideLengths[i];
    }

    glBindVertexArray(s_triInstanceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(TriInstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

    renderer.shdr_pgm_instanced->Use();
    renderer.shdr_pgm_instanced->SetUniform("uModel", glm::mat4(1.0f));

    GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_useTexture");
    if (useTexture_loc != -1) glUniform1i(useTexture_loc, 0);

    GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_tint");
    if (tint_loc != -1) glUniform4f(tint_loc, 1.0f, 1.0f, 1.0f, 1.0f);

    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0, count);
    glBindVertexArray(0);

    //glDepthMask(GL_TRUE); // Re-enable depth writing for next objects
    glDisable(GL_BLEND);  // Disable blending
    glEnable(GL_CULL_FACE);
}

void InstancingSystem::CreateInstancedTrianglesTextured(
    const std::string& textureName,
    const CEVec3* positions, const CEColor* colors,
    const float* rotations, const float* scales,
    const float* sideLengths,
    int count,
    const CEColor& tint,
    const CEVec2& uvSize
) {
    if (count <= 0) return;
    glDisable(GL_CULL_FACE);

    if (!s_triInstanceBuffersInitialized) {
        CEVec3 p; CEColor c; float f = 0;
        CreateInstancedTriangles(&p, &c, &f, &f, &f, 0);
    }

    std::vector<TriInstanceData> instanceData(count);
    for (int i = 0; i < count; ++i) {
        instanceData[i].position = positions[i];
        instanceData[i].color = colors[i];
        instanceData[i].rotation = rotations[i];
        instanceData[i].scale = scales[i];
        instanceData[i].sideLength = sideLengths[i];
    }

    glBindVertexArray(s_triInstanceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_triInstanceDataVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(TriInstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

    Component::Rendering& renderer = Component::Rendering::Get();
    GLuint textureHandle = Component::TextureRenderer::GetTextureHandle(textureName);

    renderer.shdr_pgm_instanced->Use();
    renderer.shdr_pgm_instanced->SetUniform("uModel", glm::mat4(1.0f));

    GLint useTexture_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_useTexture");
    if (useTexture_loc != -1) glUniform1i(useTexture_loc, 1);

    GLint tint_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_tint");
    if (tint_loc != -1) glUniform4f(tint_loc, tint.r, tint.g, tint.b, tint.a);

    GLint uvSize_loc = glGetUniformLocation(renderer.shdr_pgm_instanced->GetHandle(), "u_uvSize");
    if (uvSize_loc != -1) glUniform2f(uvSize_loc, uvSize.x, uvSize.y);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glDepthMask(GL_FALSE);

    glDisable(GL_BLEND);    // No blending
    glDepthMask(GL_TRUE);   // Write depth normally

    glBindTextureUnit(0, textureHandle);
    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0, count);
    glBindTextureUnit(0, 0);

    glBindVertexArray(0);

    //glDepthMask(GL_TRUE); 
    //glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void InstancingSystem::RenderInstancedTrianglesBatch(
    const CEVec3* positions,
    const CEColor* colors,
    const float* rotations,
    const float* scales,
    const float* sideLengths,
    int count,
    const InstancingSystem::InstancedParams& params
) {
    if (count <= 0) return;
    if (!params.textured) {
        CreateInstancedTriangles(positions, colors, rotations, scales, sideLengths, count);
    }
    else {
        CreateInstancedTrianglesTextured(params.textureName, positions, colors, rotations, scales, sideLengths, count, params.tint, params.uvSize);
    }
}


// ... existing code ...
void InstancingSystem::RenderMode4Circles(const InstancedParams& params) {
    if (gM4CircPos.empty()) return;
    InstancingSystem::RenderInstancedCirclesBatch(
        gM4CircPos.data(), gM4CircColor.data(),
        gM4CircRot.data(), gM4CircScale.data(),
        gM4CircRadius.data(),
        static_cast<int>(gM4CircPos.size()),
        gM4CircSegments,
        params
    );
}

void InstancingSystem::RenderMode4Triangles(const InstancedParams& params) {
    if (gM4TriPos.empty()) return;
    if (!params.textured) {
        InstancingSystem::CreateInstancedTriangles(
            gM4TriPos.data(), gM4TriColor.data(),
            gM4TriRot.data(), gM4TriScale.data(),
            gM4TriSide.data(),
            static_cast<int>(gM4TriPos.size())
        );
    }
    else {
        InstancingSystem::CreateInstancedTrianglesTextured(
            params.textureName,
            gM4TriPos.data(), gM4TriColor.data(),
            gM4TriRot.data(), gM4TriScale.data(),
            gM4TriSide.data(),
            static_cast<int>(gM4TriPos.size()),
            params.tint, params.uvSize
        );
    }
}

void InstancingSystem::RenderMode4Rectangles(const InstancedParams& params) {
    if (gM4RectPos.empty()) return;
    std::vector<float> rectScales(gM4RectPos.size(), 1.0f);
    if (!params.textured) {
        InstancingSystem::CreateInstancedRectangles(
            gM4RectPos.data(), gM4RectColor.data(),
            gM4RectRot.data(), rectScales.data(),
            gM4RectW.data(), gM4RectH.data(),
            static_cast<int>(gM4RectPos.size())
        );
    }
    else {
        InstancingSystem::CreateInstancedRectanglesTextured(
            params.textureName,
            gM4RectPos.data(), gM4RectColor.data(),
            gM4RectRot.data(), rectScales.data(),
            gM4RectW.data(), gM4RectH.data(),
            static_cast<int>(gM4RectPos.size()),
            params.tint, params.uvSize
        );
    }
}

void InstancingSystem::ShutdownInstancing() {
    if (s_rectInstanceVAO) { glDeleteVertexArrays(1, &s_rectInstanceVAO); s_rectInstanceVAO = 0; }
    if (s_rectInstanceVBO) { glDeleteBuffers(1, &s_rectInstanceVBO); s_rectInstanceVBO = 0; }
    if (s_rectInstanceEBO) { glDeleteBuffers(1, &s_rectInstanceEBO); s_rectInstanceEBO = 0; }
    if (s_rectInstanceDataVBO) { glDeleteBuffers(1, &s_rectInstanceDataVBO); s_rectInstanceDataVBO = 0; }
    s_rectInstanceBuffersInitialized = false;

    if (s_circInstanceVAO) { glDeleteVertexArrays(1, &s_circInstanceVAO); s_circInstanceVAO = 0; }
    if (s_circInstanceVBO) { glDeleteBuffers(1, &s_circInstanceVBO); s_circInstanceVBO = 0; }
    if (s_circInstanceEBO) { glDeleteBuffers(1, &s_circInstanceEBO); s_circInstanceEBO = 0; }
    if (s_circInstanceDataVBO) { glDeleteBuffers(1, &s_circInstanceDataVBO); s_circInstanceDataVBO = 0; }
    s_circInstanceBuffersInitialized = false;
    s_circSegmentsCurrent = 0;

    if (s_triInstanceVAO) { glDeleteVertexArrays(1, &s_triInstanceVAO); s_triInstanceVAO = 0; }
    if (s_triInstanceVBO) { glDeleteBuffers(1, &s_triInstanceVBO); s_triInstanceVBO = 0; }
    if (s_triInstanceEBO) { glDeleteBuffers(1, &s_triInstanceEBO); s_triInstanceEBO = 0; }
    if (s_triInstanceDataVBO) { glDeleteBuffers(1, &s_triInstanceDataVBO); s_triInstanceDataVBO = 0; }
    s_triInstanceBuffersInitialized = false;

    gM4RectPos.clear(); gM4RectColor.clear(); gM4RectRot.clear(); gM4RectW.clear(); gM4RectH.clear();
    gM4TriPos.clear(); gM4TriColor.clear(); gM4TriRot.clear(); gM4TriScale.clear(); gM4TriSide.clear();
    gM4CircPos.clear(); gM4CircColor.clear(); gM4CircRot.clear(); gM4CircScale.clear(); gM4CircRadius.clear();
}



