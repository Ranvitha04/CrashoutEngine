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
#include "engine.h"
/*                                                   Global Variables
----------------------------------------------------------------------------- */

static CrashoutsEngine::ECS* globalECS;
static const char* sceneName = "descentgame";

static bool isLight{ false };
static float energyBar{ 100.f };
static float energyBarLen;

#define NORMAL_MASS		5.0f
#define LIGHT_MASS		2.5f
#define ENERGY_USAGE	10.f

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

	Entity energyEntity = globalECS->FindEntityByName("Energy_Bar_Inner");
	TransformComponent* energyTransform = globalECS->GetComponent<TransformComponent>(energyEntity);
	if (energyTransform) {
		energyBarLen = energyTransform->size.x;
	}

	LOGI(Core, "Descent Menu Initialized");
}

void UpdateDescentGame()
{
	using namespace CrashoutsEngine;
	// Update logic for DescentGame state
	// Player W,A,S,D, SPACE
	Entity playerEntity = globalECS->FindEntityByName("Player");
	TransformComponent* playerTransform = globalECS->GetComponent<TransformComponent>(playerEntity);
	if (playerTransform && engineState == EngineState::PLAY) {
		if (InputManager::IsKeyTriggered(Key::A)) {
			if (playerTransform->size.x > 0) playerTransform->size.x *= -1.f;
		}
		if (InputManager::IsKeyTriggered(Key::D)) {
			if (playerTransform->size.x < 0) playerTransform->size.x *= -1.f;
		}
		if (InputManager::IsKeyTriggered(Key::E)) {
			if(energyBar > 0) energyBar -= ENERGY_USAGE;
			// Update energy bar
			Entity energyEntity = globalECS->FindEntityByName("Energy_Bar_Inner");
			TransformComponent* energyTransform = globalECS->GetComponent<TransformComponent>(energyEntity);
			if(energyTransform) energyTransform->size.x = energyBarLen * (energyBar * 0.01f);

			if (energyBar > 0) {
				isLight = !isLight;
				ShapeComponent* s = globalECS->GetComponent<ShapeComponent>(playerEntity);
				if (s) {
					if (isLight) {
						s->textureName = "Character/New Artstyle/Chibi-7.png";
					}
					else {
						s->textureName = "Character/New Artstyle/Chibi-1.png";
					}
				}
				RigidBodyComponent* r = globalECS->GetComponent<RigidBodyComponent>(playerEntity);
				if (r) {
					if (isLight)	r->setMass((CE::f32)LIGHT_MASS);
					if (!isLight)	r->setMass((CE::f32)NORMAL_MASS);
				}
			}
		}
	}
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
	if(CurrentGameState != NextGameState) CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadDescentGame()
{
	// Unload DescentGame state
}