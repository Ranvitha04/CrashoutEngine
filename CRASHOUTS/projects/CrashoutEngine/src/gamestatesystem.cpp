#include "gamestatesystem.h"
#include <cassert>


size_t GameStateSystem::Index(GameState state)
{
	const auto i = static_cast<size_t>(state);
	assert(i < static_cast<size_t>(GameState::COUNT));
	return i;
}

void GameStateSystem::Init(CrashoutsEngine::ECS& ecs, GameState initial)
{
	mECS = &ecs;
	mGS.current = mGS.next = mGS.previous = initial;
	mGS.TransitionRequested = true; // force first transition
}

void GameStateSystem::SetCallbacks(GameState state, GSCallback onEnter, GSCallback onExit)
{
	mGS.OnEnter[Index(state)] = onEnter;
	mGS.OnExit[Index(state)] = onExit;
}

void GameStateSystem::Request(GameState state)
{
	mGS.next = state;
	mGS.TransitionRequested = true;
}

void GameStateSystem::Restart()
{
	mGS.next = mGS.current;
	mGS.TransitionRequested = true;
}

void GameStateSystem::Tick()
{
	if (!mGS.TransitionRequested || !mECS) {
		return;
	}

	if (mGS.next == mGS.current) {
		const auto i = Index(mGS.current);

		if (mGS.OnExit[i]) {
			mGS.OnExit[i](*mECS);
		}

		if (mGS.OnEnter[i]) {
			mGS.OnEnter[i](*mECS);
		}

		mGS.previous = mGS.current;
		mGS.TransitionRequested = false;
		return;
	}

	const auto CurrentIndex = Index(mGS.current);
	if (mGS.OnExit[CurrentIndex]) {
		mGS.OnExit[CurrentIndex](*mECS);
	}

	mGS.previous = mGS.current;
	mGS.current = mGS.next;

	const auto NewIndex = Index(mGS.current);

	if (mGS.OnEnter[NewIndex]) {
		mGS.OnEnter[NewIndex](*mECS);
	}

	mGS.TransitionRequested = false;
}

GameState GameStateSystem::Current() const
{
	return mGS.current;
}