/*
@file       LeaderboardService.cpp
@author     Renfred Lee
@author     Shahrin Idzuan
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
    namespace {
        /*!
        @brief Build HTTP headers required for authenticated leaderboard requests.
        @param apiKey Optional API key configured for the backend.
        @return Header list suitable for HttpClient calls.
        */
        static std::vector<std::string> BuildApiKeyHeaders(const std::string& apiKey) {
            if (apiKey.empty()) return {};
            return {
                "X-API-Key: " + apiKey
            };
        }

        /*!
        @brief Construct a leaderboard GET endpoint with query parameters.
        @param baseUrl Base backend URL.
        @param limit Maximum number of entries to request.
        @param sortBy Field name to sort by.
        @param sortOrder Requested sort order such as asc or desc.
        @return Fully composed leaderboard request URL.
        */
        static std::string BuildLeaderboardUrl(const std::string& baseUrl, int limit,
            const std::string& sortBy, const std::string& sortOrder) {
            std::string url = baseUrl + "/leaderboard?limit=" + std::to_string(limit);
            if (!sortBy.empty()) {
                url += "&sortBy=" + sortBy;
            }
            if (!sortOrder.empty()) {
                url += "&sortOrder=" + sortOrder;
            }
            return url;
        }
    }

    /*!
    @brief Convert an arbitrary RapidJSON value into a string for extra leaderboard fields.
    @param v JSON value to convert.
    @return String representation of the input value, or an empty string for unsupported types.
    */
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

    /*!
    @brief Construct a service wrapper for one leaderboard backend.
    @param baseUrl Base backend URL.
    @param apiKey Optional API key header value.
    */
    LeaderboardService::LeaderboardService(const std::string& baseUrl, const std::string& apiKey)
        : baseUrl_(baseUrl), apiKey_(apiKey) {}

    /*!
    @brief Fetch leaderboard entries synchronously and cache them in the service instance.
    @param limit Maximum number of entries to request.
    @param sortBy Field name to sort by.
    @param sortOrder Requested sort order such as asc or desc.
    */
    void LeaderboardService::FetchLeaderboard(int limit, const std::string& sortBy, const std::string& sortOrder) {
        entries_.clear();

        HttpClient http;
        std::string url = BuildLeaderboardUrl(baseUrl_, limit, sortBy, sortOrder);
        std::string response = http.HttpGet(url, BuildApiKeyHeaders(apiKey_));
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

    /*!
    @brief Submit a score synchronously to the backend.
    @param name Player name to submit.
    @param score Score value to send.
    */
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
        http.HttpPostJson(url, buffer.GetString(), BuildApiKeyHeaders(apiKey_));
    }

    /*!
    @brief Queue an asynchronous leaderboard fetch on the job system.
    @param limit Maximum number of entries to request.
    @param sortBy Field name to sort by.
    @param sortOrder Requested sort order such as asc or desc.
    @return Future containing the fetched leaderboard entries.
    */
    std::future<std::vector<LeaderboardEntry>> LeaderboardService::FetchLeaderboardAsync(
        int limit, const std::string& sortBy, const std::string& sortOrder) {
        std::string baseUrl = baseUrl_;
        std::string apiKey = apiKey_;
        std::string sortByCopy = sortBy;
        std::string sortOrderCopy = sortOrder;
        return JobSystem::Get().Enqueue([baseUrl, apiKey, limit, sortByCopy, sortOrderCopy]() {
            std::vector<LeaderboardEntry> results;

            HttpClient http;
            std::string url = BuildLeaderboardUrl(baseUrl, limit, sortByCopy, sortOrderCopy);
            std::string response = http.HttpGet(url, BuildApiKeyHeaders(apiKey));
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

    /*!
    @brief Queue an asynchronous score submission on the job system.
    @param name Player name to submit.
    @param score Score value to send.
    @return Future resolving to true when the backend returned a non-empty response.
    */
    std::future<bool> LeaderboardService::SubmitScoreAsync(const std::string& name, int score) {
        std::string baseUrl = baseUrl_;
        std::string apiKey = apiKey_;
        std::string nameCopy = name;

        return JobSystem::Get().Enqueue([baseUrl, apiKey, nameCopy, score]() {
            rapidjson::Document doc;
            doc.SetObject();
            auto& alloc = doc.GetAllocator();

            doc.AddMember("name", rapidjson::Value(nameCopy.c_str(), alloc), alloc);
            doc.AddMember("score", score, alloc);
            doc.AddMember("time", score, alloc);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            HttpClient http;
            std::string url = baseUrl + "/submitScore";
            std::string response = http.HttpPostJson(url, buffer.GetString(), BuildApiKeyHeaders(apiKey));

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
