/*
@file       HttpClient.cpp
@author     Shahrin Idzuan
@date       04/03/2026
@brief      Simple HTTP client wrapper for libcurl.
*/
#include "HttpClient.h"
#include <curl/curl.h>

namespace CrashoutsEngine {
    namespace {
        /** @brief curl_slist */
        struct curl_slist* BuildHeaders(const std::vector<std::string>& headers) {
            /** @brief curl_slist */
            struct curl_slist* curlHeaders = nullptr;
            for (const std::string& header : headers) {
                curlHeaders = curl_slist_append(curlHeaders, header.c_str());
            }
            return curlHeaders;
        }
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        if (!userp || total == 0) return 0;
        std::string* out = static_cast<std::string*>(userp);
        out->append(static_cast<const char*>(contents), total);
        return total;
    }

    std::string HttpClient::HttpGet(const std::string& url, const std::vector<std::string>& headers) {
        std::string response;
        CURL* curl = curl_easy_init();
        if (!curl) return response;
        /** @brief curl_slist */
        struct curl_slist* curlHeaders = BuildHeaders(headers);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (curlHeaders) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            response.clear();
        }

        curl_slist_free_all(curlHeaders);
        curl_easy_cleanup(curl);
        return response;
    }

    std::string HttpClient::HttpPostJson(const std::string& url, const std::string& jsonBody,
        const std::vector<std::string>& headers) {
        std::string response;
        CURL* curl = curl_easy_init();
        if (!curl) return response;

        std::vector<std::string> combinedHeaders = headers;
        combinedHeaders.push_back("Content-Type: application/json");
        /** @brief curl_slist */
        struct curl_slist* curlHeaders = BuildHeaders(combinedHeaders);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
        if (curlHeaders) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            response.clear();
        }

        curl_slist_free_all(curlHeaders);
        curl_easy_cleanup(curl);
        return response;
    }

}
