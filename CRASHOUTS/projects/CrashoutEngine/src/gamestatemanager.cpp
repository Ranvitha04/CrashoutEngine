/*!
@file		gamestatemanager.cpp
@author     m.biniswandi@digipen.edu
@date       13/09/2025
@brief		Implementation of the game state manager

Handles registration initialization updating and transitions between different game states based on Soroor's Module
*/

#include "gamestatemanager.h"
#include <unordered_map>

 /// Registry that maps game states to their descriptors
static std::unordered_map<GameState, GameStateDescriptor> StateRegistry;

/// The current active game state
GameState CurrentGameState = MAIN_MENU;

/// The next game state requested
GameState NextGameState = MAIN_MENU;

/// The previous game state before the last transition
GameState PreviousGameState = MAIN_MENU;

/// Pointer to the descriptor of the current active state
static GameStateDescriptor* CurrentDescriptor = nullptr;

static bool RestartRequested = false;

void RestartGameState() {
	RestartRequested = true;
}

/*!
@brief Register a new game state with its descriptor
@param state The game state identifier
@param descriptor The descriptor containing state lifecycle functions
*/
void RegisterGameState(GameState state, GameStateDescriptor descriptor){
	StateRegistry[state] = descriptor;
}

/*!
@brief Initialize system components

Placeholder for global system initialization
*/
void InitSystemComponents() {
	// Initialize system components here
}

/*!
@brief Initialize the frame rate controller

Placeholder for frame rate control setup
*/
void InitFrameRateController() {
	// Initialize frame rate controller here
}

/*!
@brief Initialize the game state manager with an initial state
@param initialState The state to start the game in
*/
void InitGameStateManager(GameState initialState){
	CurrentGameState = NextGameState = PreviousGameState = initialState;
	CurrentDescriptor = &StateRegistry[CurrentGameState];

	if (CurrentDescriptor->Load){
		CurrentDescriptor->Load();
	}
	if (CurrentDescriptor->Init) {
		CurrentDescriptor->Init();
	}
}

/*!
@brief Make a transition to a new game state
@param newState The state to transition to
*/
void ChangeGameState(GameState newState) {
	NextGameState = newState;
}

/*!
@brief Update the game state manager

Handles transitions between states and calls update and render on the active state
*/
void UpdateGameStateManager() {
    /// restart to reload the same state
    if (RestartRequested) {
        if (CurrentDescriptor) {
            if (CurrentDescriptor->Free)   CurrentDescriptor->Free();
            if (CurrentDescriptor->Unload) CurrentDescriptor->Unload();
        }
        PreviousGameState = CurrentGameState;
        CurrentDescriptor = &StateRegistry[CurrentGameState];
        if (CurrentDescriptor->Load) CurrentDescriptor->Load();
        if (CurrentDescriptor->Init) CurrentDescriptor->Init();
        RestartRequested = false;
    }
    /// normal transition to a different state
    else if (NextGameState != CurrentGameState) {
        if (CurrentDescriptor) {
            if (CurrentDescriptor->Free)   CurrentDescriptor->Free();
            if (CurrentDescriptor->Unload) CurrentDescriptor->Unload();
        }
        PreviousGameState = CurrentGameState;
        CurrentGameState = NextGameState;
        CurrentDescriptor = &StateRegistry[CurrentGameState];
        if (CurrentDescriptor->Load) CurrentDescriptor->Load();
        if (CurrentDescriptor->Init) CurrentDescriptor->Init();
    }

    // per frame
    if (CurrentDescriptor) {
        if (CurrentDescriptor->Update) CurrentDescriptor->Update();
        if (CurrentDescriptor->Render) CurrentDescriptor->Render();
    }
}

/*!
@brief Reset the frame rate controller

Placeholder for resetting frame timing
*/
void ResetFrameRateController() {
	// Reset frame rate controller here
}

/*!
@brief Reset the frame rate controller

Placeholder for resetting frame timing
*/
void StartFrameRateController() {
	// Start frame rate controller here
}

/*!
@brief Update input status

Placeholder for input polling and updates
*/
void UpdateInputStatus() {
	// Update input status here
}

/*!
@brief End the frame rate controller

Placeholder for ending frame timing
*/
void EndFrameRateController() {
	// End frame rate controller here
}