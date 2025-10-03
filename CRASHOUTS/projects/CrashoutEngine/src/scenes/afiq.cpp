// afiq.cpp — scene bootstrap + audio + in-file prefab registry (no ImGui here)
#include "scenes/afiq.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"   // ECS + Component types
#include "prefab_init.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

// AUDIO
#include "audioSystem.h"
using CrashoutsEngine::Audio::AudioSystem;

using namespace CrashoutsEngine;
using namespace Component;

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
    static bool gPrefabsInited = false;
    static void InitPrefabsOnce() {
        if (gPrefabsInited) return;
        gPrefabsInited = true;

        // Fallback: create a plain named entity with a Transform
        PrefabSetFallback([](ECS& e, const std::string& name) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ name });
            if (!e.HasComponent<TransformComponent>(ent)) {
                e.AddComponent<TransformComponent>(ent, TransformComponent{});
            }
            });

        // Example prefabs you can extend any time:

        // Platform: wide rectangle
        PrefabRegister("Platform", [](ECS& e) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ "Platform" });
            e.AddComponent<TransformComponent>(ent, TransformComponent{});
            e.AddComponent<ShapeComponent>(ent, ShapeComponent{
                .type = ShapeType::Rectangle, .width = 100.f, .height = 10.f
                });
            // add collider/rigidbody/etc. here if desired
            });

        // Box: small rectangle
        PrefabRegister("Box", [](ECS& e) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ "Box" });
            e.AddComponent<TransformComponent>(ent, TransformComponent{});
            e.AddComponent<ShapeComponent>(ent, ShapeComponent{
                .type = ShapeType::Rectangle, .width = 20.f, .height = 20.f
                });
            });

        // Add more:
        // PrefabRegister("Ball", [](ECS& e){ /* components... */ });
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
        s_audioReady = AudioSystem::instance().init("assets"); // looks in <exe>/assets
        if (s_audioReady) {
            AudioSystem::instance().loadBank("sfx.bank");
            AudioSystem::instance().loadBank("Music.bank");
        }
    }
}

void InitAfiq()
{
    LOGI(Core, "Afiq Initialized");

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
        AudioSystem::instance().update();
    }
}

void RenderAfiq()
{
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // NOTE: do NOT draw ImGui windows here.
    // If you want a Prefab Creator UI, put it inside an existing debug window (e.g., ECS panel).
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
