/*
@file       LeaderboardService.h
@author     
@date       04/03/2026
@brief      Leaderboard client for backend API.
*/
#pragma once

#include <string>
#include <vector>

namespace CrashoutsEngine {

    struct LeaderboardEntry {
        int rank{};
        std::string name;
        int score{};
        std::string created_at;
    };

    class LeaderboardService {
    public:
        explicit LeaderboardService(const std::string& baseUrl);

        void FetchLeaderboard(int limit);
        void SubmitScore(const std::string& name, int score);

        const std::vector<LeaderboardEntry>& GetEntries() const { return entries_; }

    private:
        std::string baseUrl_;
        std::vector<LeaderboardEntry> entries_;
    };

    // Example usage:
    // LeaderboardService svc("http://159.223.52.31");
    // svc.FetchLeaderboard(10);
    // svc.SubmitScore("Player1", 5000);

}
