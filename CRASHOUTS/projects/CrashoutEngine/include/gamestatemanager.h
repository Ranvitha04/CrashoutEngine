/**
 * @file gamestatemanager.h
 * @brief Declarations for the game state manager
 *
 * Provides enums structures and functions for handling game state registration initialization updates and transitions
 */
#pragma once


/// Function pointer type for state lifecycle functions
typedef void(*FP)(void);

/// Enumeration of possible game states
enum GameState{
	QUIT,
	MAIN_MENU,
	RESTART,
};

/// Descriptor for a game state containing lifecycle function pointers
struct GameStateDescriptor {
	FP load;
	FP init;
	FP update;
	FP render;
	FP free;
	FP unload;
};

/// Initialize core system components
extern void initSystemComponents();

/// Initialize the frame rate controller
extern void initFrameRateController();

/**
 * @brief Initialize the game state manager
 * @param state The initial state to start with
 */
void initGameStateManager(GameState state);

/// Update the game state manager handling transitions updates and rendering
void updateGameStateManager();

/**
 * @brief Change the game state
 * @param newState The new state to switch to
 */
void changeGameState(GameState newState);

/**
 * @brief Register a new game state and its descriptor
 * @param state The game state identifier
 * @param descriptor The descriptor containing state lifecycle functions
 */
void registerGameState(GameState state, GameStateDescriptor descriptor);

/// Reset the frame rate controller
extern void resetFrameRateController();

/// Start the frame rate controller
extern void startFrameRateController();

/// Update the input status
extern void updateInputStatus();

/// End the frame rate controller
extern void endFrameRateController();

/// The current active game state
extern GameState currentGameState;

/// The next game state requested
extern GameState nextGameState;

/// The previous game state before the current one
extern GameState previousGameState;