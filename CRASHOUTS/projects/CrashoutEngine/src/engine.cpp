/*!
@file    	engine.cpp
@author  	lee.r@digipen.edu
@date    	18/10/2025

This file uses functionality defined in type GLApp to initialize an OpenGL
context and implement a game loop.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

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
#include "sceneasset.h"
#include "keyframe.h"

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
static CrashoutsEngine::AIManager aiSystem;

// ImGui in-game performance viewer
static FrameProfiler gProfiler;

#if USE_IMGUI_EDITOR
EngineState engineState = EngineState::STOP;
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
bool PerformPendingSceneLoad() {
    if (g_IsSceneLoadPending) {

        CrashoutsEngine::Audio::AudioSystem::instance().stopAll(false);

        // Before nuking, destroy scripts
        CrashoutsEngine::ScriptSystem::DestroyAll();

        // Now it is safe to nuke the world
        CrashoutsEngine::ECS::getInstance()->Reset();
        CrashoutsEngine::CollisionEventManager::GetInstance().Clear();
        CrashoutsEngine::Serialization::currSceneName = g_PendingSceneName;
        CrashoutsEngine::Serialization::LoadScenes(g_PendingSceneName);

        // Reset flags
        g_IsSceneLoadPending = false;
        g_PendingSceneName = "";

        // Reset Mono to ensure scripts re-bind correctly
        // (Optional depending on your architecture, but safe)
        // MonoManager::ReloadScripts(); 
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
    //InstallCrashLogger("CrashoutsEngine");
    glfwSwapInterval(0); // disable vsync
    collisionSystem.registerObserver(Messaging::getCollisionMsgID(), physicsSystem); /*Listen to messages(collisions)*/
    collisionSystem.registerObserver(Messaging::getExitMsgID(), collisionEvent); /*Listen to messages(collisions)*/
    collisionSystem.registerObserver(Messaging::getStayMsgID(), collisionEvent); /*Listen to messages(collisions)*/
    collisionSystem.registerObserver(Messaging::getEnterMsgID(), collisionEvent); /*Listen to messages(collisions)*/

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
                engineState = EngineState::PAUSE;
#endif
                CrashoutsEngine::Audio::AudioSystem::instance().setPaused(true);
                InputManager::ClearInputs();
            }
        }
    );

    // Generate and Load Assets and Manifest

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

    CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

    InputManager::Init(GLApp::ptr_window);
    UI::Init(GLApp::ptr_window); // NEW
    PerformanceSystem::Get().Attach(gProfiler);

    #if USE_IMGUI_EDITOR
    engineState = EngineState::STOP;
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

    static UI::UIState uiState;

    // --- 1: RENDER THE RAW GAME SCENE ---
    // FOR USE_IMGUI_EDITOR AND NOT USING
    // All game objects, sprites, text, etc., draw into 'gameFboID'.
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);

    #if !USE_IMGUI_EDITOR
        int windowW, windowH;
        glfwGetFramebufferSize(GLApp::ptr_window, &windowW, &windowH);

        const float targetAspectRatio = 16.0f / 9.0f;
        float currentAspectRatio = (float)windowW / (float)windowH;

        int vpX, vpY, vpW, vpH;

        // Calculate Viewport (Letterboxing)
        if (currentAspectRatio > targetAspectRatio) {
            // Window is too wide (Black bars on left/right)
            vpH = windowH;
            vpW = static_cast<int>(windowH * targetAspectRatio);
            vpX = (windowW - vpW) / 2;
            vpY = 0;
        }
        else {
            // Window is too tall (Black bars on top/bottom)
            vpW = windowW;
            vpH = static_cast<int>(windowW / targetAspectRatio);
            vpX = 0;
            vpY = (windowH - vpH) / 2;
        }

        // 4. Clear the FULL window to black (The "Bars")

        // 5. Set Viewport to the Game Area & Enable Scissor Test
        // Scissor ensures we don't draw outside the calculated game area
        //glEnable(GL_SCISSOR_TEST);
        //glScissor(vpX, vpY, vpW, vpH);
        glViewport(vpX, vpY, vpW, vpH);

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

    #endif

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
    /*auto DrawTitleButton = [&](const char* label)
        {
            ImFont* mono = ImGui::GetIO().Fonts->Fonts[0]; // first baked font is monospace
            ImGui::PushFont(mono);

            float btnW = 32.0f;
            float btnH = 28.0f;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 40, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 220, 255));

            ImGui::Button(label, ImVec2(btnW, btnH));

            bool clicked = ImGui::IsItemClicked();

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(3);
            ImGui::PopFont();


            return clicked;
        };*/

#if USE_IMGUI_EDITOR
    // 1. Start the Dear ImGui frame
    UI::BeginFrame();
    //goddamn lobotomy
    /*const float titleBarHeight = 30.0f;
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, titleBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));

        ImGui::Begin("##TitleBarOverlay", nullptr, flags);

        // Title text
        ImGui::SetCursorPosY((titleBarHeight - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text("CRASHOUTS ENGINE");

        // Right aligned buttons
        float buttonRightOffset = 10.0f;
        float buttonArea = 3 * 32;    // three 32px buttons
        ImGui::SameLine(ImGui::GetWindowWidth() - buttonArea - buttonRightOffset);

		// minimize
        if (DrawTitleButton("-"))
            glfwIconifyWindow(GLApp::ptr_window);

        ImGui::SameLine(0.0f, 0.0f);

        // fullscreen
        static bool fullscreen = false; // start windowed

        if (DrawTitleButton(fullscreen ? "[ ]" : "[]"))
        {
            GLFWmonitor* mon = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(mon);

            if (!fullscreen)
                glfwSetWindowMonitor(GLApp::ptr_window, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
            else
                glfwSetWindowMonitor(GLApp::ptr_window, NULL, 100, 100, 1600, 900, 0);

            fullscreen = !fullscreen;
        }



        ImGui::SameLine(0.0f, 0.0f);

        //close
        if (DrawTitleButton("X"))
            glfwSetWindowShouldClose(GLApp::ptr_window, true);

		// dragging is stupidly bugged but whatever - Rain
		// YEAHHHHH ITS FIXED
        // Disable dragging in fullscreen
        if (!fullscreen)
        {
            static bool draggingWindow = false;
            static POINT cursorStart;
            static int winStartX, winStartY;

            float buttonsWidth = 3 * 28 + 12;
            ImVec2 titlebarMin = ImGui::GetWindowPos();
            ImVec2 titlebarMax = ImVec2(
                titlebarMin.x + ImGui::GetWindowWidth() - buttonsWidth,
                titlebarMin.y + titleBarHeight
            );

            bool mouseInTitleBar = ImGui::IsMouseHoveringRect(titlebarMin, titlebarMax);
            bool leftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

            // Start drag
            if (mouseInTitleBar && leftDown && !draggingWindow && !ImGui::IsAnyItemHovered())
            {
                draggingWindow = true;

                // Get initial cursor position (WIN32 absolute)
                GetCursorPos(&cursorStart);

                glfwGetWindowPos(GLApp::ptr_window, &winStartX, &winStartY);
            }

            // Dragging
            if (draggingWindow)
            {
                POINT cursorNow;
                GetCursorPos(&cursorNow);

                int dx = cursorNow.x - cursorStart.x;
                int dy = cursorNow.y - cursorStart.y;

                glfwSetWindowPos(GLApp::ptr_window,
                    winStartX + dx,
                    winStartY + dy);

                if (!leftDown)
                    draggingWindow = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }*/

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

        for (Component::TileMapComponent& tm : tmArray.dense) {
            CrashoutsEngine::Editor::DrawTileMapEditor(&tm, &tm.pOpen);
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
    CrashoutsEngine::UpdateHierarchy(*CrashoutsEngine::ECS::getInstance());

    if (engineState == EngineState::PLAY) {
        KeyframeUnity::UpdateAll(*CrashoutsEngine::ECS::getInstance(), (float)dt);
        CrashoutsEngine::ScriptSystem::UpdateAll();

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
    else if (engineState == EngineState::PAUSE) {
        // Freeze gameplay, but still allow editor camera to move
        physicsSystem.update(0.f);
        InputManager::ClearInputs();
        return;
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

}

/*  _________________________________________________________________________ */
/*! CleanUp

@param none

@return none

Release resources
*/
void CleanUp() {
    UI::Shutdown();
    CrashoutsEngine::ScriptSystem::DestroyAll();

    CrashoutsEngine::PostProcessing::ShutdownPP();

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

            path.resize(requiredSize - 1);
        }
    }

    // Free mem allocated 
    if (ppszPath) CoTaskMemFree(ppszPath);
    return path + gamePathOffset;
}