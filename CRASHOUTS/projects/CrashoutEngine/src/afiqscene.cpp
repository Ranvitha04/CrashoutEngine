// afiq.cpp — scene bootstrap + audio + in-file prefab registry (no ImGui here)
#include "afiqscene.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"   // ECS + Component types
#include "prefab_init.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include "prefab.h"

// AUDIO
#include "audioSystem.h"
using CrashoutsEngine::Audio::AudioSystem;

using namespace CrashoutsEngine;
using namespace Component;

// ============================================================================
// In-file, minimal Prefab registry (name -> factory) so we don't need new files
// ============================================================================
  // anon ns (prefab)

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
    for (const auto& n : Prefab::Names()) {
        LOGI(Core, std::string("Registered prefab: ") + n);   // or build a std::string first
    }

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
