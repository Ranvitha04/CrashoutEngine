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
#include "rendering.h"
#include "assetmanager.h"
#include "monomanager.h"
#include "entitymanager.h"
#include "scriptcomponent.h"
#include "cameracomponent.h"
#include "textureasset.h"
#include "scriptsystem.h"
#include "audioSystem.h"
#include "debugmenu.h"
#include "sceneutils.h"
#include "camera.h"
#include "performanceSystem.h"
#include "performanceViewer.h"

//Testing for grouping
#include "hierarchy.h"   
#include "hierarchy_components.h" 
/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS Singletons
CrashoutsEngine::ECS* CrashoutsEngine::ECS::instancePtr = nullptr;
std::mutex CrashoutsEngine::ECS::mtx;

static Physics::PhysicsHandler physicsSystem;
static Collision::CollisionHandler collisionSystem;
static CrashoutsEngine::AIManager aiSystem;

// ImGui in-game performance viewer
static FrameProfiler gProfiler;

EngineState engineState = EngineState::STOP;

const float moveSpeed = 6.7f;

extern "C" void RegisterBuiltinComponentAdapters();

#if USE_IMGUI_EDITOR
bool gameWinHovered{ false };
#endif

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
    
    // Generate and Load Assets and Manifest
    CrashoutsEngine::Assets::GenerateAssetManifest();
    CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();

    Component::Rendering::Get().Setup_VAO(); // Setup default quad VAO
    Component::Rendering::Initialize(); // Initializes the main shader
    Component::TextureRenderer::Initialize(); // Initializes the texture/sprite shader

    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(MAIN_MENU);

    RegisterBuiltinComponentAdapters();

	MonoManager::Init("CrashoutDomain");
    MonoManager::LoadAssembly("scripts/ManagedScripts.dll");

    CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW
    PerformanceSystem::Get().Attach(gProfiler);
    engineState = EngineState::STOP;
}

/*  _________________________________________________________________________ */
/*! draw

@param 

@return none

Call application to draw and then swap front and back frame buffers ...
Uses GLHelper::GLFWWindow* to get handle to OpenGL context.
*/
void Draw(CrashoutsEngine::Entity& globalSelectedEntity) {

    static UI::UIState uiState;

#ifdef USE_IMGUI_EDITOR
    // 1. Start the Dear ImGui frame
    UI::BeginFrame();

    // 2. Setup Dockspace 
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    // Begin Dockspace window (needs ImGuiWindowFlags_MenuBar for the menu)
    ImGui::Begin("Dockspace", nullptr,
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(3);

    // Render the menu bar *inside* the Dockspace Begin/End
    UI::RenderDebugMenuBar(uiState);

    // Define the actual dockspace area
    ImGuiID dockspace_id = ImGui::GetID("Dockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

#endif

#ifdef USE_IMGUI_EDITOR

    // Render to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);
    glViewport(0, 0, gameTextureWidth, gameTextureHeight);

    // clear colorbuffer with RGBA value in glClearColor ...
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    using namespace CrashoutsEngine::RenderCamera;

    // Camera Related Processes
    //Camera& currentCamera = (engineState == EngineState::PLAY) ? gameCamera : engineCamera;
    // TEST WITH ENGINE CAMERA
    Camera& currentCamera = (engineState == EngineState::STOP) ? engineCamera : gameCamera;
    glm::mat4 view = currentCamera.GetViewMatrix();
    glm::mat4 proj = currentCamera.GetProjectionMatrix();
    
    FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Graphics");
    ScopedMemTag mem(MemTag::Graphics);
    {
        FrameProfiler::Scoped s2(PerformanceSystem::Get().GetProfiler(), "Scene");
        GLApp::draw();
        Scene::RenderScene(*CrashoutsEngine::ECS::getInstance());
    }

    // Render ImGui UI on top of the scene
    {
        FrameProfiler::Scoped s2(PerformanceSystem::Get().GetProfiler(), "UI");
        UI::Render(gProfiler, physicsSystem, globalSelectedEntity, uiState);
    }
    PerformanceSystem::Get().RenderUI();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::End(); // End the Dockspace window begun earlier

    // End the ImGui frame and render draw data
    UI::EndFrame();
#else

    // Render directly to main window
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(GLApp::ptr_window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight); // Use main window size

    // clear colorbuffer with RGBA value in glClearColor ...
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    GLApp::draw();
    Scene::RenderScene(*CrashoutsEngine::ECS::getInstance());

#endif

    // render scene
    // GLApp::draw();

    // end of frame profiling
    PerformanceSystem::Get().EndFrame();

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

    GLApp::update_time();
    UpdateGameStateManager();

	CrashoutsEngine::ScriptSystem::UpdateAll(static_cast<float>(FrameRateController::GetInstance().DT()));
    aiSystem.Update();


    // process events if any associated with input devices
    //glfwPollEvents();

    // start frame profiling
    PerformanceSystem::Get().BeginFrame();

    // main loop computes fps and other time related stuff once for all apps ...
    GLApp::update_time(1.0);
    
    FrameProfiler::Scoped frameScope(PerformanceSystem::Get().GetProfiler(), "Game/Update");
    ScopedMemTag mem(MemTag::Update);
    CrashoutsEngine::UpdateHierarchy(*CrashoutsEngine::ECS::getInstance());

    if (engineState == EngineState::PLAY) {

        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Collision");
            collisionSystem.update(dt);
        }
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
            physicsSystem.update(dt);
        }
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Audio");
            CrashoutsEngine::Audio::AudioSystem::instance().update();
        }

        // process events if any associated with input devices
        //glfwPollEvents();

        // main loop computes fps and other time related stuff once for all apps ...
        GLApp::update_time(1.0);

        //Update 
        Player::UpdateVelocity(playerRigidBody, moveSpeed);

        // animate scene
        GLApp::update();
    }
    else if (engineState == EngineState::STOP) {
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
            physicsSystem.update(0.f);
        }
        // Process Editor Camera Input
        if(gameWinHovered) CrashoutsEngine::RenderCamera::EditorCameraInput();
#ifdef USE_IMGUI_EDITOR
        CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(Settings::GetScreenWidth(), Settings::GetScreenHeight());
#else
        CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(GLApp::width, GLApp::height);
#endif
    }

    // Process Game Camera
    if (engineState != EngineState::STOP) {
        using namespace CrashoutsEngine;

        ECS& ecs = *ECS::getInstance();
        std::vector<CameraComponent> c = ecs.GetComponentArray<CameraComponent>().dense;
        if (c.size() > 0) {
            Entity camEntity = ecs.GetComponentArray<CameraComponent>().indexToEntity.back();
            RenderCamera::PlayerCameraInput(c.back(), camEntity);
        }

#ifdef USE_IMGUI_EDITOR
        RenderCamera::gameCamera.UpdateMatrices(Settings::GetScreenWidth(), Settings::GetScreenHeight());
#else
        RenderCamera::gameCamera.UpdateMatrices(GLApp::width, GLApp::height);
#endif
        Scene::UpdateScene(*CrashoutsEngine::ECS::getInstance());
    }

    InputManager::Update();

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

    MonoManager::Shutdown();
}