#pragma once

#include "entitymanager.h"
#include "gamestatesystem.h"

namespace States
{
	void EnterMainMenu(CrashoutsEngine::ECS& ecs);
	void ExitMainMenu(CrashoutsEngine::ECS& ecs);
}