/*!
@file    	main.cpp
@author  	lee.r@digipen.edu
@date    	17/09/2025

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Extension loader library's header must be included before GLFW's header!!!
#pragma warning(push)
#pragma warning(disable: 4005)

#include "engine.h"

#pragma warning(pop)

#include <cstdio>
#include <crtdbg.h>

#include "transformcomponent.h"
#include "performanceViewer.h"

#include "glapp.h"

#include "frameratecontroller.h"
#include "crashlogger.h"

/*                                                   type declarations
----------------------------------------------------------------------------- */

//static CrashoutsEngine::ECS* globalECS = nullptr;
static CrashoutsEngine::Entity globalSelectedEntity = 0;

// frame rate controller
auto& FRC = FrameRateController::GetInstance();

/*  _________________________________________________________________________ */
/*! main

@param none

@return int

Indicates how the program existed. Normal exit is signaled by a return value of
0. Abnormal termination is signaled by a non-zero return value.
Note that the C++ compiler will insert a return 0 statement if one is missing.
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
    //_CrtMemState stateStart, stateEnd, stateDiff;
    //_CrtMemCheckpoint(&stateStart);

    SetupConsole();
#endif
    InstallCrashLogger("CrashoutsEngine");

    SetGameRoot("../../projects/GameProjects/Descent");

    Init();
    double fixedDT = FRC.FixedDT();

    // window's close flag is set by clicking close widget or Alt+F4
    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        FRC.FrameStart();
        glfwPollEvents();
        fixedDT = FRC.FixedDT();

        Update(fixedDT);
        Draw(globalSelectedEntity);
        FRC.FrameEnd();
    }

    LogExitCode();
    // Cleanup
    CleanUp();

//#if DEBUG
//    _CrtMemCheckpoint(&stateEnd);
//    if (_CrtMemDifference(&stateDiff, &stateStart, &stateEnd)) {
//        OutputDebugString("Memory leaks detected!\n");
//        _CrtMemDumpStatistics(&stateDiff);
//    }
//#endif

    return EXIT_SUCCESS;
}