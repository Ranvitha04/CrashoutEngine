/*!
@file    	engine.cpp
@author  	lee.r@digipen.edu
@date    	18/10/2025

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
// Extension loader library's header must be included before GLFW's header!!!
#include "engine.h"

/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS Singletons
CrashoutsEngine::ECS* CrashoutsEngine::ECS::instancePtr = nullptr;
std::mutex CrashoutsEngine::ECS::mtx;

static Physics::PhysicsHandler physicsSystem;
static Collision::CollisionHandler collisionSystem;

// ImGui in-game performance viewer
static FrameProfiler gProfiler;

extern "C" void RegisterBuiltinComponentAdapters();

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

/*  _________________________________________________________________________ */
/*! init

@param 

@return none

*/
void Init() {
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

    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(MAIN_MENU);

    RegisterBuiltinComponentAdapters();

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW
}

/*  _________________________________________________________________________ */
/*! draw

@param 

@return none

Call application to draw and then swap front and back frame buffers ...
Uses GLHelper::GLFWWindow* to get handle to OpenGL context.
*/
void Draw(CrashoutsEngine::Entity& globalSelectedEntity) {

    FrameProfiler::Scoped s(gProfiler, "Graphics/Scene");
    ScopedMemTag mem(MemTag::Graphics);
    // render scene
    // GLApp::draw();

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
void Update(Component::RigidBodyComponent& playerRigidBody, double const& dt) {

    InputManager::Update();
    GLApp::update_time();
    UpdateGameStateManager();

    collisionSystem.update(dt);
    physicsSystem.update(dt);

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

    // animate scene
    GLApp::update();
}

/*  _________________________________________________________________________ */
/*! CleanUp

@param none

@return none

Release resources
*/
void CleanUp() {
    UI::Shutdown();
    DebugReflection::ComponentRegistry::instance().clear();
    DebugReflection::PanelRegistry::instance().clear();
    DebugReflection::PhysicsRegistry::instance().clear();
    Logger::Instance().Clear();
    GLApp::cleanup();
    InputManager::Shutdown();
    ShutdownGameStateManager();

    CrashoutsEngine::ECS::DestroyInstance();
}