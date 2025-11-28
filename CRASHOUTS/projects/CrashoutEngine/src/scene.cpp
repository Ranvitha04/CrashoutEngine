/*!
@file        renfred.cpp
@author      lee.r@digipen.edu
@date        12/11/2025
@brief       

This file is the base template which all the future serialized scenes would 
build off from.
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
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;
//static const char* sceneName = "";

void LoadScene()
{
	using namespace CrashoutsEngine::Assets;
	//sceneName = "";
	// Load resources specific to Scene state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

}

void InitScene()
{
	// Initialize Scene state
	CrashoutsEngine::Serialization::LoadScenes("descentmenu");

	LOGI(Core, "Scene Initialized");
}

void UpdateScene()
{
	// Update logic for Scene state
}

void RenderScene()
{
	// Render Scene state
#if USE_IMGUI_EDITOR
#else
	//glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
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