/*!
@file       leaderboardbindings.cpp
@author     Renfred (70%)
@author     Shahrin Idzuan (30%)
@date       08/03/2026
@brief      Bind leaderboard functionalities to C#

All content � 2026 Digipen Institute of Technology Singapore. All rights reserved.
*/

#include <mono/jit/jit.h>
#include "LeaderboardService.h"
#include "leaderboardbindings.h"
#include "leaderboardcomponent.h"

using namespace CrashoutsEngine;
using namespace Component;

namespace {

    /*!
    @brief Native C# binding that submits a player score to the default leaderboard backend.
    @param nameObj Managed player name string.
    @param score Score value to submit.
    */
    static void SubmitScore_Internal(MonoString* nameObj, int score) {
        if (!nameObj) return;

        // Convert Name string
        char* nameCStr = mono_string_to_utf8(nameObj);
        std::string name(nameCStr);
        mono_free(nameCStr);

        // Define your Single Source of Truth here! 
        std::string backendUrl = CrashoutsEngine::kDefaultLeaderboardBaseUrl;

        // Instantiate the service and submit asynchronously
        LeaderboardService svc(backendUrl, CrashoutsEngine::kDefaultLeaderboardApiKey);

        svc.SubmitScoreAsync(name, score);
    }

    /*!
    @brief Native C# binding that flags a leaderboard component for immediate refresh.
    @param entityID Entity containing the LeaderboardComponent to refresh.
    */
    static void ForceRefresh_Internal(Entity entityID) {
        auto* ecs = ECS::getInstance();
        if (ecs->HasComponent<Component::LeaderboardComponent>(entityID)) {
            ecs->GetComponent<Component::LeaderboardComponent>(entityID)->requestNow = true;
        }
    }

}

/*!
@brief Register Mono internal calls exposed by the leaderboard system to managed scripts.
*/
void CrashoutsEngine::Bindings::RegisterLeaderboardBindings() {
    // Register the Submit function
    mono_add_internal_call("CrashoutsEngine.Leaderboard::SubmitScore_Internal", (void*)SubmitScore_Internal);
    // Register the Refresh function
    mono_add_internal_call("CrashoutsEngine.Leaderboard::ForceRefresh_Internal", (void*)ForceRefresh_Internal);
}
