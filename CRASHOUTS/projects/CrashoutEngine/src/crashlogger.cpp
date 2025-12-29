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
#include <vector>
//---------------------------------------------------------------------------

// Application name used in the crash log
static const char* sApplicationName = "App";

// -- Helpers ---------------------------------------------------------------
/*!
 @brief Get the directory where the current executable resides.
 @return Filesystem path to the folder containing the running executable.
         Falls back to the current working directory if retrieval fails.
*/
static std::string GetExeDir()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

/*!
 @brief Ensure that a `crashreports` folder exists (Win32 API version).
*/
static std::string EnsureCrashLogFolder()
{
#if USE_IMGUI_EDITOR
    std::string dir = GetExeDir() + "\\crashreports";
#else
    std::string dir = GetLocalAppData() + "\\crashreports";
#endif
    // Win32 API to create directory (Returns non-zero on success or if exists)
    CreateDirectoryA(dir.c_str(), NULL);
    return dir;
}

/*!
 @brief Generate a timestamped filename for a crash log.
 @return Full file path for a new crash log in the `crashreports` folder.
         The filename format is `crashlog_YYYYMMDD_HHMMSS.txt`.
*/
static std::string GetCrashLogFilePath()
{
    std::string dir = EnsureCrashLogFolder();

    SYSTEMTIME st;
    GetLocalTime(&st);

    char timeBuffer[64];
    // Manually format: crashlog_YYYYMMDD_HHMMSS.txt
    wsprintfA(timeBuffer, "crashlog_%04d%02d%02d_%02d%02d%02d.txt",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    return dir + "\\" + timeBuffer;
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
        OutputDebugStringA("Failed to open log file\n");
        return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    char timeBuffer[64];
    // Format: [YYYY-MM-DD HH:MM:SS]
    wsprintfA(timeBuffer, "[%04d-%02d-%02d %02d:%02d:%02d]",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::fprintf(logFile, "%s Crash in %s: %s\n", timeBuffer, sApplicationName, message);
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
void LogExitCode()
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

}