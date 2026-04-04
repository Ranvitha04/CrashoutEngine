/*
@file       LeaderboardService.h
@author     Shahrin Idzuan
@date       04/03/2026
@brief      Leaderboard client for backend API.
*/
#pragma once

#include <string>
#include <vector>
#include <future>
#include <unordered_map>

namespace CrashoutsEngine {
    inline constexpr const char* kDefaultLeaderboardBaseUrl = "http://104.214.187.128";
    inline constexpr const char* kDefaultLeaderboardApiKey = "leaderboardsecretapikey";

    /** @brief LeaderboardEntry */
    struct LeaderboardEntry {
        int rank{};
        std::string name;
        int score{};
        std::string created_at;
        std::unordered_map<std::string, std::string> extraFields;
    };

    /** @brief LeaderboardService */
    class LeaderboardService {
    public:
        explicit LeaderboardService(const std::string& baseUrl,
            const std::string& apiKey = kDefaultLeaderboardApiKey);

        void FetchLeaderboard(int limit, const std::string& sortBy = "", const std::string& sortOrder = "");
        void SubmitScore(const std::string& name, int score);

        const std::vector<LeaderboardEntry>& GetEntries() const { return entries_; }

        // Async versions using JobSystem worker threads
        std::future<std::vector<LeaderboardEntry>> FetchLeaderboardAsync(int limit, const std::string& sortBy = "", const std::string& sortOrder = "");
        std::future<bool> SubmitScoreAsync(const std::string& name, int score);

    private:
        std::string baseUrl_;
        std::string apiKey_;
        std::vector<LeaderboardEntry> entries_;
    };

    // Example usage:
    // LeaderboardService svc("http://104.214.187.128");
    // svc.FetchLeaderboard(10);
    // svc.SubmitScore("Player1", 5000);
    // auto fut = svc.FetchLeaderboardAsync(10);
    // auto entries = fut.get();

}
