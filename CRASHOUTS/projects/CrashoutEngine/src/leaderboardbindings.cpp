/*!
@file       leaderboardbindings.cpp
@author     Renfred (RTIS)
@author     Renfred Lee
@author     Shahrin Idzuan
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

    // The C++ function that C# will call (URL removed)
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

    // This allows C# to instantly update the UI after submitting a score
    static void ForceRefresh_Internal(Entity entityID) {
        auto* ecs = ECS::getInstance();
        if (ecs->HasComponent<Component::LeaderboardComponent>(entityID)) {
            ecs->GetComponent<Component::LeaderboardComponent>(entityID)->requestNow = true;
        }
    }

}

void CrashoutsEngine::Bindings::RegisterLeaderboardBindings() {
    // Register the Submit function
    mono_add_internal_call("CrashoutsEngine.Leaderboard::SubmitScore_Internal", (void*)SubmitScore_Internal);
    // Register the Refresh function
    mono_add_internal_call("CrashoutsEngine.Leaderboard::ForceRefresh_Internal", (void*)ForceRefresh_Internal);
}
