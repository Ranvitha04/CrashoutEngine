/*!
@file       log.h
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

@brief      Lightweight logging utility for Crashout Engine.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
/*                                                                  
----------------------------------------------------------------------------- */
/*!
@struct LogMessage
@brief  A single log entry with level and subsystem metadata.
*/
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

/*!
@class  Logger
@brief  Thread-safe global logger (singleton).
@details
Use @ref Instance() to access the logger. Messages are stored in-memory and
also printed to the terminal with colored severity tags.
*/
class Logger {
public:
    /*!
    @brief  Get the global logger instance.
    @return Reference to the singleton logger.
    */
    static Logger& Instance(){
        static Logger instance;
        return instance;
    }
    
    /*!
    @brief  Add a message to the log and print it to the terminal.
    @param  lvl Severity level.
    @param  sys Source subsystem.
    @param  msg Message text.
    */
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

    /*!
    @brief  Read-only access to the accumulated messages.
    @return Const reference to the message buffer.
    */
    const std::vector<LogMessage>& Get() const {
        return messages;
    }
  
private:
    std::vector<LogMessage> messages;
    std::mutex mutex;
};

// LOGI - INFO, LOGW - WARNING, LOGE - ERROR, LOGS - SUCCESS

/*!
@def LOGI(sys, msg)
@brief Convenience macro for info messages.
@param sys Subsystem enum member (e.g., Core, Physics).
@param msg Text to log.
*/
#define LOGI(sys, msg) Logger::Instance().Add(LogMessage::Level::Info,    LogMessage::Subsystem::sys, (msg))

/*!
@def LOGW(sys, msg)
@brief Convenience macro for warnings.
*/
#define LOGW(sys, msg) Logger::Instance().Add(LogMessage::Level::Warning, LogMessage::Subsystem::sys, (msg))

/*!
@def LOGE(sys, msg)
@brief Convenience macro for errors.
*/
#define LOGE(sys, msg) Logger::Instance().Add(LogMessage::Level::Error,   LogMessage::Subsystem::sys, (msg))

/*!
@def LOGS(sys, msg)
@brief Convenience macro for success/ok messages.
*/
#define LOGS(sys, msg) Logger::Instance().Add(LogMessage::Level::Success, LogMessage::Subsystem::sys, (msg))