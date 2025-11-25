/*!
@file        scenebindings.cpp
@author      lee.r@digipen.edu
@date        23/11/2025
@brief       Definition of scene bindings functions
*/

#include "scenebindings.h"
#include "sceneasset.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "glapp.h"

// Mono headers
#include "mono/metadata/loader.h"
#include "mono/metadata/appdomain.h"
#include "mono/metadata/object.h"
#include <mono/metadata/assembly.h>

using namespace CrashoutsEngine;

namespace
{
    // Load scene from name
    static void InternalLoadScene(MonoString* sceneName) {
        char* utf8 = mono_string_to_utf8(sceneName);

        std::string nameString{ utf8 };
        if (nameString.empty()) return;
        if (nameString == Serialization::currSceneName) return;

        Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();
        if (!assetManager.Exists(nameString)) return;

        ECS::getInstance()->Reset();
        Serialization::currSceneName = nameString;
        Serialization::LoadScenes(nameString);

        mono_free(utf8);
    }

    static void InternalQuitGame() {
        glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
    }
}

void CrashoutsEngine::Bindings::RegisterSceneBindings()
{
    mono_add_internal_call("CrashoutsEngine.SceneManager::LoadScene", (const void*)InternalLoadScene);
    mono_add_internal_call("CrashoutsEngine.SceneManager::QuitGame", (const void*)InternalQuitGame);
}
