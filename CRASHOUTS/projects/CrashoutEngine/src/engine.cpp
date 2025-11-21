/*!
@file    	engine.cpp
@author  	lee.r@digipen.edu
@date    	18/10/2025

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "engine_pch.h"

// Extension loader library's header must be included before GLFW's header!!!
#include "engine.h"
#include "rendering.h"
#include "assetmanager.h"
#include "monomanager.h"
#include "entitymanager.h"
#include "scriptcomponent.h"
#include "cameracomponent.h"
#include "gamestatemanager.h"
#include "tilemapcomponent.h"
#include "ppmanager.h"
#include "textureasset.h"
#include "scriptsystem.h"
#include "audioSystem.h"
#include "debugmenu.h"
#include "sceneutils.h"
#include "camera.h"
#include "performanceSystem.h"
#include "performanceViewer.h"
#include "hierarchy.h"   
#include "hierarchy_components.h" 
#include "prefab_init.h"
#include "instanceSystem.h"

#include "glapp.h"
#include "log.h"
#include "physicsmanager.h"            // Component::RigidBodyComponent
#include "collidereventmanager.h"
#include "aimanager.h"
#include "inputmanager.h"
#include "debugreflection.h"
#include "crashlogger.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "frameratecontroller.h"
#include "registergamestate.h"
#include "settings.h"
#include <filesystem>
/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS Singletons
CrashoutsEngine::ECS* CrashoutsEngine::ECS::instancePtr = nullptr;
std::mutex CrashoutsEngine::ECS::mtx;

auto& physicsSystem = Physics::PhysicsHandler::GetInstance();
static Collision::CollisionHandler collisionSystem;
CrashoutsEngine::CollisionEventManager &collisionEvent = CrashoutsEngine::CollisionEventManager::GetInstance();
static CrashoutsEngine::AIManager aiSystem;

// ImGui in-game performance viewer
static FrameProfiler gProfiler;

EngineState engineState = EngineState::STOP;

const float moveSpeed = 6.7f;

extern "C" void RegisterBuiltinComponentAdapters();

GLuint gameFboID = 0;
//GLuint gameTextureID = 0;
GLuint gameDepthBufferID = 0;
GLuint gameTextureColorBuffer = 0;
GLuint gameRboDepthStencil = 0;
int gameTextureWidth = 800;     // Will be resized
int gameTextureHeight = 600;    // Will be resized

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
    collisionSystem.registerObserver(Messaging::getExitMsgID(), collisionEvent); /*Listen to messages(collisions)*/
    collisionSystem.registerObserver(Messaging::getStayMsgID(), collisionEvent); /*Listen to messages(collisions)*/
    collisionSystem.registerObserver(Messaging::getEnterMsgID(), collisionEvent); /*Listen to messages(collisions)*/

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

    //Component::Rendering::Get().Setup_VAO(); // Setup default quad VAO
    Component::Rendering::Initialize(); // Initializes the main shader
    Component::TextureRenderer::Initialize(); // Initializes the texture/sprite shader

    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(MAIN_MENU);

    RegisterBuiltinComponentAdapters();
    CrashoutsEngine::InitPrefabs();
	MonoManager::Init("CrashoutDomain");
    MonoManager::LoadAssembly("scripts/ManagedScripts.dll");

    CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW
    PerformanceSystem::Get().Attach(gProfiler);
    engineState = EngineState::STOP;

    glEnable(GL_DEPTH_TEST);

    CrashoutsEngine::PostProcessing::InitPP();
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

    // --- 1: RENDER THE RAW GAME SCENE ---
    // FOR USE_IMGUI_EDITOR AND NOT USING
    // All game objects, sprites, text, etc., draw into 'gameFboID'.
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);
    glViewport(0, 0, gameTextureWidth, gameTextureHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // --- Camera Setup ---
    using namespace CrashoutsEngine::RenderCamera;
#if USE_IMGUI_EDITOR
    Camera& currentCamera = (engineState == EngineState::STOP) ? engineCamera : gameCamera;
#else
    Camera& currentCamera = gameCamera;
#endif
    glm::mat4 view = currentCamera.GetViewMatrix();
    glm::mat4 proj = currentCamera.GetProjectionMatrix();

    // --- Profiling & Scene Rendering ---
    FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Graphics");
    ScopedMemTag mem(MemTag::Graphics);
    {
        FrameProfiler::Scoped s2(PerformanceSystem::Get().GetProfiler(), "Scene");
        //GLApp::draw();
        Scene::RenderScene(*CrashoutsEngine::ECS::getInstance());

        if (CurrentDescriptor) {
            //if (CurrentDescriptor->Update) CurrentDescriptor->Update();
            if (CurrentDescriptor->Render) CurrentDescriptor->Render();
        }
    }
    
    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

#if USE_IMGUI_EDITOR
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

    // Render ImGui UI on top of the scene
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);
    {
        FrameProfiler::Scoped s2(PerformanceSystem::Get().GetProfiler(), "UI");
        UI::Render(gProfiler, physicsSystem, globalSelectedEntity, uiState);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Cycle and draw Tile Map Editor if needed
    CrashoutsEngine::ECS::ComponentArray<Component::TileMapComponent>& tmArray = 
        CrashoutsEngine::ECS::getInstance()->GetComponentArray<Component::TileMapComponent>();

    for (Component::TileMapComponent & tm : tmArray.dense) {
        CrashoutsEngine::Editor::DrawTileMapEditor(&tm, &tm.pOpen);
    }

    ImGui::End(); // End the Dockspace window begun earlier

    // End the ImGui frame and render draw data
    UI::EndFrame();

#else

    // Render directly to screen
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(GLApp::ptr_window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight); // Use main window size

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    CrashoutsEngine::PostProcessing::RenderPP(gameTextureColorBuffer);

#endif
    // APPLY POST_PROCESSING TO FBO 

    // Bind FBO to get post-processed image
    glBindFramebuffer(GL_FRAMEBUFFER, CrashoutsEngine::PostProcessing::ppFboID);
    glViewport(0, 0, gameTextureWidth, gameTextureHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    CrashoutsEngine::PostProcessing::RenderPP(gameTextureColorBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
void Update(double const& dt) {

    GLApp::update_time();

    UpdateGameStateManager();

    MonoManager::CheckHotReload();

	CrashoutsEngine::ScriptSystem::UpdateAll();


    if (CurrentDescriptor->Update) CurrentDescriptor->Update();

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

        aiSystem.Update();

        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Collision");
            collisionSystem.update(dt, (Collision::EngineState)engineState);
        }
        //fixed update please work
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "ScriptFixedUpdate");
            CrashoutsEngine::ScriptSystem::FixedUpdateAll();
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
        //Player::UpdateVelocity(playerRigidBody, moveSpeed);

        // animate scene
        GLApp::update();
    }
    else if (engineState == EngineState::STOP) {
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Collision");
            collisionSystem.update(dt, (Collision::EngineState)engineState);
        }
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
            physicsSystem.update(0.f);
        }
        // Process Editor Camera Input
#if USE_IMGUI_EDITOR
        if(gameWinHovered) CrashoutsEngine::RenderCamera::EditorCameraInput();
        CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(gameTextureWidth, gameTextureHeight);
#else
        CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(GLApp::width, GLApp::height);
#endif
    }
    else {
        FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
        physicsSystem.update(0.f);
    }

    // Process Game Camera
    if (engineState != EngineState::STOP) {
        using namespace CrashoutsEngine;
        using namespace Component;

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
    
    CrashoutsEngine::PostProcessing::UpdatePP();

#if USE_IMGUI_EDITOR
    // Cycle and draw Tile Map Editor if needed
    CrashoutsEngine::ECS::ComponentArray<Component::TileMapComponent>& tmArray =
        CrashoutsEngine::ECS::getInstance()->GetComponentArray<Component::TileMapComponent>();

    for (size_t i = 0; i < tmArray.dense.size(); ++i) {
        // 1. Get the Component
        Component::TileMapComponent& tm = tmArray.dense[i];

        // 2. Get the Entity ID associated with this index
        CrashoutsEngine::Entity entityID = tmArray.indexToEntity[i];
        if (tm.isCacheDirty) CrashoutsEngine::TileMapUtils::RebuildCache(tm, entityID);
    }
#endif

}

/*  _________________________________________________________________________ */
/*! CleanUp

@param none

@return none

Release resources
*/
void CleanUp() {
    CrashoutsEngine::PostProcessing::ShutdownPP();

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

    InstancingSystem::ShutdownInstancing();
    Component::Rendering::Cleanup();
    CrashoutsEngine::Audio::AudioSystem::instance().shutdown();
    CrashoutsEngine::Assets::AssetManager::GetInstance().Shutdown();
}
