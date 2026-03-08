/*!
@file       leaderboardbindings.cpp
@author     lee.r@digipen.edu
@date       08/03/2026
@brief      Bind leaderboard functionalities to C#

All content © 2026 Digipen Institute of Technology Singapore. All rights reserved.
*/

#include <mono/jit/jit.h>
//#include <mono/metadata/string-icalls.h> // For mono_string_to_utf8
#include "LeaderboardService.h"
#include "leaderboardbindings.h"

using namespace CrashoutsEngine;
using namespace Component;

namespace {

    // The C++ function that C# will call (URL removed)
    static void SubmitScore_Internal(MonoString* nameObj, int score) {
        if (!nameObj) return;

        char* nameCStr = mono_string_to_utf8(nameObj);
        std::string name(nameCStr);
        mono_free(nameCStr);

        // Define your Single Source of Truth here! 
        // (Alternatively, fetch this from a Settings/Config manager)
        std::string backendUrl = "http://104.214.187.128";

        // Instantiate the service and submit asynchronously
        LeaderboardService svc(backendUrl);
        svc.SubmitScoreAsync(name, score);
    }
}

void CrashoutsEngine::Bindings::RegisterLeaderboardBindings() {
    mono_add_internal_call("CrashoutsEngine.Leaderboard::SubmitScore_Internal", (void*)SubmitScore_Internal);
}