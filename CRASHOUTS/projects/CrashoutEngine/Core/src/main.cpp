/*!
@file    	main.cpp
@author  	lee.r@digipen.edu
@date    	17/09/2025

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Extension loader library's header must be included before GLFW's header!!!
#include "glapp.h"
#include "performanceViewer.h"
#include "entitymanager.h"
#include "log.h"
#include "playerMovement.h"     // Player::UpdateADVelocity
#include "physicsmanager.h"            // Component::RigidBodyComponent
#include "inputmanager.h"
#include "debugreflection.h"
#include "debugmenu.h"
#include "crashlogger.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "frameratecontroller.h"
#include "gamestatemanager.h"
#include "registergamestate.h"
#include "settings.h"
#include <filesystem>

/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS Singletons
CrashoutsEngine::ECS* CrashoutsEngine::ECS::instancePtr = nullptr;
std::mutex CrashoutsEngine::ECS::mtx;

// ECS instance and entity storage
static CrashoutsEngine::ECS* globalECS = nullptr;
static CrashoutsEngine::Entity globalSelectedEntity = 0;
static Physics::PhysicsHandler physicsSystem;
static Collision::CollisionHandler collisionSystem;


// ImGui in-game performance viewer
static FrameProfiler gProfiler;

// tests
static bool gOnce = false;
// frame rate controller
auto& FRC = FrameRateController::GetInstance();
/*                                                      function declarations
----------------------------------------------------------------------------- */
static void draw();
static void update();
static void init();

/*                                                   objects with file scope
----------------------------------------------------------------------------- */

/*                                                      function definitions
----------------------------------------------------------------------------- */
static Component::RigidBodyComponent  playerRigidBody{};
static Component::TransformComponent  playerTransform{};
extern "C" void RegisterBuiltinComponentAdapters();
/*  _________________________________________________________________________ */
/*! main

@param none

@return int

Indicates how the program existed. Normal exit is signaled by a return value of
0. Abnormal termination is signaled by a non-zero return value.
Note that the C++ compiler will insert a return 0 statement if one is missing.
*/

#include <cstdio>
#include <windows.h>

/*  _________________________________________________________________________ */
/*! SetupConsole

@param none

@return none

Call to create a cmd console for debugging purposes
*/
void SetupConsole() {
    // Create a new window
    if (AllocConsole()) {
        // Redirect standard I/O streams to the new console

        // C/C++ standard input
        // The "CONOUT$" decide is the console screen buffer.
        (void)freopen("CONIN$", "r", stdin);

        // C/C++ standard output
        // The "CONOUT$" decide is the console screen buffer.
        (void)freopen("CONOUT$", "w", stdout);

        // C/C++ standard error
        // The "CONOUT$" decide is the console screen buffer.
        (void)freopen("CONERR$", "w", stderr);

        // Clear the error flags for C++ streams
        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
        std::wcin.clear();
        std::wcin.clear();
    }
}

int WINAPI WinMain(
    [[maybe_unused]] _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPSTR     lpCmdLine,
    [[maybe_unused]] _In_     int       nCmdShow
){
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(2235);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    _CrtMemState stateStart, stateEnd, stateDiff;
    _CrtMemCheckpoint(&stateStart);

    #ifdef DEBUG
    
    SetupConsole();

    #else

    (void)stateStart;
    (void)stateEnd;
    (void)stateDiff;

    #endif

    InstallCrashLogger("CrashoutsEngine");
    glfwSwapInterval(0); // disable vsync
    collisionSystem.registerObserver(Messaging::getCollisionMsgID(), physicsSystem); /*Listen to messages(collisions)*/

    // start with a 16:9 aspect ratio
    if (!GLApp::init(Settings::GetScreenWidth(), Settings::GetScreenHeight(), "CRASHOUTS")) {
        std::exit(EXIT_FAILURE);
    }
    std::cout << Settings::GetScreenWidth()
        << "x"
        << Settings::GetScreenHeight()
        << std::endl;

    init();
    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(MAIN_MENU);

    RegisterBuiltinComponentAdapters();

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW

    // window's close flag is set by clicking close widget or Alt+F4
    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        FRC.FrameStart();
        glfwPollEvents();
        InputManager::Update();
        GLApp::update_time();
        collisionSystem.update(FRC.DT());
        physicsSystem.update(FRC.DT());
        UpdateGameStateManager();
        update();
        draw();

        FRC.FrameEnd();
    }

    // Cleanup
    UI::Shutdown();
    DebugReflection::ComponentRegistry::instance().clear();
    DebugReflection::PanelRegistry::instance().clear();
    DebugReflection::PhysicsRegistry::instance().clear();
    Logger::Instance().Clear();
    GLApp::cleanup();
    InputManager::Shutdown();
    ShutdownGameStateManager();

    CrashoutsEngine::ECS::DestroyInstance();

    _CrtMemCheckpoint(&stateEnd);
    if (_CrtMemDifference(&stateDiff, &stateStart, &stateEnd)) {
        OutputDebugString("Memory leaks detected!\n");
        _CrtMemDumpStatistics(&stateDiff);
    }
    return EXIT_SUCCESS;
}

static void init() {

}

/*  _________________________________________________________________________ */
/*! draw

@param none

@return none

Call application to draw and then swap front and back frame buffers ...
Uses GLHelper::GLFWWindow* to get handle to OpenGL context.
*/
static void draw() {
    FrameProfiler::Scoped s(gProfiler, "Graphics/Scene");
    ScopedMemTag mem(MemTag::Graphics);
    // render scene
    GLApp::draw();

    // Render ImGui UI on top of the scene
    UI::Render(gProfiler, physicsSystem, globalSelectedEntity);


    // end of frame profiling
    gProfiler.end_frame();

    // swap buffers: front <-> back
    // GLApp::ptr_window is handle to window that defines the OpenGL context
    glfwSwapBuffers(GLApp::ptr_window);
}

/*  _________________________________________________________________________ */
/*! update

@param none

@return none

Make sure callbacks are invoked when state changes in input devices occur.
Ensure time per frame and FPS are recorded.
Let application update state changes (such as animation).
*/
static void update() {
    // process events if any associated with input devices
    //glfwPollEvents();

    // start frame profiling
    gProfiler.begin_frame();

    // main loop computes fps and other time related stuff once for all apps ...
    GLApp::update_time(1.0);

    FrameProfiler::Scoped s(gProfiler, "Game/Update");
    ScopedMemTag mem(MemTag::Update);
    // 6 and 7
    const float moveSpeed = 6.7f;
    //Update 
    Player::UpdateVelocity(playerRigidBody, moveSpeed);



    //const double dt = 1.0 / 60.0;
    //playerTransform.position.x += playerRigidBody.velocity.x * static_cast<float>(dt);

    // animate scene
    GLApp::update();
}