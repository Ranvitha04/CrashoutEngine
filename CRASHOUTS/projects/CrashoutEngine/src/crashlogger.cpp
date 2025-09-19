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
#include "crash_logger.h"
#include <windows.h>
#include <csignal>
#include <cstdio>
#include <ctime>
//---------------------------------------------------------------------------

// Application name used in the crash log
static const char* sApplicationName = "App";

/*!
 @brief Writes a crash message into the crash log file.
 @param message String containing the crash message.
*/
static void WriteCrashLog(const char* message)
{
    FILE* logFile = nullptr;
    fopen_s(&logFile, "crashlog/crashlog.txt", "a");
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
    sprintf_s(messageBuffer, "Exception code 0x%08X at address 0x%p",
        static_cast<unsigned>(exceptionInfo->ExceptionRecord->ExceptionCode),
        exceptionInfo->ExceptionRecord->ExceptionAddress);

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
}