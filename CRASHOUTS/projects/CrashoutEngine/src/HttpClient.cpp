/*
@file       HttpClient.cpp
@author     
@date       04/03/2026
@brief      Simple HTTP client wrapper for libcurl.
*/
#include "HttpClient.h"
#include <curl/curl.h>

namespace CrashoutsEngine {

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        if (!userp || total == 0) return 0;
        std::string* out = static_cast<std::string*>(userp);
        out->append(static_cast<const char*>(contents), total);
        return total;
    }

    std::string HttpClient::HttpGet(const std::string& url) {
        std::string response;
        CURL* curl = curl_easy_init();
        if (!curl) return response;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            response.clear();
        }

        curl_easy_cleanup(curl);
        return response;
    }

    std::string HttpClient::HttpPostJson(const std::string& url, const std::string& jsonBody) {
        std::string response;
        CURL* curl = curl_easy_init();
        if (!curl) return response;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            response.clear();
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return response;
    }

}
