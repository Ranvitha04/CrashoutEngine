/*!
@file       crashlogger.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       06/09/2025

@brief
This file provides a simple crash logger that writes error information
to a text file when the program crashes.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "crashlogger.h"
#include "engine.h"
#include <windows.h>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <string>
#include <cstdlib>
#include <filesystem>
//---------------------------------------------------------------------------

// Application name used in the crash log
static const char* sApplicationName = "App";

// -- Helpers ---------------------------------------------------------------
/*!
 @brief Get the directory where the current executable resides.
 @return Filesystem path to the folder containing the running executable.
         Falls back to the current working directory if retrieval fails.
*/
[[maybe_unused]] static std::filesystem::path GetExeDir()
{
    char exePath[MAX_PATH]{};
    DWORD n = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    if (n == 0 || n == MAX_PATH) {
        // Fallback to current_path() if something very odd happens.
        return std::filesystem::current_path();
    }
    return std::filesystem::path(exePath).parent_path();
}

/*!
 @brief Ensure that a `crashreports` folder exists beside the executable.
 @return Filesystem path to the `crashreports` folder. The folder will be
         created if it does not already exist.
*/
static std::filesystem::path EnsureCrashLogFolder()
{
    std::filesystem::path dir;
    std::error_code ec;

#if USE_IMGUI_EDITOR
    dir = GetExeDir() / "crashreports";
#else
    std::string appDataDir = GetLocalAppData();

    if (appDataDir.empty()) {
        OutputDebugStringA("[CrashLogger] FATAL: Could nto resolve AppData path.\n");
        return {};
    }

    dir = std::filesystem::path(appDataDir) / "crashreports";
#endif
    std::filesystem::create_directories(dir, ec); // harmless if it already exists

    if (ec) {
        OutputDebugStringA("[CrashLogger] FATAL: Failed to create log directory.\n");
        return {};
    }

    return dir;
}

/*!
 @brief Generate a timestamped filename for a crash log.
 @return Full file path for a new crash log in the `crashreports` folder.
         The filename format is `crashlog_YYYYMMDD_HHMMSS.txt`.
*/
static std::string GetCrashLogFilePath()
{
    auto dir = EnsureCrashLogFolder();

    std::time_t currentTime = std::time(nullptr);
    char timeBuffer[64]{};
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d_%H%M%S",
        std::localtime(&currentTime));

    auto path = dir / (std::string("crashlog_") + timeBuffer + ".txt");
    return path.string();
}

// -- Core ------------------------------------------------------------------
/*!
 @brief Write a crash entry to the log file.

 @param message Human-readable description of the crash event to record.
*/
static void WriteCrashLog(const char* message)
{
    std::string filePath = GetCrashLogFilePath();

    FILE* logFile = nullptr;
    fopen_s(&logFile, filePath.c_str(), "a");
    if (!logFile) {
        // Make failures visible in the debugger.
        OutputDebugStringA("[CrashLogger] Failed to open crash log file.\n");
        OutputDebugStringA(filePath.c_str());
        OutputDebugStringA("\n");
        return;
    }

    // Timestamp for the entry
    std::time_t currentTime = std::time(nullptr);
    char timeBuffer[64]{};
    ctime_s(timeBuffer, sizeof(timeBuffer), &currentTime);
    timeBuffer[strcspn(timeBuffer, "\n")] = 0; // strip newline

    std::fprintf(logFile, "[%s] Crash in %s: %s\n", timeBuffer, sApplicationName, message);
    std::fclose(logFile);
}

/*!
 @brief Windows Structured Exception Handler (SEH) callback.
 @param exceptionInfo Pointer to the exception information provided by the operating system.
 @return Always returns `EXCEPTION_EXECUTE_HANDLER` to terminate the program after logging.
*/
static LONG WINAPI WindowsCrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    char messageBuffer[128];
    std::sprintf(messageBuffer, "Exception code 0x%08X at address 0x%p",
        static_cast<unsigned>(exceptionInfo->ExceptionRecord->ExceptionCode),
        exceptionInfo->ExceptionRecord->ExceptionAddress);

    WriteCrashLog(messageBuffer);
    return EXCEPTION_EXECUTE_HANDLER;
}

/*!
 @brief C runtime signal handler for abnormal termination signals.
 @param signalNumber The signal identifier (e.g., SIGABRT, SIGSEGV).
*/
static void SignalCrashHandler(int signalNumber)
{
    char messageBuffer[64];
    std::sprintf(messageBuffer, "Signal %d", signalNumber);

    WriteCrashLog(messageBuffer);
    std::_Exit(1); // exit immediately after logging
}

/*!
 @brief Called automatically on normal exit to record the exit code.
*/
static void OnExit()
{
    DWORD code = 0;
    if (GetExitCodeProcess(GetCurrentProcess(), &code))
    {
        if (code != 0)
        {
            char buf[128];
            std::sprintf(buf, "Process exited with code %lu (0x%lX)", code, code);
            WriteCrashLog(buf);
        }
    }
}

/*!
 @brief Install the global crash logger.

 @param name Application name to include in crash log entries.

 @details
 Sets up both Windows SEH and CRT signal handlers, and ensures that the
 crash log directory exists. This should be called once at the start of
 the program before any risky operations.
*/
void InstallCrashLogger(const char* name)
{
    sApplicationName = name;

    // Windows SEH
    SetUnhandledExceptionFilter(WindowsCrashHandler);

    // CRT signals
    std::signal(SIGABRT, SignalCrashHandler);
    std::signal(SIGSEGV, SignalCrashHandler);

    // Create the folder right away so path problems appear early
    (void)EnsureCrashLogFolder();
    
    // Log exit codes too
    std::atexit(OnExit);
}