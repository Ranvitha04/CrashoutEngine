#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>

struct LogMessage {
    std::string text;
    enum class Level {
        Info,
        Warning,
        Error,
        Success
    } level;
};


class Logger {
public:
    static Logger& Instance(){
        static Logger instance;
        return instance;
    }

    void Add(LogMessage::Level lvl, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        messages.push_back({ msg, lvl });

        switch (lvl) {
        case LogMessage::Level::Info:
            std::cout << "\033[36m[Info] " << msg << "\033[0m\n"; 
            break;
        case LogMessage::Level::Warning:
            std::cout << "\033[33m[Warning] " << msg << "\033[0m\n";
            break;
        case LogMessage::Level::Error:
            std::cout << "\033[31m[Error] " << msg << "\033[0m\n";
            break;
        case LogMessage::Level::Success:
            std::cout << "\033[32m[Success " << msg << "\033[0m\n";
            break;
        }
    }

    const std::vector<LogMessage>& GetMessages() const {
        return messages;
    }
  
private:
    std::vector<LogMessage> messages;
    std::mutex mutex;
};

#define LOG_INFO(x)    Logger::Instance().Add(LogMessage::Level::Info, x)
#define LOG_WARN(x)    Logger::Instance().Add(LogMessage::Level::Warning, x)
#define LOG_ERROR(x)   Logger::Instance().Add(LogMessage::Level::Error, x)
#define LOG_SUCCESS(x) Logger::Instance().Add(LogMessage::Level::Success, x)