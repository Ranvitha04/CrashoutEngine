/*
@file       LeaderboardService.cpp
@author     
@date       04/03/2026
@brief      Leaderboard client for backend API.
*/
#include "LeaderboardService.h"
#include "HttpClient.h"
#include "jobsystem.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "log.h"

namespace CrashoutsEngine {

    // --- Read File to Byte Buffer ---
    static std::vector<unsigned char> ReadFileBinary(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return {};
        return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), {});
    }

    // --- Base64 Encoder ---
    static std::string Base64Encode(const std::vector<unsigned char>& data) {
        static const char* base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string ret;
        int i = 0, j = 0;
        unsigned char char_array_3[3], char_array_4[4];

        for (unsigned char c : data) {
            char_array_3[i++] = c;
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        if (i > 0) {
            for (j = i; j < 3; j++) char_array_3[j] = '\0';
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
            while (i++ < 3) ret += '=';
        }
        return ret;
    }

    static std::string JsonValueToString(const rapidjson::Value& v) {
        if (v.IsString()) return v.GetString();
        if (v.IsInt()) return std::to_string(v.GetInt());
        if (v.IsUint()) return std::to_string(v.GetUint());
        if (v.IsInt64()) return std::to_string(v.GetInt64());
        if (v.IsUint64()) return std::to_string(v.GetUint64());
        if (v.IsDouble()) return std::to_string(v.GetDouble());
        if (v.IsBool()) return v.GetBool() ? "true" : "false";
        return "";
    }

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
            for (auto it = item.MemberBegin(); it != item.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                if (key == "rank" || key == "name" || key == "score" || key == "created_at") continue;
                e.extraFields[key] = JsonValueToString(it->value);
            }
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

    std::future<std::vector<LeaderboardEntry>> LeaderboardService::FetchLeaderboardAsync(int limit) {
        std::string baseUrl = baseUrl_;
        return JobSystem::Get().Enqueue([baseUrl, limit]() {
            std::vector<LeaderboardEntry> results;

            HttpClient http;
            std::string url = baseUrl + "/leaderboard?limit=" + std::to_string(limit);
            std::string response = http.HttpGet(url);
            if (response.empty()) return results;

            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.HasParseError() || !doc.IsObject()) return results;
            if (!doc.HasMember("entries") || !doc["entries"].IsArray()) return results;

            const auto& arr = doc["entries"];
            for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
                const auto& item = arr[i];
                if (!item.IsObject()) continue;

                LeaderboardEntry e{};
                if (item.HasMember("rank") && item["rank"].IsInt()) e.rank = item["rank"].GetInt();
                if (item.HasMember("name") && item["name"].IsString()) e.name = item["name"].GetString();
                if (item.HasMember("score") && item["score"].IsInt()) e.score = item["score"].GetInt();
                if (item.HasMember("created_at") && item["created_at"].IsString()) e.created_at = item["created_at"].GetString();
                for (auto it = item.MemberBegin(); it != item.MemberEnd(); ++it) {
                    std::string key = it->name.GetString();
                    if (key == "rank" || key == "name" || key == "score" || key == "created_at") continue;
                    e.extraFields[key] = JsonValueToString(it->value);
                }
                results.push_back(std::move(e));
            }

            return results;
        });
    }

    std::future<bool> LeaderboardService::SubmitScoreAsync(const std::string& name, int score, const std::string& ghostFilePath) {
        std::string baseUrl = baseUrl_;
        std::string nameCopy = name;
        std::string ghostPathCopy = ghostFilePath; // Capture the path for the async thread

        return JobSystem::Get().Enqueue([baseUrl, nameCopy, score, ghostPathCopy]() {
            rapidjson::Document doc;
            doc.SetObject();
            auto& alloc = doc.GetAllocator();

            doc.AddMember("name", rapidjson::Value(nameCopy.c_str(), alloc), alloc);
            doc.AddMember("score", score, alloc);
            doc.AddMember("time", score, alloc);

            // --- ATTACH GHOST DATA IF IT EXISTS ---
            if (!ghostPathCopy.empty()) {
                std::vector<unsigned char> fileData = ReadFileBinary(ghostPathCopy);
                if (!fileData.empty()) {
                    std::string base64Ghost = Base64Encode(fileData);
                    doc.AddMember("ghost_data", rapidjson::Value(base64Ghost.c_str(), alloc), alloc);
                    LOGI(Debug, "[Leaderboard] Attached Ghost Data: " + std::to_string(fileData.size()) + " bytes.");
                }
                else {
                    LOGI(Debug, "[Leaderboard] Warning: Could not read ghost file at " + ghostPathCopy);
                }
            }

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            HttpClient http;
            std::string url = baseUrl + "/submitScore";
            std::string response = http.HttpPostJson(url, buffer.GetString());

            if (response.empty()) {
                LOGI(Debug, "[Leaderboard] ERROR: Server did not respond.");
            }
            else {
                LOGI(Debug, "[Leaderboard] Server Response: " + response);
            }

            return !response.empty();
        });
    }

}
