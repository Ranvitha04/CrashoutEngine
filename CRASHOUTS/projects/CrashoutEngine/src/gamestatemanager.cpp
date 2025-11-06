/*!
@file		gamestatemanager.cpp
@author     m.biniswandi@digipen.edu
@date       13/09/2025
@brief		Implementation of the game state manager

Handles registration initialization updating and transitions between different game states based on Soroor's Module
*/

#include "gamestatemanager.h"
#include "engine.h"
#include <GLFW/glfw3.h>

 /// Registry that maps game states to their descriptors
static std::unordered_map<GameState, GameStateDescriptor> StateRegistry;

/// The current active game state
GameState CurrentGameState = MAIN_MENU;

/// The next game state requested
GameState NextGameState = MAIN_MENU;

/// The previous game state before the last transition
GameState PreviousGameState = MAIN_MENU;

/// Pointer to the descriptor of the current active state
GameStateDescriptor* CurrentDescriptor = nullptr;

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
@brief Exposes a view over the registry so that ImGui can read IDs
*/
const std::unordered_map<GameState, GameStateDescriptor>& GetStateRegistry() {
    return StateRegistry; // existing static in this translation unit
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
void ChangeGameState(GameState newState, bool resetEngineState) {
    if(resetEngineState) engineState = EngineState::STOP;
    //engineState = EngineState::STOP;
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
        //if (CurrentDescriptor->Update) CurrentDescriptor->Update();
        //if (CurrentDescriptor->Render) CurrentDescriptor->Render();
        glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

/*!
@brief Shuts down the Game State Manager.
@details
Clears the static registry map to explicitly free all dynamically allocated
memory associated with the registered state descriptors before program exit.
*/
void ShutdownGameStateManager() {
    // Clean up the currently active state's resources
    if (CurrentDescriptor) {
        if (CurrentDescriptor->Free) CurrentDescriptor->Free();
        if (CurrentDescriptor->Unload) CurrentDescriptor->Unload();
    }

    StateRegistry.clear();

    CurrentDescriptor = nullptr;
    CurrentGameState = NextGameState = PreviousGameState = MAIN_MENU;
    RestartRequested = false;
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

