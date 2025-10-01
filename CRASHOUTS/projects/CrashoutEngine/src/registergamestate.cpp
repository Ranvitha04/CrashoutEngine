/*!
@file		registergamestate.cpp
@author     m.biniswandi@digipen.edu
@date       13/09/2025
@brief		Registers all available game states

Creates descriptors for each state and registers them with the game state manager
*/

#include "registergamestate.h"

#include "scenes/afiq.h"
#include "scenes/amir.h"
#include "scenes/mainmenu.h"
#include "scenes/rain.h"
#include "scenes/renfred.h"
#include "scenes/tristan.h"
#include "scenes/ran.h"

/*!
@brief Register all game states

Builds descriptors for different scenes
*/
void RegisterAllStates(){
	GameStateDescriptor MainMenu = {
		"Main Menu",
		LoadMainMenu,
		InitMainMenu,
		UpdateMainMenu,
		RenderMainMenu,
		FreeMainMenu,
		UnloadMainMenu
	};

	GameStateDescriptor Renfred = {
		"Renfred",
		LoadRenfred,
		InitRenfred,
		UpdateRenfred,
		RenderRenfred,
		FreeRenfred,
		UnloadRenfred
	};

	GameStateDescriptor Rain = {
		"Rain",
		LoadRain,
		InitRain,
		UpdateRain,
		RenderRain,
		FreeRain,
		UnloadRain
	};

	GameStateDescriptor Amir = {
		"Amir",
		LoadAmir,
		InitAmir,
		UpdateAmir,
		RenderAmir,
		FreeAmir,
		UnloadAmir
	};

	GameStateDescriptor Afiq = {
		"Afiq",
		LoadAfiq,
		InitAfiq,
		UpdateAfiq,
		RenderAfiq,
		FreeAfiq,
		UnloadAfiq
	};

	GameStateDescriptor Tristan = {
		"Tristan",
		LoadTristan,
		InitTristan,
		UpdateTristan,
		RenderTristan,
		FreeTristan,
		UnloadTristan
	};

	GameStateDescriptor Ran = {
		"Ran",
		LoadRan,
		InitRan,
		UpdateRan,
		RenderRan,
		FreeRan,
		UnloadRan
	};
	/// Register the states with the game state manager
	RegisterGameState(MAIN_MENU, MainMenu);
	RegisterGameState(RENFRED, Renfred);
	RegisterGameState(RAIN, Rain);
	RegisterGameState(AMIR, Amir);
	RegisterGameState(AFIQ, Afiq);
	RegisterGameState(TRISTAN, Tristan);
	RegisterGameState(RAN, Ran);
}