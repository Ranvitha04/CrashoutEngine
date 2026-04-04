/*!
@file   log.h
@author Mohamad Amir Akmal (44%), mohamadamirakmal.b (34%), Ranvitha (22%)
@date   14/09/2025

@brief  Lightweight logging utility for Crashout Engine.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include "logdefs.h"
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
/*                                                                  
----------------------------------------------------------------------------- */
/*!
@struct LogMessage
@brief  A single log entry with level and subsystem metadata.
*/
/*                                                       message & enums
----------------------------------------------------------------------------- */
/*!
 @struct LogMessage
 @brief  A single log entry with level and subsystem metadata.
 @details
 Stores the text payload, severity level, subsystem, timestamp, and the
 originating thread ID for diagnostic and filtering purposes.
*/
/** @brief LogMessage */
struct LogMessage {
    std::string text;

    /*! @enum Level
        @brief Severity levels generated from the `LOG_LEVELS` X-macro. */
    enum class Level : unsigned char {
        #define LOG_LEVEL_ITEM(n) n,
        LOG_LEVELS(LOG_LEVEL_ITEM)
        #undef LOG_LEVEL_ITEM
    };

    /*! @enum Subsystem
        @brief Subsystems generated from the `LOG_SYSTEMS` X-macro. */
    enum class Subsystem : unsigned char {
        #define LOG_SYS_ITEM(n) n,
        LOG_SYSTEMS(LOG_SYS_ITEM)
        #undef LOG_SYS_ITEM
    };

    Level level{}; //!< Message severity.
    Subsystem system{}; //!< Source subsystem.

    // Optional metadata for sinks/search/export.
    std::chrono::system_clock::time_point ts{}; //!< Timestamp (system clock).
    std::thread::id tid{}; //!< Originating thread ID.
};

/*!
 @brief  Converts a Level enum to its string name.
 @param  v Severity level.
 @return Null-terminated string representing @p v.
*/
inline const char* ToString(LogMessage::Level v) {
    static const char* k[] = {
        #define X(n) #n,
        LOG_LEVELS(X)
        #undef X
    };
    return k[static_cast<int>(v)];
}

/*!
 @brief  Converts a Subsystem enum to its string name.
 @param  v Subsystem value.
 @return Null-terminated string representing @p v.
*/
inline const char* ToString(LogMessage::Subsystem v) {
    static const char* k[] = {
        #define X(n) #n,
        LOG_SYSTEMS(X)
        #undef X
    };
    return k[static_cast<int>(v)];
}

/*                                                            logger (api)
----------------------------------------------------------------------------- */
/*!
 @class  Logger
 @brief  Thread-safe global logger (singleton).
 @details
 - Use @ref Instance() to access the logger.
 - Messages are appended to an in-memory ring buffer (bounded by capacity).
 - Each message is also printed to the terminal with colored severity tags.
*/
/** @brief Logger */
class Logger {
public:
    /*!
     @brief  Gets the global logger instance.
     @return Reference to the singleton logger.
     @note   Meyers singleton; thread-safe initialization in modern C++.
    */
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    /*!
     @brief  Adds a message to the log and prints it to the terminal.
     @param  lvl Severity level.
     @param  sys Source subsystem.
     @param  msg Message text.
     @details
     - Captures timestamp and thread ID for each message.
     - Appends to the ring buffer; trims oldest entries if capacity is exceeded.
     - Prints to stdout (non-errors) or stderr (errors) with ANSI colors.
    */
    void Add(LogMessage::Level lvl, LogMessage::Subsystem sys, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex);

        LogMessage m{ msg, lvl, sys };
        m.ts = std::chrono::system_clock::now();
        m.tid = std::this_thread::get_id();
        messages.push_back(m);
        if (messages.size() > maxCapacity) {
            messages.erase(messages.begin()); // simple ring behavior
        }

        // terminal colours
        const char* RESET = "\033[0m";
        const char* RED = "\033[31m";
        const char* YEL = "\033[33m";
        const char* GRN = "\033[32m";
        const char* CYN = "\033[36m";

        const char* col =
            (lvl == LogMessage::Level::Error) ? RED :
            (lvl == LogMessage::Level::Warning) ? YEL :
            (lvl == LogMessage::Level::Success) ? GRN : CYN;

        std::ostream& os = (lvl == LogMessage::Level::Error) ? std::cerr : std::cout;
        os << col << "[" << ToString(lvl) << "][" << ToString(sys) << "] "
            << msg << RESET << std::endl;
    }

    /*!
     @brief  Returns a read-only view of the accumulated messages.
     @return Const reference to the message buffer.
     @warning The reference becomes invalidated if the logger mutates the buffer
              (e.g., subsequent calls to @ref Add, @ref Clear, or @ref SetCapacity).
    */
    const std::vector<LogMessage>& Get() const {
        return messages;
    }

    /*!
     @brief  Clears all accumulated log messages.
     @details
     Thread-safe; acquires the logger mutex and removes all entries.
    */
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex);
        messages.clear();
    }

    /*!
     @brief  Sets the maximum message capacity for the in-memory ring buffer.
     @param  max New maximum number of messages to retain.
     @details
     If the current size exceeds @p max, the oldest entries are discarded to
     match the new capacity.
    */
    void SetCapacity(size_t max) {
        std::lock_guard<std::mutex> lock(mutex);
        maxCapacity = max;
        if (messages.size() > maxCapacity) {
            messages.erase(messages.begin(), messages.end() - maxCapacity);
        }
    }

private:
    std::vector<LogMessage> messages;  //!< In-memory message buffer.
    std::mutex mutex;                  //!< Guards access to the buffer.
    size_t maxCapacity = 10000;        //!< Ring buffer capacity (messages).
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