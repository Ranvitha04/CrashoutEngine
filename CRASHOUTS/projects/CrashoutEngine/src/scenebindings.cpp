/*!
@file        scenebindings.cpp
@author      Renfred Lee
@author      Mohamad Amir Akmal
@author      Shahrin Idzuan
@date        23/11/2025
@brief       Definition of scene bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "scenebindings.h"
#include "sceneasset.h"
//#include "entitymanager.h"
#include "assetmanager.h"
#include "glapp.h"
#include "engine.h"
#include "audioSystem.h"
#include "scenetransition.h"

// Mono headers
#include "mono/metadata/loader.h"
#include "mono/metadata/appdomain.h"
#include "mono/metadata/object.h"
//#include <mono/metadata/assembly.h>

using namespace CrashoutsEngine;

namespace
{
    // Load scene from name
    static void InternalLoadScene(MonoString* sceneName) {
        char* utf8 = mono_string_to_utf8(sceneName);

        std::string nameString{ utf8 };
        mono_free(utf8);
        if (nameString.empty()) return;
        //if (nameString == Serialization::currSceneName) return;

        Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();
        if (!assetManager.Exists(nameString)) return;

        g_PendingSceneName = nameString;
        g_IsSceneLoadPending = true;

        std::cout << "[Engine] Scene Load Scheduled: " << nameString << std::endl;
    }

    static void InternalFadeEffect(float fadeOut, float fadeIn) {
        CrashoutsEngine::SceneTransition::StartFade(fadeOut, fadeIn);
    }

    // Returns TRUE if the screen is currently fading (transition not over)
    static bool InternalIsTransitioning() {
        return CrashoutsEngine::SceneTransition::IsTransitioning();
    }

    static MonoString* InternalGetActiveSceneName() {
        return mono_string_new(mono_domain_get(), Serialization::currSceneName.c_str());
    }

    static void InternalQuitGame() {
        glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
    }

    static void InternalTransitionToScene(MonoString* sceneName, float fadeOut, float fadeIn) {
        char* utf8 = mono_string_to_utf8(sceneName);
        std::string nameString{ utf8 };
        mono_free(utf8);
        if (nameString.empty()) return;

        std::cout << "[Engine] Transitioning to: " << nameString << " (Out: " << fadeOut << "s, In: " << fadeIn << "s)" << std::endl;

        CrashoutsEngine::SceneTransition::Start(nameString, fadeOut, fadeIn);
    }
}

void CrashoutsEngine::Bindings::RegisterSceneBindings()
{
    mono_add_internal_call("CrashoutsEngine.SceneManager::InternalLoadScene", (const void*)InternalLoadScene);
    mono_add_internal_call("CrashoutsEngine.SceneManager::QuitGame", (const void*)InternalQuitGame);
    mono_add_internal_call("CrashoutsEngine.SceneManager::TransitionToScene", (const void*)InternalTransitionToScene);
    mono_add_internal_call("CrashoutsEngine.SceneManager::FadeEffect", (const void*)InternalFadeEffect);
    mono_add_internal_call("CrashoutsEngine.SceneManager::IsTransitioning", (const void*)InternalIsTransitioning);
    mono_add_internal_call("CrashoutsEngine.SceneManager::GetActiveSceneName", (const void*)InternalGetActiveSceneName);
}

