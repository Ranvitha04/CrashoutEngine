/**
 * @file gamestatemanager.cpp
 * @brief Implementation of the game state manager
 *
 * Handles registration initialization updating and transitions between different game states based on Soroor's Module
 */

#include "gamestatemanager.h"

#include <unordered_map>

 /// Registry that maps game states to their descriptors
static std::unordered_map<GameState, GameStateDescriptor> stateRegistry;

/// The current active game state
GameState currentGameState = MAIN_MENU;

/// The next game state requested
GameState nextGameState = MAIN_MENU;

/// The previous game state before the last transition
GameState previousGameState = MAIN_MENU;

/// Pointer to the descriptor of the current active state
static GameStateDescriptor* currentDescriptor = nullptr;

/**
 * @brief Register a new game state with its descriptor
 * @param state The game state identifier
 * @param descriptor The descriptor containing state lifecycle functions
 */
void registerGameState(GameState state, GameStateDescriptor descriptor){
	stateRegistry[state] = descriptor;
}

/**
 * @brief Initialize system components
 *
 * Placeholder for global system initialization
 */
void initSystemComponents() {
	// Initialize system components here
}

/**
 * @brief Initialize the frame rate controller
 *
 * Placeholder for frame rate control setup
 */
void initFrameRateController() {
	// Initialize frame rate controller here
}

/**
 * @brief Initialize the game state manager with an initial state
 * @param initialState The state to start the game in
 */
void initGameStateManager(GameState initialState){
	currentGameState = nextGameState = previousGameState = initialState;
	currentDescriptor = &stateRegistry[currentGameState];

	if (currentDescriptor->load){
		currentDescriptor->load();
	}
	if (currentDescriptor->init) {
		currentDescriptor->init();
	}
}

/**
 * @brief Make a transition to a new game state
 * @param newState The state to transition to
 */
void changeGameState(GameState newState) {
	nextGameState = newState;
}

/**
 * @brief Update the game state manager
 *
 * Handles transitions between states and calls update and render on the active state
 */
void updateGameStateManager() {
	if (nextGameState!=currentGameState) {
		if (currentDescriptor){
			if (currentDescriptor->free) currentDescriptor->free();
			if (currentDescriptor->unload) currentDescriptor->unload();
		}

		// switch to the next game state
		previousGameState = currentGameState;
		currentGameState = nextGameState;
		currentDescriptor = &stateRegistry[currentGameState];

		// load new state
		if (currentDescriptor->load) currentDescriptor->load();
		if (currentDescriptor->init) currentDescriptor->init();
	}
	if (nextGameState == RESTART) {
		nextGameState = previousGameState;
	}

	// per frame update and render
	if (currentDescriptor) {
		if (currentDescriptor->update) currentDescriptor->update();
		if (currentDescriptor->render) currentDescriptor->render();
	}
}

/**
 * @brief Reset the frame rate controller
 *
 * Placeholder for resetting frame timing
 */
void resetFrameRateController() {
	// Reset frame rate controller here
}

/**
 * @brief Reset the frame rate controller
 *
 * Placeholder for resetting frame timing
 */
void startFrameRateController() {
	// Start frame rate controller here
}

/**
 * @brief Update input status
 *
 * Placeholder for input polling and updates
 */
void updateInputStatus() {
	// Update input status here
}

/**
 * @brief End the frame rate controller
 *
 * Placeholder for ending frame timing
 */
void endFrameRateController() {
	// End frame rate controller here
}