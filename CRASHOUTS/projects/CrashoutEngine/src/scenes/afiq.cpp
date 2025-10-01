#include "scenes/afiq.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"
#include "audioSystem.h"
using CrashoutsEngine::Audio::AudioSystem;

static bool s_audioReady = false;                     
static FMOD::Studio::EventInstance* s_menuMusic = nullptr;  


void LoadAfiq()
{
  
    if (!s_audioReady) {
        s_audioReady = AudioSystem::instance().init();  
        if (s_audioReady) {
         
            AudioSystem::instance().loadBank("sfx.bank");
        }
    }
}


void InitAfiq()
{
    LOGI(Core, "Afiq Initialized");

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
   
}
