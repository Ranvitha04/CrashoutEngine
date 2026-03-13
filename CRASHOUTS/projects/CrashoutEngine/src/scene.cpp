/*!
@file        scene.cpp
@author      Renfred Lee
@author      Renfred (RTIS)
@author      Shahrin Idzuan
@date        12/11/2025
@brief       

This file is the base template which all the future serialized scenes would 
build off from.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

/*                                                   Includes
----------------------------------------------------------------------------- */
#include "shaderasset.h"

#include "scene.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "sceneasset.h"
#include "gamestatemanager.h"
#include "scenetransition.h"
#include "frameratecontroller.h"
#include "engine.h"
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;

void LoadScene()
{
    using namespace CrashoutsEngine::Assets;
    // Load resources specific to Scene state
    globalECS = CrashoutsEngine::ECS::getInstance();
    globalECS->Reset();
}

void InitScene()
{
    // 1. Figure out what game we are playing and what its first scene is
    // (This now calls the global function from engine.h!)
    std::string startScene = GetStartingSceneName();

    // 2. Update the Engine's current scene tracker
    CrashoutsEngine::Serialization::currSceneName = startScene;

    // 3. Load it dynamically!
    CrashoutsEngine::Serialization::LoadScenes(startScene);

    LOGI(Core, "Scene Initialized: " + startScene);
}

void UpdateScene()
{
    // Update logic for Scene state
    float dt = (float)FrameRateController::GetInstance().DT();
    CrashoutsEngine::SceneTransition::Update(dt);
}

void RenderScene()
{
	// Render Scene state
#if USE_IMGUI_EDITOR
#else
	//glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	CrashoutsEngine::SceneTransition::Render();
}

void FreeScene()
{
	// Free resources specific to Scene state
	//if (CurrentGameState != NextGameState) CrashoutsEngine::Serialization::SaveScenes(sceneName);
	globalECS->Reset();
}

void UnloadScene()
{
	// Unload Scene state
}