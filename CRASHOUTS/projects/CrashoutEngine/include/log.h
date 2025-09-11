#pragma once
#include <iostream>
#include <string>


namespace Log {

    // escape color codes
    constexpr const char* RESET = "\033[0m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* CYAN = "\033[36m";

    inline void Info(const std::string& msg) {
        std::cout << CYAN << "[INFO] " << msg << RESET << std::endl;
    }

    inline void Warning(const std::string& msg) {
        std::cout << YELLOW << "[WARNING] " << msg << RESET << std::endl;
    }

    inline void Error(const std::string& msg) {
        std::cout << RED << "[ERROR] " << msg << RESET << std::endl;
    }

    inline void Success(const std::string& msg) {
        std::cout << GREEN << "[SUCCESS] " << msg << RESET << std::endl;
    }
}