/*!
@file       crashlogger.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       06/09/2025

@brief
This file provides a simple crash logger that writes error information
to a text file when the program crashes.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "crashlogger.h"
#include <windows.h>
#include <direct.h>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <string>    // <-- needed for std::string
#include <direct.h>  // <-- for _mkdir
#include <cstdlib>   // <-- for std::_Exit
//---------------------------------------------------------------------------

// Application name used in the crash log
static const char* sApplicationName = "App";

/*!
 @brief Ensure the crashreports folder exists.
*/
static void EnsureCrashLogFolder()
{
    _mkdir("CrashoutEngine/crashreports"); // safe: does nothing if already exists
}

/*!
 @brief Create a timestamped log file path inside crashreports folder.
 @return std::string with the full path to the log file.
*/
static std::string GetCrashLogFilePath()
{
    EnsureCrashLogFolder();

    std::time_t currentTime = std::time(nullptr);
    char timeBuffer[64];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S",
        std::localtime(&currentTime));

    std::string filePath = "CrashoutEngine/crashreports/crashlog_" + std::string(timeBuffer) + ".txt";
    return filePath;
}

/*!
 @brief Writes a crash message into the crash log file.
 @param message String containing the crash message.
*/
static void WriteCrashLog(const char* message)
{
    std::string filePath = GetCrashLogFilePath();

    FILE* logFile = nullptr;
    fopen_s(&logFile, filePath.c_str(), "a");
    if (!logFile) return;

    // Get current time for the log entry
    std::time_t currentTime = std::time(nullptr);
    char timeBuffer[64];
    ctime_s(timeBuffer, sizeof(timeBuffer), &currentTime);
    timeBuffer[strcspn(timeBuffer, "\n")] = 0; // remove newline

    fprintf(logFile, "[%s] Crash in %s: %s\n", timeBuffer, sApplicationName, message);

    fclose(logFile);
}

/*!
 @brief Windows exception handler that logs the exception details.
 @param exceptionInfo Exception pointers provided by Windows.
 @return LONG Execution handler status.
*/
static LONG WINAPI WindowsCrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    char messageBuffer[128];
    sprintf_s(messageBuffer, "Exception code 0x%08X at address 0x%p", static_cast<unsigned>(exceptionInfo->ExceptionRecord->ExceptionCode), exceptionInfo->ExceptionRecord->ExceptionAddress);

    WriteCrashLog(messageBuffer);
    return EXCEPTION_EXECUTE_HANDLER;
}

/*!
 @brief Signal handler for runtime errors like abort or segmentation fault.
 @param signalNumber The signal ID raised by the operating system.
*/
static void SignalCrashHandler(int signalNumber)
{
    char messageBuffer[64];
    sprintf_s(messageBuffer, "Signal %d", signalNumber);

    WriteCrashLog(messageBuffer);
    std::_Exit(1); // exit immediately after logging
}

/*!
 @brief Installs the crash logger hooks for unhandled exceptions and signals.
 @param name The name of the application to include in crash logs.
*/
void InstallCrashLogger(const char* name)
{
    sApplicationName = name;

    // Handle Windows exceptions
    SetUnhandledExceptionFilter(WindowsCrashHandler);

    // Handle standard C runtime signals
    std::signal(SIGABRT, SignalCrashHandler);
    std::signal(SIGSEGV, SignalCrashHandler);

    // Create the folder right away at startup
    EnsureCrashLogFolder();
}