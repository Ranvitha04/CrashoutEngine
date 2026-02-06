/*!
@file    	engine.cpp
@author  	lee.r@digipen.edu
@date    	18/10/2025

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
#include "hierarchycomponent.h" 
#include "prefab.h"
#include "instanceSystem.h"
#include "sceneasset.h"
#include "keyframe.h"
#include "activecomponent.h"
#include "audiocomponent.h"
#include "videosystem.h"

#include "glapp.h"
#include "log.h"
#include "physicsmanager.h"            // Component::RigidBodyComponent
#include "collidereventmanager.h"
#include "inputmanager.h"
#include "debugreflection.h"
#include "textrenderer.h"
#include "rendererconnector.h"
#include "CEColor.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "frameratecontroller.h"
#include "registergamestate.h"
#include "settings.h"
#include <filesystem>

#include "imgui_internal.h"

// For getting folder path in windows
#include "ShlObj_core.h"

/*                                                   type declarations
----------------------------------------------------------------------------- */

// ECS Singletons
CrashoutsEngine::ECS* CrashoutsEngine::ECS::instancePtr = nullptr;
std::mutex CrashoutsEngine::ECS::mtx;

auto& physicsSystem = Physics::PhysicsHandler::GetInstance();
Collision::CollisionHandler& collisionSystem{Collision::CollisionHandler::GetInstance()};
CrashoutsEngine::CollisionEventManager &collisionEvent = CrashoutsEngine::CollisionEventManager::GetInstance();

// ImGui in-game performance viewer
static FrameProfiler gProfiler;

#if USE_IMGUI_EDITOR
EngineState engineState = EngineState::STOP;
static UI::UIState g_uiState;
#else
EngineState engineState = EngineState::PLAY;
#endif

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

std::string g_PendingSceneName = "";
bool g_IsSceneLoadPending = false;
bool g_ShowFPS = false;

#if USE_IMGUI_EDITOR
static EngineState previousState = EngineState::STOP;
#else
static EngineState previousState = EngineState::PLAY;
#endif

bool g_fullscreen = false;

/*  _________________________________________________________________________ */
/*! PerformPendingSceneLoad

@param none

@return none

Call to ensure safe load based off run order
*/
static bool PerformPendingSceneLoad() {
    if (g_IsSceneLoadPending) {

        CrashoutsEngine::Audio::AudioSystem::instance().stopAll(false);

        // Before nuking, destroy scripts
        CrashoutsEngine::ScriptSystem::DestroyAll();

        // Now it is safe to nuke the world
        CrashoutsEngine::ECS::getInstance()->Reset();
        CrashoutsEngine::CollisionEventManager::GetInstance().Clear();
        CrashoutsEngine::Serialization::currSceneName = g_PendingSceneName;
        CrashoutsEngine::Serialization::LoadScenes(g_PendingSceneName);
        
        Scene::InitScene(*CrashoutsEngine::ECS::getInstance());

        // Reset flags
        g_IsSceneLoadPending = false;
        g_PendingSceneName = "";

        // Reset Mono to ensure scripts re-bind correctly
        // (Optional depending on your architecture, but safe)
        // MonoManager::ReloadScripts(); 
#if !USE_IMGUI_EDITOR
        if (Physics::PhysicsHandler::GetInstance().GetPaused())
            Physics::PhysicsHandler::PhysicsHandler::GetInstance().SetPaused(false);
#endif
        return true;
    }
    return false;
}

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
    glfwSwapInterval(0); // disable vsync

    using namespace Messaging;
    namespace fs = std::filesystem;

#if USE_IMGUI_EDITOR
    const fs::path prefabDir = fs::path(CrashoutsEngine::Assets::rootPath) / "prefabs";
#else
    const fs::path prefabDir = fs::path(GetLocalAppData()) / "prefabs";
#endif

    fs::create_directories(prefabDir);
    CrashoutsEngine::ReloadPrefabsFromDisk(prefabDir.generic_string());
    // COLLISION EVENT MANAGER
    // Physical Collisions
    collisionSystem.registerObserver(EventType::CollisionEnter, collisionEvent);
    collisionSystem.registerObserver(EventType::CollisionStay, collisionEvent);
    collisionSystem.registerObserver(EventType::CollisionExit, collisionEvent);

    // Trigger Overlaps
    collisionSystem.registerObserver(EventType::TriggerEnter, collisionEvent);
    collisionSystem.registerObserver(EventType::TriggerStay, collisionEvent);
    collisionSystem.registerObserver(EventType::TriggerExit, collisionEvent);

    // start with a 16:9 aspect ratio
    if (!GLApp::init(Settings::GetScreenWidth(), Settings::GetScreenHeight(), "CRASHOUTS")) {
        std::exit(EXIT_FAILURE);
    }
    glfwSetWindowFocusCallback(GLApp::ptr_window,
        [](GLFWwindow*, int focused)
        {
            if (focused == GLFW_TRUE)
            {
                // Window regained focus
                if (engineState == EngineState::PLAY)
                    CrashoutsEngine::Audio::AudioSystem::instance().setPaused(false);
            }
            else 
            {
#if USE_IMGUI_EDITOR
                // Window lost focus (ALT+TAB)
                if (engineState != EngineState::STOP) engineState = EngineState::PAUSE;
#endif
                CrashoutsEngine::Audio::AudioSystem::instance().setPaused(true);
                InputManager::ClearInputs();
            }
        }
    );

    CrashoutsEngine::Audio::AudioSystem::instance().init(CrashoutsEngine::Assets::rootPath + "audio/");

#if USE_IMGUI_EDITOR
    CrashoutsEngine::Assets::GenerateAssetManifest();
#endif
    CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets();


    //Component::Rendering::Get().Setup_VAO(); // Setup default quad VAO
    Component::Rendering::Initialize(); // Initializes the main shader
    Component::TextureRenderer::Initialize(); // Initializes the texture/sprite shader

	MonoManager::Init("CrashoutDomain");
    MonoManager::LoadAssembly("scripts/ManagedScripts.dll");
    RegisterBuiltinComponentAdapters();
    CrashoutsEngine::InitPrefabs();

    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(SCENE);

    //CrashoutsEngine::Audio::AudioSystem::instance().init();

    CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

    InputManager::Init(GLApp::ptr_window);

    #if USE_IMGUI_EDITOR
    engineState = EngineState::STOP;
    UI::Init(GLApp::ptr_window, g_uiState); 
    PerformanceSystem::Get().Attach(gProfiler);
    #else
    engineState = EngineState::PLAY;
    #endif  

    glEnable(GL_DEPTH_TEST);

    CrashoutsEngine::PostProcessing::InitPP();
    glGenFramebuffers(1, &gameFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);

    // --- Color Texture ---
    glGenTextures(1, &gameTextureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, gameTextureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gameTextureColorBuffer, 0);

    // --- Depth/Stencil Renderbuffer ---
    glGenRenderbuffers(1, &gameRboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, gameRboDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gameTextureWidth, gameTextureHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gameRboDepthStencil);

    // Validate
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR: Game framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    Scene::InitScene(*CrashoutsEngine::ECS::getInstance());
}

/*  _________________________________________________________________________ */
/*! Update

@param none

@return none

Make sure callbacks are invoked when state changes in input devices occur.
Ensure time per frame and FPS are recorded.
Let application Update state changes (such as animation).
*/
void Update(double const& dt) {
    MonoManager::CheckHotReload();
    if(PerformPendingSceneLoad()) return;

    GLApp::update_time();

    UpdateGameStateManager();

    if (CurrentDescriptor->Update) CurrentDescriptor->Update();

    // process events if any associated with input devices
    //glfwPollEvents();

    // start frame profiling
    PerformanceSystem::Get().BeginFrame();

    // main loop computes fps and other time related stuff once for all apps ...
    GLApp::update_time(1.0);
    
    FrameProfiler::Scoped frameScope(PerformanceSystem::Get().GetProfiler(), "Game/Update");
    ScopedMemTag mem(MemTag::Update);

    CrashoutsEngine::ECS& ecs = *CrashoutsEngine::ECS::getInstance();

    CrashoutsEngine::UpdateHierarchy(ecs);

    if (engineState == EngineState::PLAY) {
        KeyframeUnity::UpdateAll(ecs, (float)dt);
        CrashoutsEngine::VideoSystem::Update((float)dt);
        CrashoutsEngine::ScriptSystem::UpdateAll();

        //fixed Update please work
        int steps = FrameRateController::GetInstance().StepCount();
        for (int i = 0; i < steps; ++i)
        {
            {
                FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "ScriptFixedUpdate");
                CrashoutsEngine::ScriptSystem::FixedUpdateAll();
            }
            {
                FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
                physicsSystem.Update(FrameRateController::GetInstance().FixedDT());
            }
        }
        {
        FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Audio");

        // Sync Audio Components (Mute/Unmute based on Active State)
        auto& audioArray = ecs.GetComponentArray<Component::AudioEmitterComponent>();
        for (size_t i = 0; i < audioArray.dense.size(); ++i) {
            CrashoutsEngine::Entity entityID = audioArray.indexToEntity[i];
            auto* audioComp = audioArray.dense[i];

            // Check if entity is active in hierarchy
            bool isEntityActive = true;
            if (ecs.HasComponent<Component::ActiveComponent>(entityID)) {
                isEntityActive = IsEntityActive(ecs, entityID);
            }

            // Loop through ALL clips on this component
            for (auto& clip : audioComp->clips) {

                // Case 1: Entity is Inactive -> Force Pause
                if (!isEntityActive) {
                    if (clip.channel) clip.channel->setPaused(true);
                    if (clip.inst)    clip.inst->setPaused(true);
                }
                // Case 2: Entity is Active -> Unpause (if Game is Playing)
                else {
                    if (engineState == EngineState::PLAY) {
                        // Only unpause if it was paused!
                        if (clip.channel) {
                            bool p; clip.channel->getPaused(&p);
                            if (p) clip.channel->setPaused(false);
                        }
                        if (clip.inst) {
                            bool p; clip.inst->getPaused(&p);
                            if (p) clip.inst->setPaused(false);
                        }
                    }
                }
            }
        }

        CrashoutsEngine::Audio::AudioSystem::instance().Update();
        }
        // process events if any associated with input devices
        //glfwPollEvents();

        // main loop computes fps and other time related stuff once for all apps ...
        GLApp::update_time(1.0);

        //Update 
        //Player::UpdateVelocity(playerRigidBody, moveSpeed);

        // animate scene
        GLApp::Update();
    }
    else if (engineState == EngineState::STOP) {
        {
            FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
            physicsSystem.Update(0.f);
        }

        // Process Editor Camera Input
#if USE_IMGUI_EDITOR
        if(gameWinHovered) CrashoutsEngine::RenderCamera::EditorCameraInput();
        CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(gameTextureWidth, gameTextureHeight);
#else
        CrashoutsEngine::RenderCamera::engineCamera.UpdateMatrices(GLApp::width, GLApp::height);
#endif
    }
    else if (engineState == EngineState::PAUSE) {
        // Freeze gameplay, but still allow editor camera to move
        physicsSystem.Update(0.f);
        InputManager::ClearInputs();
        return;
    }

    // Process Game Camera
    if (engineState != EngineState::STOP) {
        using namespace CrashoutsEngine;
        using namespace Component;

        std::vector<CameraComponent*>& c = ecs.GetComponentArray<CameraComponent>().dense;
        if (c.size() > 0) {
            Entity camEntity = ecs.GetComponentArray<CameraComponent>().indexToEntity.back();
            RenderCamera::PlayerCameraInput(*c.back(), camEntity);
        }

#ifdef USE_IMGUI_EDITOR
        RenderCamera::gameCamera.UpdateMatrices(Settings::GetScreenWidth(), Settings::GetScreenHeight());
#else
        // RELEASE MODE: Use Letterboxed Dimensions for consistent Camera Aspect Ratio
        int windowW = GLApp::width;
        int windowH = GLApp::height;
        const float targetAspectRatio = 16.0f / 9.0f;
        float currentAspectRatio = (float)windowW / (float)windowH;
        int vpW, vpH;

        if (currentAspectRatio > targetAspectRatio) {
            vpH = windowH;
            vpW = static_cast<int>(windowH * targetAspectRatio);
        } else {
            vpW = windowW;
            vpH = static_cast<int>(windowW / targetAspectRatio);
        }
        RenderCamera::gameCamera.UpdateMatrices(vpW, vpH);
#endif
    }

    // Always run Gizmo drawing (Debug shapes)
    CrashoutsEngine::ScriptSystem::DrawGizmosAll();

    Scene::UpdateScene(ecs);

    InputManager::Update();
    
    CrashoutsEngine::PostProcessing::UpdatePP();


    // Cycle and draw Tile Map Editor if needed
    CrashoutsEngine::ECS::ComponentArray<Component::TileMapComponent>& tmArray =
        CrashoutsEngine::ECS::getInstance()->GetComponentArray<Component::TileMapComponent>();

    for (size_t i = 0; i < tmArray.dense.size(); ++i) {
        // 1. Get the Component
        Component::TileMapComponent* tm = tmArray.dense[i];

        // 2. Get the Entity ID associated with this index
        CrashoutsEngine::Entity entityID = tmArray.indexToEntity[i];
        if (tm->isCacheDirty) CrashoutsEngine::TileMapUtils::RebuildCache(*tm, entityID);
    }

}

/*  _________________________________________________________________________ */
/*! draw

@param

@return none

Call application to draw and then swap front and back frame buffers ...
Uses GLHelper::GLFWWindow* to get handle to OpenGL context.
*/
void Draw(CrashoutsEngine::Entity& globalSelectedEntity) {

#if !USE_IMGUI_EDITOR 
    (void)globalSelectedEntity;
#endif

    // --- 1: RENDER THE RAW GAME SCENE ---
    // FOR USE_IMGUI_EDITOR AND NOT USING
    // All game objects, sprites, text, etc., draw into 'gameFboID'.
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);

#if !USE_IMGUI_EDITOR
    int windowW, windowH;
    glfwGetFramebufferSize(GLApp::ptr_window, &windowW, &windowH);

    //const float targetAspectRatio = 16.0f / 9.0f;
    //float currentAspectRatio = (float)windowW / (float)windowH;

    int vpX{ 0 }, vpY{ 0 }, vpW, vpH;

    vpH = windowH;
    vpW = windowW;

    // 4. Clear the FULL window to black (The "Bars")

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 5. Set Viewport to the Game Area & Enable Scissor Test
    glViewport(vpX, vpY, vpW, vpH);
    //glEnable(GL_SCISSOR_TEST);
    //glScissor(vpX, vpY, vpW, vpH);
    //
    //// 6. Clear Game Area to Default Grey
    //glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDisable(GL_SCISSOR_TEST);

    // 7. Update Camera Matrices to match the VIEWPORT size, not window size
    CrashoutsEngine::RenderCamera::gameCamera.UpdateMatrices(vpW, vpH);

    // If the window size differs from our internal texture size
    if (vpW != gameTextureWidth || vpH != gameTextureHeight) {
        gameTextureWidth = vpW;
        gameTextureHeight = vpH;

        // --- RESIZE THE TEXTURE MEMORY ---

        // 1. Resize the Color Texture
        glBindTexture(GL_TEXTURE_2D, gameTextureColorBuffer);
        // Re-allocate memory with the new width/height (NULL means we don't copy data, just alloc)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        // 2. Resize the Depth/Stencil Buffer (Use RBO or Texture depending on your setup)
        // Based on your variables, you likely use 'gameRboDepthStencil'
        if (gameRboDepthStencil != 0) {
            glBindRenderbuffer(GL_RENDERBUFFER, gameRboDepthStencil);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gameTextureWidth, gameTextureHeight);
        }

        // Unbind to be safe
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Note: We don't need to re-attach them to the gameFboID usually, 
        // as the FBO holds references to the IDs, which haven't changed.
    }
#else

    glViewport(0, 0, gameTextureWidth, gameTextureHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#endif

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

        // Flush Debug Queue (Lines/Circles from Scripts)
        Component::Rendering::Get().FlushDebugQueue();

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
    /*ImVec2 dockPos = ImVec2(viewport->Pos.x, viewport->Pos.y + titleBarHeight);
    ImVec2 dockSize = ImVec2(viewport->Size.x, viewport->Size.y - titleBarHeight);*/
    ImVec2 dockPos = viewport->Pos;
    ImVec2 dockSize = viewport->Size;
    /*ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);*/
    ImGui::SetNextWindowPos(dockPos);
    ImGui::SetNextWindowSize(dockSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    // Begin Dockspace window (needs ImGuiWindowFlags_MenuBar for the menu)
    bool dockspaceVisible = ImGui::Begin("Dockspace", nullptr,
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);

    if (dockspaceVisible == true) {
        ImGui::PopStyleVar(3);

        // Render the menu bar *inside* the Dockspace Begin/End
        //UI::RenderDebugMenuBar(g_uiState);

        // Define the actual dockspace area
        ImGuiID dockspace_id = ImGui::GetID("Dockspace");
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        // Render ImGui UI on top of the scene
        glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);
        {
            FrameProfiler::Scoped s2(PerformanceSystem::Get().GetProfiler(), "UI");
            UI::Render(gProfiler, globalSelectedEntity, g_uiState);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Cycle and draw Tile Map Editor if needed
        CrashoutsEngine::ECS::ComponentArray<Component::TileMapComponent>& tmArray =
            CrashoutsEngine::ECS::getInstance()->GetComponentArray<Component::TileMapComponent>();

        for (Component::TileMapComponent* tm : tmArray.dense) {
            CrashoutsEngine::Editor::DrawTileMapEditor(tm, &tm->pOpen);
        }
    }
    else {
        ImGui::PopStyleVar(3);
    }


    ImGui::End(); // End the Dockspace window begun earlier

    // End the ImGui frame and render draw data
    UI::EndFrame();


#else
    CEVec3 mouseWorld{ Scene::GetMouseInWorld() };
    InputManager::g_mouseWorldPos = CEVec2{ mouseWorld.x, mouseWorld.y };

    // Render directly to screen
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(GLApp::ptr_window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight); // Use main window size

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    //CrashoutsEngine::PostProcessing::RenderPP(gameTextureColorBuffer);

#endif
    // APPLY POST_PROCESSING TO FBO 

    // Bind FBO to get post-processed image
    glBindFramebuffer(GL_FRAMEBUFFER, CrashoutsEngine::PostProcessing::ppFboID);
    glViewport(0, 0, gameTextureWidth, gameTextureHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    CrashoutsEngine::PostProcessing::RenderPP(gameTextureColorBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --- FPS Counter ---
#if !USE_IMGUI_EDITOR && !defined(DEBUG)
    if (g_ShowFPS)
    {
        int fpsWindowW, fpsWindowH;
        glfwGetFramebufferSize(GLApp::ptr_window, &fpsWindowW, &fpsWindowH);
        glViewport(0, 0, fpsWindowW, fpsWindowH);

        if (!gTextRenderer) gTextRenderer = std::make_unique<TextRenderer>();

        double fps = FrameRateController::GetInstance().FPS();
        std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));

        // Render Green Text at Top Left
        float xPos = 20.0f;
        float yPos = (float)fpsWindowH - 150.0f;

        gTextRenderer->RenderText(fpsText, xPos, yPos, 1.0f, 10.0f, CEColor(0.0f, 1.0f, 0.0f, 1.0f), "AlegreyaSC-Regular", false, 0.0f);
    }
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
/*! CleanUp

@param none

@return none

Release resources
*/
void CleanUp() {

    if (!CrashoutsEngine::Serialization::currSceneName.empty() && engineState == EngineState::STOP)
        CrashoutsEngine::Serialization::SaveScenes(CrashoutsEngine::Serialization::currSceneName);
    gProfiler.Clear(); 
    Prefab::ClearRegistry();

    // 1. STOP LOGIC & SCRIPTS (So they don't try to play sounds/move things)
    CrashoutsEngine::ScriptSystem::DestroyAll();
    MonoManager::Shutdown(); // Stop C# scripts before killing the ECS
    ShutdownGameStateManager();
    InputManager::Shutdown();

    // 2. STOP SYSTEMS (Audio/Rendering need ECS to be alive to clean up their components)
    CrashoutsEngine::Audio::AudioSystem::instance().shutdown(); 
    InstancingSystem::ShutdownInstancing();                     
    Component::Rendering::Cleanup();    
    Component::TextureRenderer::Cleanup();
    CrashoutsEngine::PostProcessing::ShutdownPP();
    collisionSystem.Shutdown();
    CrashoutsEngine::CollisionEventManager::GetInstance().Clear();

    // 3. SHUTDOWN UI (Depends on everything)
    DebugReflection::ComponentRegistry::instance().clear();
    DebugReflection::PanelRegistry::instance().clear();
    DebugReflection::PhysicsRegistry::instance().clear();
#if USE_IMGUI_EDITOR
    UI::Shutdown(g_uiState);
    Logger::Instance().Clear();
#endif

    // 4. DESTROY ECS (Now safe, because no systems are trying to access it)
    CrashoutsEngine::ECS::DestroyInstance();

    // 5. CLEANUP RESOURCES & WINDOW (Low level stuff)
    CrashoutsEngine::Assets::AssetManager::GetInstance().Shutdown();
    GLApp::cleanup();
}

// Game path offset
const std::string gamePathOffset = "/Digipen/Descent/";

/*
/*! GetUserDocFolder

@return String to LocalAppData Folder

Retrieves the 'LocalAppData' folder of the user
*/
std::string GetLocalAppData() {
    // Pointer to wide string
    PWSTR ppszPath = NULL;  // NULL terminated
    std::string path = "";
    
    HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &ppszPath);
    if (SUCCEEDED(result) && ppszPath) {
        // Conversion to string
        int requiredSize = WideCharToMultiByte(
            CP_UTF8,       // Code page to use for the output string (UTF-8 is recommended)
            0,             // dwFlags
            ppszPath,      // Wide string input
            -1,            // Input length (-1 for null-terminated string)
            NULL,          // Output buffer (NULL to get size)
            0,             // Size of output buffer (0)
            NULL,          // lpDefaultChar
            NULL           // lpUsedDefaultChar
        );

        if (requiredSize > 0) {
            // Resize the string and perform conversion
            path.resize(requiredSize);

            WideCharToMultiByte(
                CP_ACP,
                0,
                ppszPath,
                -1,
                &path[0],
                requiredSize,
                NULL,
                NULL
            );

            path.resize((size_t)(requiredSize - 1));
        }
    }

    // Free mem allocated 
    if (ppszPath) CoTaskMemFree(ppszPath);
    return path + gamePathOffset;
}