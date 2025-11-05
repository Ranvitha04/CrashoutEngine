/*!
@file        descentmenuscene.cpp
@author      lee.r@digipen.edu
@date        18/09/2025
@brief

This file
*/

/*                                                   Includes
----------------------------------------------------------------------------- */
#include "shaderasset.h"

#include "descentmenuscene.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "sceneasset.h"
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "descentmenu";

void LoadDescentMenu()
{
	using namespace CrashoutsEngine::Assets;
	sceneName = "descentmenu";
	// Load resources specific to DescentMenu state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	//AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();
}

void InitDescentMenu()
{
	// Initialize DescentMenu state
	CrashoutsEngine::Serialization::LoadScenes(sceneName);

	LOGI(Core, "Descent Menu Initialized");
}

void UpdateDescentMenu()
{
	// Update logic for DescentMenu state
}

void RenderDescentMenu()
{
	// Render DescentMenu state
#if USE_IMGUI_EDITOR
#else
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void FreeDescentMenu()
{
	// Free resources specific to DescentMenu state
	CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadDescentMenu()
{
	// Unload DescentMenu state
}