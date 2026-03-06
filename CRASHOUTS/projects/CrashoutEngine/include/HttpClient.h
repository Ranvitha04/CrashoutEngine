/*
@file       HttpClient.h
@author     
@date       04/03/2026
@brief      Simple HTTP client wrapper for libcurl.
*/
#pragma once

#include <string>

namespace CrashoutsEngine {

    class HttpClient {
    public:
        std::string HttpGet(const std::string& url);
        std::string HttpPostJson(const std::string& url, const std::string& jsonBody);
    };

}
