/*!
@file       main.cpp
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Windows entry point for the Racer game project.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/


// Extension loader library's header must be included before GLFW's header!!!
#pragma warning(push)
#pragma warning(disable: 4005)

#include "engine.h"

#pragma warning(pop)

#define NOMINMAX     // <--- Prevents windows.h from breaking std::min/max
#include <windows.h> // <-- Fixes the WINAPI and HINSTANCE syntax errors!
#include <cstdio>
#include <crtdbg.h>

#include "transformcomponent.h"
#include "performanceViewer.h"
#include "glapp.h"
#include "frameratecontroller.h"
#include "crashlogger.h"

static CrashoutsEngine::Entity globalSelectedEntity = 0;
auto& FRC = FrameRateController::GetInstance();

/*!
@brief Windows application entry point for Racer.
@param hInstance Application instance handle.
@param hPrevInstance Previous instance handle (unused).
@param lpCmdLine Command line string (unused).
@param nCmdShow Show command (unused).
@return Process exit code.
*/
int WINAPI WinMain(
    [[maybe_unused]] _In_     HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_     LPSTR     lpCmdLine,
    [[maybe_unused]] _In_     int       nCmdShow
) {
#if DEBUG
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    SetupConsole();
#endif

    InstallCrashLogger("CrashoutsEngine");

    // --- DYNAMIC ROOT DETECTION ---
    // If we are running the Editor, we need to point back to the source files.
    // If we are running the Standalone Game, we look at our own current directory!
#if USE_IMGUI_EDITOR
    SetGameRoot("../../projects/GameProjects/Racer");
#else
    SetGameRoot(".");
#endif

    Init();
    double fixedDT = FRC.FixedDT();

    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        FRC.FrameStart();
        glfwPollEvents();
        fixedDT = FRC.FixedDT();

        Update(fixedDT);
        Draw(globalSelectedEntity);
        FRC.FrameEnd();
    }

    LogExitCode();
    CleanUp();

    return EXIT_SUCCESS;
}
