/*!
@file    	main.cpp
@author  	pghali@digipen.edu
@co-author  parminder.singh@digipen.edu
@date    	17/04/2024

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Extension loader library's header must be included before GLFW's header!!!
#include "glapp.h"
#include "performanceViewer.h"
#include "managers/entitymanager.h"
#include "log.h"
#include "playerMovement.h"     // Player::UpdateADVelocity
#include "managers/physicsmanager.h"            // Component::RigidBodyComponent
#include "managers/inputmanager.h"
#include "debugmenu.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include "gamestatemanager.h"
#include "registergamestate.h"
#include "settings.h"

/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS Singletons
CrashoutsEngine::ECS* CrashoutsEngine::ECS::instancePtr = nullptr;
std::mutex CrashoutsEngine::ECS::mtx;

// ECS instance and entity storage
static CrashoutsEngine::ECS* globalECS = nullptr;
static std::vector<CrashoutsEngine::Entity> globalEntities;
static CrashoutsEngine::Entity globalSelectedEntity = 0;
static Physics::PhysicsHandler physicsSystem;

// ImGui in-game performance viewer
static FrameProfiler gProfiler;

// tests
static bool gOnce = false;

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
/*  _________________________________________________________________________ */
/*! main

@param none

@return int

Indicates how the program existed. Normal exit is signaled by a return value of
0. Abnormal termination is signaled by a non-zero return value.
Note that the C++ compiler will insert a return 0 statement if one is missing.
*/

#include <Windows.h>

#ifdef DEBUG

int main() {
    globalECS = CrashoutsEngine::ECS::getInstance();

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

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW

    // window's close flag is set by clicking close widget or Alt+F4
    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        //TEMPORARY TO BE REPLACED BY ECS INPUT SYSTEM - Rain
        glfwPollEvents();
        InputManager::Update();
        GLApp::update_time();
        physicsSystem.update(globalEntities, GLApp::delta_time);
        draw();
        UpdateGameStateManager();
        update();

    }

    // Cleanup
    UI::Shutdown();
    GLApp::cleanup();

    return EXIT_SUCCESS;
}

#else

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, int nCmdShow){
    CrashoutsEngine::ECS* globalECS =
        CrashoutsEngine::ECS::getInstance();

    // start with a 16:9 aspect ratio
    if (!GLApp::init(1280, 720, "CRASHOUTS")) {
        std::exit(EXIT_FAILURE);
    }
    init();
    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(MAIN_MENU);

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW

    // window's close flag is set by clicking close widget or Alt+F4
    while (!glfwWindowShouldClose(GLApp::ptr_window)) {
        //TEMPORARY TO BE REPLACED BY ECS INPUT SYSTEM - Rain
        glfwPollEvents();
        InputManager::Update();
        GLApp::update_time();
        physicsSystem.update(globalEntities, GLApp::delta_time);
        draw();
        UpdateGameStateManager();
        update();

    }

    // Cleanup
    UI::Shutdown();
    GLApp::cleanup();

    return 0;
}

#endif




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
    UI::Render(gProfiler, *globalECS, physicsSystem, globalEntities, globalSelectedEntity);


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
    glfwPollEvents();

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

    const double dt = 1.0 / 60.0;
    //playerTransform.position.x += playerRigidBody.velocity.x * static_cast<float>(dt);

    // animate scene
    GLApp::update();
}