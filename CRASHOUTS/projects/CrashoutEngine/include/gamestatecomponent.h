#pragma once

#include "gamestate.h"
#include <array>

namespace CrashoutsEngine
{
	class ECS;
}

using GSCallback = void(*)(CrashoutsEngine::ECS&);

struct GameStateComponent
{
	GameState current = GameState::MAIN_MENU;
	GameState next = GameState::MAIN_MENU;
	GameState previous = GameState::MAIN_MENU;
	bool TransitionRequested = false;

	std::array<GSCallback, static_cast<size_t>(GameState::COUNT)> OnEnter{};
	std::array<GSCallback, static_cast<size_t>(GameState::COUNT)> OnExit{};
};