// afiq.cpp — scene bootstrap + audio + in-file prefab registry (no ImGui here)

/*!
@file        afiqscene.cpp
@author      b.mohamadafiq@digipen.edu
@date        30/11/2025
@brief       scene functions for Afiq scene

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

#include "afiqscene.h"
#include "log.h"

#include "entitymanager.h"  

#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include "frameratecontroller.h"
#include "keyframe.h"
#include "spritecomponent.h"
#include "spriteAnimation.h"

#include "transformcomponent.h"
#include "namecomponent.h"
#include "shapescomponent.h"
#include "inputmanager.h" 
// AUDIO
#include "audioSystem.h"
using CrashoutsEngine::Audio::AudioSystem;

using namespace CrashoutsEngine;
using namespace Component;
static Entity gKFBox = 0;
// ============================================================================
// In-file, minimal Prefab registry (name -> factory) so we don't need new files
// ============================================================================
namespace {
    using Factory = std::function<void(ECS&)>;
    using Fallback = std::function<void(ECS&, const std::string&)>;

    static std::unordered_map<std::string, Factory> gPrefabReg;
    static Fallback gPrefabFallback;

    inline bool PrefabRegister(const std::string& name, Factory f) {
        return gPrefabReg.emplace(name, std::move(f)).second;
    }
    inline void PrefabSetFallback(Fallback fb) { gPrefabFallback = std::move(fb); }

    inline bool PrefabCreate(ECS& ecs, const std::string& name) {
        if (auto it = gPrefabReg.find(name); it != gPrefabReg.end()) { it->second(ecs); return true; }
        if (gPrefabFallback) { gPrefabFallback(ecs, name); return true; }
        return false;
    }

    // One-time registration guard
    //static bool gPrefabsInited = false;
    //static void InitPrefabsOnce() {
    //    if (gPrefabsInited) return;
    //    gPrefabsInited = true;

    //    // Fallback: create a plain named entity with a Transform
    //    PrefabSetFallback([](ECS& e, const std::string& name) {
    //        auto ent = e.CreateEntity();
    //        e.AddComponent<NameComponent>(ent, NameComponent{ name });
    //        if (!e.HasComponent<TransformComponent>(ent)) {
    //            e.AddComponent<TransformComponent>(ent, TransformComponent{});
    //        }
    //        });

    //    // Example prefabs you can extend any time:

    //    // Platform: wide rectangle
    //    PrefabRegister("Platform", [](ECS& e) {
    //        auto ent = e.CreateEntity();
    //        e.AddComponent<NameComponent>(ent, NameComponent{ "Platform" });
    //        e.AddComponent<TransformComponent>(ent, TransformComponent{ .size{100.f, 10.f} });
    //        e.AddComponent<ShapeComponent>(ent, ShapeComponent{ .type = ShapeType::Rectangle });
    //      
    //        });

    //    // Box: small rectangle
    //    PrefabRegister("Box", [](ECS& e) {
    //        auto ent = e.CreateEntity();
    //        e.AddComponent<NameComponent>(ent, NameComponent{ "Box" });
    //        e.AddComponent<TransformComponent>(ent, TransformComponent{ .size{20.f, 20.f} });
    //        e.AddComponent<ShapeComponent>(ent, ShapeComponent{ .type = ShapeType::Rectangle });
    //        });
    //
    //    PrefabRegister("KF_Box", [](ECS& e) {
    //        auto ent = e.CreateEntity();
    //        gKFBox = ent;  // remember which entity is our animated test box

    //        // Basic components
    //        e.AddComponent<NameComponent>(ent, NameComponent{ "KF_Box" });

    //        TransformComponent t{};
    //        t.position = { 0.f, 0.f, 0.f };
    //        t.size = { 40.f, 40.f };
    //        e.AddComponent<TransformComponent>(ent, t);

    //        ShapeComponent s{};
    //        s.type = ShapeType::Rectangle;
    //        s.color = CEColor{ 0.9f, 0.4f, 0.2f, 1.f };
    //        e.AddComponent<ShapeComponent>(ent, s);

    //        // Attach animator component
    //        auto& anim = KeyframeUnity::AddAnimator(e, ent);

    //        // -------------------------------------------------
    //        // CLIP 1: "Walk"  (small, slower sway)
    //        // -------------------------------------------------
    //        AnimationClip walk{};
    //        walk.name = "Walk";
    //        walk.wrap = KFWrap::Loop;

    //        KFBinding walkRot{};
    //        walkRot.property = KFProperty::RotZ;

    //        KFKey walk0{};
    //        walk0.time = 0.f;
    //        walk0.value = -30.f;           // degrees
    //        walk0.inTan = 0.f;
    //        walk0.outTan = 0.f;
    //        walk0.interp = KFInterp::Linear;

    //        KFKey walk1{};
    //        walk1.time = 0.5f;
    //        walk1.value = 30.f;            // degrees
    //        walk1.inTan = 0.f;
    //        walk1.outTan = 0.f;
    //        walk1.interp = KFInterp::Linear;

    //        walkRot.curve.Add(walk0);
    //        walkRot.curve.Add(walk1);
    //        walk.bindings.push_back(walkRot);

    //        KeyframeUnity::SetClip(anim, walk);

    //        // -------------------------------------------------
    //        // CLIP 2: "Jump" (bigger, faster swing)
    //        // -------------------------------------------------
    //        AnimationClip jump{};
    //        jump.name = "Jump";
    //        jump.wrap = KFWrap::Loop;

    //        KFBinding jumpRot{};
    //        jumpRot.property = KFProperty::RotZ;

    //        KFKey jump0{};
    //        jump0.time = 0.f;
    //        jump0.value = -90.f;
    //        jump0.inTan = 0.f;
    //        jump0.outTan = 0.f;
    //        jump0.interp = KFInterp::Linear;

    //        KFKey jump1{};
    //        jump1.time = 0.25f;
    //        jump1.value = 90.f;
    //        jump1.inTan = 0.f;
    //        jump1.outTan = 0.f;
    //        jump1.interp = KFInterp::Linear;

    //        jumpRot.curve.Add(jump0);
    //        jumpRot.curve.Add(jump1);
    //        jump.bindings.push_back(jumpRot);

    //        KeyframeUnity::SetClip(anim, jump);

    //        // Start in "Walk" by default
    //        KeyframeUnity::Play(anim, "Walk", true);
    //        });

    //}
} // anon ns (prefab)

// ============================================================================
// Afiq scene audio state
// ============================================================================
static bool s_audioReady = false;
static FMOD::Studio::EventInstance* s_menuMusic = nullptr;

// ============================================================================
// Scene API
// ============================================================================
void LoadAfiq()
{
    //InitPrefabs();
    //// One-time prefab registration
    //InitPrefabsOnce();

    // Audio init + banks
    if (!s_audioReady) {
        s_audioReady = AudioSystem::instance().init("assets"); 
        if (s_audioReady) {
            AudioSystem::instance().loadBank("sfx.bank");
            AudioSystem::instance().loadBank("Music.bank");
        }
    }
}

void InitAfiq()
{
    LOGI(Core, "Afiq Initialized");

    // Get ECS pointer
    ECS* ecs = ECS::getInstance();
    ecs->Reset();

    PrefabCreate(*ecs, "KF_Box");

    if (!s_audioReady) return;

    // quick beep to prove audio works
    AudioSystem::instance().playOneShot("event:/sfexbeep");

    // start menu music
    s_menuMusic = AudioSystem::instance().createInstance("event:/Music/Menu");
    if (s_menuMusic) {
        s_menuMusic->start();
    }
}


void UpdateAfiq()
{
    if (s_audioReady) {
        AudioSystem::instance().Update();
    }

    ECS* ecs = ECS::getInstance();
    if (ecs) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        KeyframeUnity::UpdateAll(*ecs, dt);
        Component::UpdateSpriteAnimationsECS();

        // -------------------------------------------------
        // Keyboard: Z = Walk, Y = Jump (two clips on same
        // character, like your rubric wants).
        // -------------------------------------------------
        if (gKFBox && ecs->HasComponent<KeyframeAnimatorComponent>(gKFBox)) {
            auto* anim = ecs->GetComponent<KeyframeAnimatorComponent>(gKFBox);

            // Play "Walk" clip
            if (InputManager::IsKeyTriggered(Key::Z)) {
                KeyframeUnity::Play(*anim, "Walk", true);
            }

            // Play "Jump" clip
            if (InputManager::IsKeyTriggered(Key::Y)) {
                KeyframeUnity::Play(*anim, "Jump", true);
            }
        }
    }
}


void RenderAfiq()
{
#if !USE_IMGUI_EDITOR
    // Non-editor build: clear screen, then draw
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    // Draw all entities that have SpriteSheetComponent + SpriteAnimComponent
    Component::RenderSpriteAnimationsECS();
}

void FreeAfiq()
{
    if (s_menuMusic) {
        s_menuMusic->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        s_menuMusic->release();
        s_menuMusic = nullptr;
    }
}

void UnloadAfiq()
{
    // add any scene unload if needed
}
