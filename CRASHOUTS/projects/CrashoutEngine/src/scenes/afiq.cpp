#include "scenes/afiq.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "sceneutils.h"
#include "managers/entitymanager.h"
#include "managers/assetmanager.h"
#include "resources/sceneasset.h"
#include "audioSystem.h"

using CrashoutsEngine::Audio::AudioSystem;

static bool s_audioReady = false;                     
static FMOD::Studio::EventInstance* s_menuMusic = nullptr;  

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "afiq";

// <CHANGE> tell AudioSystem where the .bank files live (CMake copies to "<exe>/assets")
void LoadAfiq()
{
    // Load resources specific to Renfred state
    globalECS = CrashoutsEngine::ECS::getInstance();
    globalECS->Reset();

    CrashoutsEngine::Assets::GenerateAssetManifest();

    CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();

    if (!s_audioReady) {
        s_audioReady = AudioSystem::instance().init("assets");  // <� was init()
        if (s_audioReady) {
            AudioSystem::instance().loadBank("sfx.bank");
            AudioSystem::instance().loadBank("Music.bank");      // <ADD> you start Menu music later
        }
    }
}



void InitAfiq()
{

    if (!s_audioReady) return;

    AudioSystem::instance().playOneShot("event:/sfexbeep");

  
    s_menuMusic = AudioSystem::instance().createInstance("event:/Music/Menu");
    if (s_menuMusic) {
        s_menuMusic->start();
       
    }

    CrashoutsEngine::Serialization::LoadScenes(sceneName);

    LOGI(Core, "Afiq Initialized");
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

    Scene::RenderScene(*globalECS);
}

void FreeAfiq()
{
    if (s_menuMusic) {
        s_menuMusic->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        s_menuMusic->release();
        s_menuMusic = nullptr;
    }

    CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadAfiq()
{
   
}
