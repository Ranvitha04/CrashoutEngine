/*
@file       LeaderboardService.h
@author     
@date       04/03/2026
@brief      Leaderboard client for backend API.
*/
#pragma once

#include <string>
#include <vector>
#include <future>
#include <unordered_map>

namespace CrashoutsEngine {

    struct LeaderboardEntry {
        int rank{};
        std::string name;
        int score{};
        std::string created_at;
        std::unordered_map<std::string, std::string> extraFields;
    };

    class LeaderboardService {
    public:
        explicit LeaderboardService(const std::string& baseUrl);

        void FetchLeaderboard(int limit);
        void SubmitScore(const std::string& name, int score);

        const std::vector<LeaderboardEntry>& GetEntries() const { return entries_; }

        // Async versions using JobSystem worker threads
        std::future<std::vector<LeaderboardEntry>> FetchLeaderboardAsync(int limit);
        std::future<bool> SubmitScoreAsync(const std::string& name, int score, const std::string& ghostFilePath = "");

    private:
        std::string baseUrl_;
        std::vector<LeaderboardEntry> entries_;
    };

    // Example usage:
    // LeaderboardService svc("http://104.214.187.128");
    // svc.FetchLeaderboard(10);
    // svc.SubmitScore("Player1", 5000);
    // auto fut = svc.FetchLeaderboardAsync(10);
    // auto entries = fut.get();

}
