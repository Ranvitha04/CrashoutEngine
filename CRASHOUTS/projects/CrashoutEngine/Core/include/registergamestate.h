/*!
@file		registergamestate.h
@author     m.biniswandi@digipen.edu
@date       13/09/2025
@brief		Declaration for game state registration

Provides a single function to register all available game states with the game state manager
*/

#pragma once
#include "ImportExport.h"

/*!
@brief Register all game states

Builds descriptors for each state and registers them in the game state manager
*/
DLL_API void RegisterAllStates();