/*!
@file   crashlogger.h
@author Renfred Lee (38%), Mohamad Amir Akmal (38%), Ranvitha (24%)
@date   28/09/2025

@brief  Process-wide crash logger installation entry point.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*!
 @brief Installs the crash logger/handler for the current process.
 @param name Null-terminated application name used for tagging crash outputs.
 @note  Call this as early as possible during startup (before worker threads
        are spawned) to ensure full coverage of failures.
 @warning This function should be called once per process.
*/
void InstallCrashLogger(const char* name);

/*!
 @brief Call this manually at the end of main() to log the exit code
*/
void LogExitCode();
