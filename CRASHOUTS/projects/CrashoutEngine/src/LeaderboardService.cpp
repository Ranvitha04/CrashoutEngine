/*
@file       LeaderboardService.cpp
@author     
@date       04/03/2026
@brief      Leaderboard client for backend API.
*/
#include "LeaderboardService.h"
#include "HttpClient.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace CrashoutsEngine {

    LeaderboardService::LeaderboardService(const std::string& baseUrl)
        : baseUrl_(baseUrl) {}

    void LeaderboardService::FetchLeaderboard(int limit) {
        entries_.clear();

        HttpClient http;
        std::string url = baseUrl_ + "/leaderboard?limit=" + std::to_string(limit);
        std::string response = http.HttpGet(url);
        if (response.empty()) return;

        rapidjson::Document doc;
        doc.Parse(response.c_str());
        if (doc.HasParseError() || !doc.IsObject()) return;
        if (!doc.HasMember("entries") || !doc["entries"].IsArray()) return;

        const auto& arr = doc["entries"];
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
            const auto& item = arr[i];
            if (!item.IsObject()) continue;

            LeaderboardEntry e{};
            if (item.HasMember("rank") && item["rank"].IsInt()) e.rank = item["rank"].GetInt();
            if (item.HasMember("name") && item["name"].IsString()) e.name = item["name"].GetString();
            if (item.HasMember("score") && item["score"].IsInt()) e.score = item["score"].GetInt();
            if (item.HasMember("created_at") && item["created_at"].IsString()) e.created_at = item["created_at"].GetString();
            entries_.push_back(std::move(e));
        }
    }

    void LeaderboardService::SubmitScore(const std::string& name, int score) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        doc.AddMember("name", rapidjson::Value(name.c_str(), alloc), alloc);
        doc.AddMember("score", score, alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        HttpClient http;
        std::string url = baseUrl_ + "/submitScore";
        http.HttpPostJson(url, buffer.GetString());
    }

}
