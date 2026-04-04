/*!
@file       leaderboardcomponent.h
@author
@date       04/03/2026
@brief      Component data for leaderboard UI + refresh settings.
*/
#pragma once

#include "LeaderboardService.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace Component {

    /** @brief LeaderboardEntryData */
    struct LeaderboardEntryData {
        int rank{};
        std::string name;
        int score{};
        std::string created_at;
        std::unordered_map<std::string, std::string> extraFields;
    };

    /** @brief LeaderboardComponent */
    struct LeaderboardComponent {
        // Connection
        std::string baseUrl{ CrashoutsEngine::kDefaultLeaderboardBaseUrl };
        std::string apiKey{ CrashoutsEngine::kDefaultLeaderboardApiKey };
        int limit{ 10 };
        std::string sortBy{ "" };      // e.g. "time" for fastest-time boards
        std::string sortOrder{ "" };   // e.g. "asc" for fastest first, "desc" for highest first

        // Refresh
        bool autoRefresh{ true };
        bool fetchOnStart{ true };
        bool updateInEditor{ false };
        float refreshIntervalSec{ 10.0f };

        // UI Binding
        std::string rootEntityName{ "" };       // Optional override for UI root
        std::string entryNamePrefix{ "LB_Entry_" };
        int entryIndexBase{ 1 };
        std::string lineFormat{ "{rank}. {name} - {score}" };
        std::string emptyLineText{ "" };

        // Column mode
        bool useColumns{ false };
        std::string rankNamePrefix{ "LB_Rank_" };
        std::string playerNamePrefix{ "LB_Name_" };
        std::string scoreNamePrefix{ "LB_Score_" };
        std::string dateNamePrefix{ "" }; // Empty = skip date column
        std::vector<std::string> columnKeys;
        std::vector<std::string> columnPrefixes;

        // Render into own TextComponent (no prefab required)
        bool renderToSelfText{ false };
        bool selfTextIncludeHeader{ true };
        bool selfTextIncludeStatus{ true };
        std::string selfTextSeparator{ " | " };

        // Optional text targets
        std::string headerEntityName{ "" };
        std::string headerText{ "" };
        std::string statusEntityName{ "" };
        std::string loadingText{ "Loading..." };
        std::string errorText{ "Failed to load" };
        std::string emptyStatusText{ "" };

        // Runtime state (not serialized)
        float refreshTimer{ 0.0f };
        bool requestNow{ false };
        bool hasEverFetched{ false };
        std::string lastError{ "" };
        std::vector<LeaderboardEntryData> entries;
    };

}
