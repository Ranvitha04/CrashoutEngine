/*
@file       HttpClient.h
@author     Shahrin Idzuan
@date       04/03/2026
@brief      Simple HTTP client wrapper for libcurl.
*/
#pragma once

#include <string>
#include <vector>

namespace CrashoutsEngine {

    /** @brief HttpClient */
    class HttpClient {
    public:
        std::string HttpGet(const std::string& url, const std::vector<std::string>& headers = {});
        std::string HttpPostJson(const std::string& url, const std::string& jsonBody,
            const std::vector<std::string>& headers = {});
    };

}
