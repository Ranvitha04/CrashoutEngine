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
static float energyBarOriginX;

// Goal counter
static int goalsDone{ 0 };
static bool pp1Done, pp2Done;

#define NORMAL_MASS		7.0f
#define LIGHT_MASS		3.5f
#define ENERGY_USAGE	10.f
#define REQUIRED_GOALS	2
#define Y_TOLERANCE		-8.f

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
		energyBarOriginX = energyTransform->position.x;
	}

	Entity door = globalECS->FindEntityByName("Exit");
	ShapeComponent* doorShape = globalECS->GetComponent<ShapeComponent>(door);

	Entity playerEntity = globalECS->FindEntityByName("Player");
	RigidBodyComponent* r = globalECS->GetComponent<RigidBodyComponent>(playerEntity);
	if (r) r->setMass(NORMAL_MASS);

	if(doorShape) doorShape->color == CEVec3(0.f, 0.f, 0.f);
	goalsDone = 0;

	pp1Done = false;
	pp2Done = false;

	LOGI(Core, "Descent Menu Initialized");
}

void UpdateDescentGame()
{
	using namespace CrashoutsEngine;

	// Update logic for DescentGame state
	if (engineState == EngineState::PLAY) {

		// Goals Text Display
		Entity goalsText = globalECS->FindEntityByName("Goals_Text");
		TextComponent* tComp = globalECS->GetComponent<TextComponent>(goalsText);
		if (tComp) {
			std::string text = "Activated: " + std::to_string(goalsDone) + " / " + std::to_string(REQUIRED_GOALS);
			tComp->text = text;
		}

		// Player W,A,S,D, SPACE
		Entity playerEntity = globalECS->FindEntityByName("Player");
		TransformComponent* playerTransform = globalECS->GetComponent<TransformComponent>(playerEntity);
		if (playerTransform) {
			if (InputManager::IsKeyTriggered(Key::A)) {
				if (playerTransform->size.x > 0) playerTransform->size.x *= -1.f;
			}
			if (InputManager::IsKeyTriggered(Key::D)) {
				if (playerTransform->size.x < 0) playerTransform->size.x *= -1.f;
			}
			if (InputManager::IsKeyTriggered(Key::E)) {
				if (energyBar > 0) energyBar -= ENERGY_USAGE;
				// Update energy bar
				Entity energyEntity = globalECS->FindEntityByName("Energy_Bar_Inner");
				TransformComponent* energyTransform = globalECS->GetComponent<TransformComponent>(energyEntity);
				if (energyTransform) {
					// 1. Calculate the new width
					float newWidth = energyBarLen * (energyBar * 0.01f);

					// 2. Calculate how much width was lost from the original
					float widthDifference = energyBarLen - newWidth;

					// 3. Set the new size
					energyTransform->size.x = newWidth;

					// 4. Set the new position:
					// Shift the center to the left by *half* the width that was lost.
					energyTransform->position.x = energyBarOriginX - (widthDifference / 2.0f);
				}

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
			
			// Check collision with the player and pressure plate
			// Pressure Plate 1
			Entity pp1 = globalECS->FindEntityByName("Pressure_Plate_1");
			TransformComponent* pp1Transform = globalECS->GetComponent<TransformComponent>(pp1);

			if (pp1Transform && !pp1Done && !isLight) {
				// Get difference with player
				float yDiff = (playerTransform->position.y - playerTransform->size.y * .5f) - 
					(pp1Transform->position.y + pp1Transform->size.y * .5f);

				//std::cout << "Player bottom limit is " << (playerTransform->position.y - playerTransform->size.y * .5f) <<
				//	" Plat top limit is " << (pp1Transform->position.y + pp1Transform->size.y * .5f) << " " << yDiff << std::endl;

				if (yDiff <= 0.f && yDiff >= Y_TOLERANCE) {	// IS ON TOP
					// Check within X
					float xMin = pp1Transform->position.x - pp1Transform->size.x * 0.2f;
					float xMax = pp1Transform->position.x + pp1Transform->size.x * 0.2f;
					float playerMin = playerTransform->position.x - playerTransform->size.x * 0.2f;
					float playerMax = playerTransform->position.x + playerTransform->size.x * 0.2f;
					if (playerMin < xMax && playerMax > xMin) {
						// Is Colliding?
						pp1Done = true;
						goalsDone++;
						pp1Transform->position.y -= pp1Transform->size.y * .5f;
					}
				}
			}

			// Pressure Plate 2
			Entity pp2 = globalECS->FindEntityByName("Pressure_Plate_2");
			TransformComponent* pp2Transform = globalECS->GetComponent<TransformComponent>(pp2);

			if (pp2Transform && !pp2Done && !isLight) {
				// Get difference with player
				float yDiff = (playerTransform->position.y - playerTransform->size.y * .5f) -
					(pp2Transform->position.y + pp2Transform->size.y * .5f);
				if (yDiff <= 0.f && yDiff >= Y_TOLERANCE) {	// IS ON TOP
					// Check within X
					float xMin = pp2Transform->position.x - pp2Transform->size.x * 0.2f;
					float xMax = pp2Transform->position.x + pp2Transform->size.x * 0.2f;
					float playerMin = playerTransform->position.x - playerTransform->size.x * 0.2f;
					float playerMax = playerTransform->position.x + playerTransform->size.x * 0.2f;
					if (playerMin < xMax && playerMax > xMin) {
						// Is Colliding?
						pp2Done = true;
						goalsDone++;
						pp2Transform->position.y -= pp2Transform->size.y * .5f;
					}
				}
			}

			// Unlock door if goal counter hits required
			if (goalsDone >= REQUIRED_GOALS) {
				// Get reference to door
				Entity door = globalECS->FindEntityByName("Exit");
				TransformComponent* doorTransform = globalECS->GetComponent<TransformComponent>(door);
				ShapeComponent* doorShape = globalECS->GetComponent<ShapeComponent>(door);

				if (doorShape && doorShape->color == CEVec3(0.f, 0.f, 0.f))
					doorShape->color = CEVec3(255.f, 255.f, 255.f); // unlocked visual cue

				// Compute door bounds
				float doorLeft = doorTransform->position.x - doorTransform->size.x * 0.2f;
				float doorRight = doorTransform->position.x + doorTransform->size.x * 0.2f;
				float doorTop = doorTransform->position.y + doorTransform->size.y * 0.2f;
				float doorBottom = doorTransform->position.y - doorTransform->size.y * 0.2f;

				// Compute player bounds
				float playerLeft = playerTransform->position.x - playerTransform->size.x * 0.2f;
				float playerRight = playerTransform->position.x + playerTransform->size.x * 0.2f;
				float playerTop = playerTransform->position.y + playerTransform->size.y * 0.2f;
				float playerBottom = playerTransform->position.y - playerTransform->size.y * 0.2f;

				// --- Proper AABB overlap check ---
				bool overlapX = (playerLeft < doorRight) && (playerRight > doorLeft);
				bool overlapY = (playerBottom < doorTop) && (playerTop > doorBottom);

				if (overlapX && overlapY) {
					// Player is inside door area
					RestartGameState();
					MonoManager::ReloadScripts();
					engineState = EngineState::STOP;
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
	if(engineState == EngineState::STOP && CurrentGameState != NextGameState) CrashoutsEngine::Serialization::SaveScenes(sceneName);
}

void UnloadDescentGame()
{
	// Unload DescentGame state
}