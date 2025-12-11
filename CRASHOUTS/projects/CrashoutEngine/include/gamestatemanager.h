/*!
@file		gamestatemanager.h
@author     m.biniswandi@digipen.edu
@date       13/09/2025
@brief		Declarations for the game state manager

Provides enums structures and functions for handling game state registration initialization updates and transitions

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include <unordered_map>


/// Function pointer type for state lifecycle functions
typedef void(*FP)(void);

void RestartGameState();

/// Enumeration of possible game states
enum GameState{
	QUIT,
	MAIN_MENU,
	RENFRED,
	RAIN,
	AMIR,
	AFIQ,
	TRISTAN,
	RAN,
	//DESCENT_MENU,
	//DESCENT_GAME,
	SCENE
};

/// Descriptor for a game state containing lifecycle function pointers
struct GameStateDescriptor {
	const char* Label;
	FP Load;
	FP Init;
	FP Update;
	FP Render;
	FP Free;
	FP Unload;
};

/**
 * @brief Initialize the game state manager
 * @param state The initial state to start with
 */
void InitGameStateManager(GameState state);

/// Update the game state manager handling transitions updates and rendering
void UpdateGameStateManager();

// Shutdown the game state manager and free allocated data
void ShutdownGameStateManager();

/**
 * @brief Change the game state
 * @param newState The new state to switch to
 */
void ChangeGameState(GameState newState, bool resetEngineState = true);

/**
 * @brief Register a new game state and its descriptor
 * @param state The game state identifier
 * @param descriptor The descriptor containing state lifecycle functions
 */
void RegisterGameState(GameState state, GameStateDescriptor descriptor);

/*!
@brief Exposes a view over the registry so that ImGui can read IDs - Amir was here
*/
const std::unordered_map<GameState, GameStateDescriptor>& GetStateRegistry();

/// The current active game state
extern GameState CurrentGameState;

/// The next game state requested
extern GameState NextGameState;

/// The previous game state before the current one
extern GameState PreviousGameState;

extern GameStateDescriptor* CurrentDescriptor;