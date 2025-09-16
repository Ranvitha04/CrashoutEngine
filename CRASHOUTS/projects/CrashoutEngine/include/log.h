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
    };
    enum class Subsystem {
        Core,
        Physics,
        Collision,
        Input,
        AI,
        ECS,
        Renderer,
        UI
    };
    Level level;
    Subsystem system;
};


class Logger {
public:
    static Logger& Instance(){
        static Logger instance;
        return instance;
    }

    void Add(LogMessage::Level lvl, LogMessage::Subsystem sys, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        messages.push_back({ msg, lvl, sys });

        // terminal colours
        const char* RESET = "\033[0m"; 
        const char* RED = "\033[31m"; 
        const char* YEL = "\033[33m";
        const char* GRN = "\033[32m"; 
        const char* CYN = "\033[36m";

        const char* lvlTag = (lvl == LogMessage::Level::Error) ? "[Error]" : (lvl == LogMessage::Level::Warning) ? "[Warning]" : (lvl == LogMessage::Level::Success) ? "[Success]" : "[Info]";
        const char* col = (lvl == LogMessage::Level::Error) ? RED : (lvl == LogMessage::Level::Warning) ? YEL : (lvl == LogMessage::Level::Success) ? GRN : CYN;
        std::ostream& os = (lvl == LogMessage::Level::Error) ? std::cerr : std::cout;
        os << col << lvlTag << " " << msg << RESET << std::endl;
    }

    const std::vector<LogMessage>& Get() const {
        return messages;
    }
  
private:
    std::vector<LogMessage> messages;
    std::mutex mutex;
};

// LOGI - INFO, LOGW - WARNING, LOGE - ERROR, LOGS - SUCCESS
#define LOGI(sys, msg) Logger::Instance().Add(LogMessage::Level::Info,    LogMessage::Subsystem::sys, (msg))
#define LOGW(sys, msg) Logger::Instance().Add(LogMessage::Level::Warning, LogMessage::Subsystem::sys, (msg))
#define LOGE(sys, msg) Logger::Instance().Add(LogMessage::Level::Error,   LogMessage::Subsystem::sys, (msg))
#define LOGS(sys, msg) Logger::Instance().Add(LogMessage::Level::Success, LogMessage::Subsystem::sys, (msg))