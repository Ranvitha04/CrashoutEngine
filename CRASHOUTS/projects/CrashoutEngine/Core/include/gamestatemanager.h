/*!
@file		gamestatemanager.h
@author     m.biniswandi@digipen.edu
@date       13/09/2025
@brief		Declarations for the game state manager

Provides enums structures and functions for handling game state registration initialization updates and transitions
*/
#pragma once
#pragma warning(disable: 4251)
#include <unordered_map>

#include "ImportExport.h"


/// Function pointer type for state lifecycle functions
typedef void(*FP)(void);

DLL_API void RestartGameState();

/// Enumeration of possible game states
DLL_API enum GameState{
	QUIT,
	MAIN_MENU,
	RENFRED,
	RAIN,
	AMIR,
	AFIQ,
	TRISTAN,
	RAN,
};

/// Descriptor for a game state containing lifecycle function pointers
DLL_API struct GameStateDescriptor {
	const char* Label;
	FP Load;
	FP Init;
	FP Update;
	FP Render;
	FP Free;
	FP Unload;
};

/// Initialize core system components
extern void InitSystemComponents();

/// Initialize the frame rate controller
extern void InitFrameRateController();

/**
 * @brief Initialize the game state manager
 * @param state The initial state to start with
 */
DLL_API void InitGameStateManager(GameState state);

/// Update the game state manager handling transitions updates and rendering
DLL_API void UpdateGameStateManager();

// Shutdown the game state manager and free allocated data
DLL_API void ShutdownGameStateManager();

/**
 * @brief Change the game state
 * @param newState The new state to switch to
 */
DLL_API void ChangeGameState(GameState newState);

/**
 * @brief Register a new game state and its descriptor
 * @param state The game state identifier
 * @param descriptor The descriptor containing state lifecycle functions
 */
DLL_API void RegisterGameState(GameState state, GameStateDescriptor descriptor);

/*!
@brief Exposes a view over the registry so that ImGui can read IDs - Amir was here
*/
DLL_API const std::unordered_map<GameState, GameStateDescriptor>& GetStateRegistry();

/// Reset the frame rate controller
extern void ResetFrameRateController();

/// Start the frame rate controller
extern void StartFrameRateController();

/// Update the input status
extern void UpdateInputStatus();

/// End the frame rate controller
extern void EndFrameRateController();

/// The current active game state
extern GameState CurrentGameState;

/// The next game state requested
extern GameState NextGameState;

/// The previous game state before the current one
extern GameState PreviousGameState;