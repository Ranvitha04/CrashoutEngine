/*!
@file		registergamestate.cpp
@author     Shahrin Idzuan
@date       13/09/2025
@brief		Registers all available game states

Creates descriptors for each state and registers them with the game state manager

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "registergamestate.h"

#include "gamestatemanager.h"
#include "scene.h"


/*!
@brief Register all game states

Builds descriptors for different scenes
*/
void RegisterAllStates(){
	GameStateDescriptor Scene = {
		"Scene",
		LoadScene,
		InitScene,
		UpdateScene,
		RenderScene,
		FreeScene,
		UnloadScene
	};

	RegisterGameState(SCENE, Scene);
}
