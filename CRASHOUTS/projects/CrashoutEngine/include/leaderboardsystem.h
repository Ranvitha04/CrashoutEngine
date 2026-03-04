/*!
@file       leaderboardsystem.h
@author
@date       04/03/2026
@brief      Runtime update system for LeaderboardComponent.
*/
#pragma once

namespace CrashoutsEngine {
    class ECS;

    namespace LeaderboardSystem {
        void Update(ECS& ecs, float dt, bool inEditor);
    }
}
