/*!
@file        renfred.cpp
@author      lee.r@digipen.edu
@date        18/09/2025
@brief       Implements the 

This file implements
*/

/*                                                   Includes
----------------------------------------------------------------------------- */
#include "scenes/renfred.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "managers/entitymanager.h"
#include "managers/assetmanager.h"


/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;

void LoadRenfred()
{
	// Load resources specific to Renfred state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	CrashoutsEngine::Assets::GenerateAssetManifest();

	CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();
}

void InitRenfred()
{
	// Initialize Renfred state
	LOGI(Core, "Renfred Initialized");
}

void UpdateRenfred()
{
	// Update logic for Renfred state
}

void RenderRenfred()
{
	// Render Renfred state
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	Scene::RenderScene(*globalECS);
}

void FreeRenfred()
{
	// Free resources specific to Renfred state
}

void UnloadRenfred()
{
	// Unload Renfred state
}