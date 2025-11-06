/*!
@file        descentgamescene.cpp
@author      lee.r@digipen.edu
@date        18/09/2025
@brief

This file
*/

/*                                                   Includes
----------------------------------------------------------------------------- */
#include "shaderasset.h"

#include "descentgamescene.h"

#include "sceneutils.h"
#include "log.h"
#include "GLFW/glfw3.h"
#include "entitymanager.h"
#include "assetmanager.h"
#include "sceneasset.h"
#include "monomanager.h"
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "descentgame";

void LoadDescentGame()
{
	using namespace CrashoutsEngine::Assets;
	sceneName = "descentgame";
	// Load resources specific to DescentGame state
	globalECS = CrashoutsEngine::ECS::getInstance();
	globalECS->Reset();

	//AssetManager::GetInstance().Get<ShaderAsset>("TextShader")->shaderObject.Use();
}

void InitDescentGame()
{
	using namespace CrashoutsEngine;
	// Initialize DescentGame state
	CrashoutsEngine::Serialization::LoadScenes(sceneName);
	Entity playerID = globalECS->FindEntityByName("Player");
	if (playerID >= 0) {
		globalECS->AddComponent<ScriptComponent>(playerID, ScriptComponent("PlayerController", playerID));
		ScriptComponent* s = globalECS->GetComponent<ScriptComponent>(playerID);
		s->Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
	}
	LOGI(Core, "Descent Menu Initialized");
}

void UpdateDescentGame()
{
	// Update logic for DescentGame state

}

void RenderDescentGame()
{
	// Render DescentGame state
#if USE_IMGUI_EDITOR
#else
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void FreeDescentGame()
{
	// Free resources specific to DescentGame state
	CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadDescentGame()
{
	// Unload DescentGame state
}