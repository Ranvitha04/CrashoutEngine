/**
 * @file registergamestate.cpp
 * @brief Registers all available game states
 *
 * Creates descriptors for each state and registers them with the game state manager
 */

#include "registergamestate.h"

#include "mainmenu.h"

 /**
  * @brief Register all game states
  *
  * Builds descriptors for main menu top view and iso view and adds them to the game state registry
  */
void registerAllStates(){
	GameStateDescriptor mainMenu = {
		LoadMainMenu,
		InitMainMenu,
		UpdateMainMenu,
		RenderMainMenu,
		FreeMainMenu,
		UnloadMainMenu
	};

	/// Register the states with the game state manager
	registerGameState(MAIN_MENU, mainMenu);
}