// afiq.cpp — scene bootstrap + audio + in-file prefab registry (no ImGui here)
#include "afiqscene.h"
#include "log.h"

#include "entitymanager.h"  
#include "prefab_init.h"
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
static bool gUseSwitchStyle = false;
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

    
    static bool gPrefabsInited = false;
    static void InitPrefabsOnce() {
        if (gPrefabsInited) return;
        gPrefabsInited = true;

      
        PrefabSetFallback([](ECS& e, const std::string& name) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ name });
            if (!e.HasComponent<TransformComponent>(ent)) {
                e.AddComponent<TransformComponent>(ent, TransformComponent{});
            }
            });

      
        PrefabRegister("Platform", [](ECS& e) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ "Platform" });
            e.AddComponent<TransformComponent>(ent, TransformComponent{ .size{100.f, 10.f} });
            e.AddComponent<ShapeComponent>(ent, ShapeComponent{ .type = ShapeType::Rectangle });
          
            });

        
        PrefabRegister("Box", [](ECS& e) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ "Box" });
            e.AddComponent<TransformComponent>(ent, TransformComponent{ .size{20.f, 20.f} });
            e.AddComponent<ShapeComponent>(ent, ShapeComponent{ .type = ShapeType::Rectangle });
            });
    
        PrefabRegister("KF_Box", [](ECS& e) {
            auto ent = e.CreateEntity();
            gKFBox = ent; 

            
            e.AddComponent<NameComponent>(ent, NameComponent{ "KF_Box" });

            TransformComponent t{};
            t.position = { 0.f, 0.f, 0.f };
            t.size = { 40.f, 40.f };
            e.AddComponent<TransformComponent>(ent, t);

            ShapeComponent s{};
            s.type = ShapeType::Rectangle;
            s.color = CEColor{ 0.9f, 0.4f, 0.2f, 1.f };
            e.AddComponent<ShapeComponent>(ent, s);
            
            {
                CEVec2 worldSize{ 40.0f, 40.0f };  
            
                Component::SetSpriteSheet(
                    ent,
                    "images/spritesheet/Chibi-Actions-SpriteSheet.png", 
                    3, 
                    3,  
                    worldSize
                );

                
                auto* animSprite = e.GetComponent<Component::SpriteAnimComponent>(ent);
                if (!animSprite) {
                    Component::SpriteAnimComponent newAnim{};
                    newAnim.startFrame = 0;
                    newAnim.frameCount = 1;
                    newAnim.frameTime = 0.1f;
                    newAnim.frameIndex = 0;
                    newAnim.timer = 0.0f;
                    newAnim.flip = false;
                    e.AddComponent<Component::SpriteAnimComponent>(ent, newAnim);
                }
            }

            
            auto& anim = KeyframeUnity::AddAnimator(e, ent);

            AnimationClip walk{};
            walk.name = "Walk";
            walk.wrap = KFWrap::Loop;

            
            KFBinding walkSprite{};
            walkSprite.property = KFProperty::SpriteIndex;

            
            {
               
                KFKey k0{};
                k0.time = 0.0f;
                k0.value = 3.0f;   
                k0.inTan = 0.0f;
                k0.outTan = 0.0f;
                k0.interp = KFInterp::Linear;
                walkSprite.curve.Add(k0);

              
                KFKey k1{};
                k1.time = 0.12f;
                k1.value = 4.0f;   
                k1.inTan = 0.0f;
                k1.outTan = 0.0f;
                k1.interp = KFInterp::Linear;
                walkSprite.curve.Add(k1);

               
                KFKey k2{};
                k2.time = 0.24f;
                k2.value = 5.0f;   
                k2.inTan = 0.0f;
                k2.outTan = 0.0f;
                k2.interp = KFInterp::Linear;
                walkSprite.curve.Add(k2);

                
                KFKey k3{};
                k3.time = 0.36f;
                k3.value = 4.0f;   
                k3.inTan = 0.0f;
                k3.outTan = 0.0f;
                k3.interp = KFInterp::Linear;
                walkSprite.curve.Add(k3);
            }

          
            walk.bindings.push_back(walkSprite);
         

           
            KFBinding scaleX;
            scaleX.property = KFProperty::ScaleX;

            KFKey sx0{};
            sx0.time = 0.0f;
            sx0.value = 1.0f; 
            sx0.inTan = 0.0f;
            sx0.outTan = 0.0f;
            sx0.interp = KFInterp::Linear;
            scaleX.curve.Add(sx0);

            walk.bindings.push_back(scaleX);

            
            KFBinding scaleY;
            scaleY.property = KFProperty::ScaleY;

            KFKey sy0{};
            sy0.time = 0.0f;
            sy0.value = 1.0f;  
            sy0.inTan = 0.0f;
            sy0.outTan = 0.0f;
            sy0.interp = KFInterp::Linear;
            scaleY.curve.Add(sy0);

            walk.bindings.push_back(scaleY);

            KeyframeUnity::SetClip(anim, walk);

            KeyframeUnity::SetClip(anim, walk);

            
            AnimationClip jump{};
            jump.name = "Jump";
            jump.wrap = KFWrap::Loop;

            KFBinding jumpSprite{};
            jumpSprite.property = KFProperty::SpriteIndex;

          
            {
                KFKey j0{};
                j0.time = 0.0f;
                j0.value = 0.0f;   
                j0.inTan = 0.0f;
                j0.outTan = 0.0f;
                j0.interp = KFInterp::Linear;
                jumpSprite.curve.Add(j0);

                KFKey j1{};
                j1.time = 0.10f;
                j1.value = 1.0f;  
                j1.inTan = 0.0f;
                j1.outTan = 0.0f;
                j1.interp = KFInterp::Linear;
                jumpSprite.curve.Add(j1);

                KFKey j2{};
                j2.time = 0.20f;
                j2.value = 2.0f;   
                j2.inTan = 0.0f;
                j2.outTan = 0.0f;
                j2.interp = KFInterp::Linear;
                jumpSprite.curve.Add(j2);
            }

            jump.bindings.push_back(jumpSprite);
            KeyframeUnity::SetClip(anim, jump);

           
            KeyframeUnity::Play(anim, "Walk", true);


            
            KeyframeUnity::Play(anim, "Walk", true);
            });

    }
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
    InitPrefabs();
    // One-time prefab registration
    InitPrefabsOnce();

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

    
    ECS* ecs = ECS::getInstance();
    ecs->Reset();

    PrefabCreate(*ecs, "KF_Box");

    if (!s_audioReady) return;

    
    AudioSystem::instance().playOneShot("event:/sfexbeep");

   
    s_menuMusic = AudioSystem::instance().createInstance("event:/Music/Menu");
    if (s_menuMusic) {
        s_menuMusic->start();
    }
}


void UpdateAfiq()
{
    if (s_audioReady) {
        AudioSystem::instance().update();
    }

    ECS* ecs = ECS::getInstance();
    if (ecs) {
        float dt = static_cast<float>(FrameRateController::GetInstance().DT());
        KeyframeUnity::UpdateAll(*ecs, dt);
        Component::UpdateSpriteAnimationsECS();

    
      
        if (gKFBox && ecs->HasComponent<KeyframeAnimatorComponent>(gKFBox)) {
            auto* anim = ecs->GetComponent<KeyframeAnimatorComponent>(gKFBox);

            
            if (InputManager::IsKeyTriggered(Key::Z)) {
                gUseSwitchStyle = !gUseSwitchStyle;

                CEVec2 worldSize{ 40.0f, 40.0f }; 

                if (gUseSwitchStyle) {
                    
                    Component::SetSpriteSheet(
                        gKFBox,
                        "images/Character/New Artstyle/Switch-Style-SpriteSheet.png",
                        3, 3,
                        worldSize
                    );
                }
                else {
                    
                    Component::SetSpriteSheet(
                        gKFBox,
                        "images/spritesheet/Chibi-Actions-SpriteSheet.png",
                        3, 3,
                        worldSize
                    );
                }

                
                KeyframeUnity::Play(*anim, "Walk", true);
            }

           
            if (InputManager::IsKeyTriggered(Key::Y)) {
                KeyframeUnity::Play(*anim, "Jump", true);
            }
        }

    }
}


void RenderAfiq()
{
#if !USE_IMGUI_EDITOR
    
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    
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
