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
#include "gamestatemanager.h"
#include "inputmanager.h"

#include "buttoncomponent.h"
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

	// Change Game State if button is pressed
	using namespace CrashoutsEngine;
	using namespace Component;
	Entity startBtnEntity = globalECS->FindEntityByName("Start_Button");
	ButtonComponent* startBtn = globalECS->GetComponent<ButtonComponent>(startBtnEntity);
	if (startBtn) {
		if (startBtn->buttonState == ButtonState::PRESSED) {
			startBtn->buttonState = ButtonState::NORMAL;
			ChangeGameState(GameState::DESCENT_GAME, false);
		}
	}
	// QUIT
	/*Entity quitBtnEntity = globalECS->FindEntityByName("Quit_Button");
	ButtonComponent* quitBtn = globalECS->GetComponent<ButtonComponent>(quitBtnEntity);
	if (quitBtn) {
		if (quitBtn->buttonState == ButtonState::PRESSED) {
			quitBtn->buttonState = ButtonState::NORMAL;
			glfwSetWindowShouldClose(GLApp::ptr_window, GLFW_TRUE);
		}
	}*/
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
	if (CurrentGameState != NextGameState) CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadDescentMenu()
{
	// Unload DescentMenu state
}