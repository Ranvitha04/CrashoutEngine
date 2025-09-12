#pragma once
#include "gamestatecomponent.h"

namespace CrashoutsEngine { class ECS; }

class GameStateSystem
{
public:
	void Init(CrashoutsEngine::ECS& ecs, GameState initial);
	void SetCallbacks(GameState state, GSCallback onEnter, GSCallback onExit);
	void Request(GameState state);
	void Restart();
	void Tick();
	GameState Current() const;

private:
	static size_t Index(GameState state);

	GameStateComponent mGS;
	CrashoutsEngine::ECS* mECS = nullptr;
};