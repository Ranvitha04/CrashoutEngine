/*!
@file registergamestate.cpp
@brief Registers all available game states

Creates descriptors for each state and registers them with the game state manager
*/

#include "registergamestate.h"

#include "mainmenu.h"

/*!
@brief Register all game states

Builds descriptors for different scenes
*/
void RegisterAllStates(){
	GameStateDescriptor MainMenu = {
		LoadMainMenu,
		InitMainMenu,
		UpdateMainMenu,
		RenderMainMenu,
		FreeMainMenu,
		UnloadMainMenu
	};

	/// Register the states with the game state manager
	RegisterGameState(MAIN_MENU, MainMenu);
}