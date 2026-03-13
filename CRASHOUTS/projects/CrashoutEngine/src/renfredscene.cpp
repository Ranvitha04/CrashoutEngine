/*!
@file        renfred.cpp
@author      Renfred Lee
@author      Renfred (RTIS)
@author      Ranvitha
@date        18/09/2025
@brief       

This file 
*/

/*!
All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

/*                                                   Includes
----------------------------------------------------------------------------- */
#include "shaderasset.h"

#include "renfredscene.h"

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
static const char* sceneName = "renfred";

void LoadRenfred()
{
	using namespace CrashoutsEngine::Assets;
	sceneName = "renfred";
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	//AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();
}

void InitRenfred()
{
	// Initialize Renfred state
	CrashoutsEngine::Serialization::LoadScenes(sceneName);

	LOGI(Core, "Renfred Initialized");
}

void UpdateRenfred()
{
	// Update logic for Renfred state
}

void RenderRenfred()
{
	// Render Renfred state
#if USE_IMGUI_EDITOR
#else
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void FreeRenfred()
{
	// Free resources specific to Renfred state
	if (CurrentGameState != NextGameState) CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadRenfred()
{
	// Unload Renfred state
}