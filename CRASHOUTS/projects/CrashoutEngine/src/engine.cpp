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
#include "tagcomponent.h"
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
#include "replaymanager.h"

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

#include "jobsystem.h"
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

std::string g_GameRootPath = "";

void SetGameRoot(const std::string& path) {
    g_GameRootPath = path;

    // 1. Extract the name (e.g., "Descent", "test", or ".") from the path
    std::string gameName = std::filesystem::path(path).filename().string();

    // 2. Determine the display name
    std::string title;

#if USE_IMGUI_EDITOR
    // If the path is empty or just a dot, it's the base engine
    if (gameName.empty() || gameName == ".") {
        title = "CrashoutEditor";
    }
    else {
        title = "CrashoutEditor - " + gameName;
    }
#else
    // For standalone games, just show the game name
    title = (gameName.empty() || gameName == ".") ? "Crashouts Engine" : gameName;
#endif

    // 3. Update the GLFW window title immediately if it's already initialized
    if (GLApp::ptr_window) {
        glfwSetWindowTitle(GLApp::ptr_window, title.c_str());
    }
}

std::string GetGameRoot() {
    return g_GameRootPath;
}

std::string GetStartingSceneName()
{
    std::string manifestPath;
    if (std::filesystem::exists("core-assets")) {
        manifestPath = "assets/json/assetmanifest.json";
    }
    else {
        manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
    }
    std::ifstream ifs(manifestPath);

    if (ifs.is_open()) {
        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (!doc.HasParseError()) {
            if (doc.HasMember("start_scene") && doc["start_scene"].IsString()) {
                return doc["start_scene"].GetString();
            }
            if (doc.HasMember("scenes") && doc["scenes"].IsArray() && doc["scenes"].Size() > 0) {
                const auto& firstScene = doc["scenes"][0];
                if (firstScene.HasMember("name")) {
                    return firstScene["name"].GetString();
                }
            }
        }
    }
    return "StartScene"; // Ultimate fallback
}

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

        CrashoutsEngine::ScriptSystem::InitializeAll(
            MonoManager::GetDomain(),
            MonoManager::GetAssembly()
        );

        // Reset flags
        g_IsSceneLoadPending = false;
        g_PendingSceneName = "";

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

    fs::path prefabDir;
    if (fs::exists("core-assets")) {
        // STANDALONE MODE
        prefabDir = "assets/prefabs";
    }
    else {
        // EDITOR MODE
        prefabDir = fs::path(GetGameRoot()) / "assets" / "prefabs";
    }

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
    std::string gameName = fs::path(GetGameRoot()).filename().string();
    std::string initialTitle;

#if USE_IMGUI_EDITOR
    if (gameName.empty() || gameName == ".") {
        initialTitle = "CrashoutEditor";
    } else {
        initialTitle = "CrashoutEditor - " + gameName;
    }
#else
    initialTitle = (gameName.empty() || gameName == ".") ? "Crashouts Engine" : gameName;
#endif

    // Initialize with the formatted title immediately
    if (!GLApp::init(Settings::GetScreenWidth(), Settings::GetScreenHeight(), initialTitle.c_str())) {
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

    std::string manifestPath;

    if (std::filesystem::exists("core-assets")) {
        // STANDALONE MODE
        CrashoutsEngine::Audio::AudioSystem::instance().init("assets/audio/");
        manifestPath = "assets/json/assetmanifest.json";
        // Do NOT generate manifest. We use the one shipped in the folder.
    }
    else {
        // EDITOR MODE
        CrashoutsEngine::Audio::AudioSystem::instance().init(GetGameRoot() + "/assets/audio/");
        manifestPath = GetGameRoot() + "/assets/json/assetmanifest.json";
        CrashoutsEngine::Assets::GenerateAssetManifest(manifestPath);
    }

    // 3. Load the manifest we just created
    CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets(manifestPath);


    //Component::Rendering::Get().Setup_VAO(); // Setup default quad VAO
    Component::Rendering::Initialize(); // Initializes the main shader
    Component::TextureRenderer::Initialize(); // Initializes the texture/sprite shader

	MonoManager::Init("CrashoutDomain");
    MonoManager::LoadAssembly();
    RegisterBuiltinComponentAdapters();
    CrashoutsEngine::InitPrefabs();

    /// register game states and start with main menu
    RegisterAllStates();
    InitGameStateManager(SCENE);

    CrashoutsEngine::ScriptSystem::InitializeAll(MonoManager::GetDomain(), MonoManager::GetAssembly());

    InputManager::Init(GLApp::ptr_window);
    CrashoutsEngine::JobSystem::Get().Initialize();

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

    // Give the async loader a moment to upload initial textures to the GPU 
    // before the first frame tries to render them.
    std::cout << "[Engine] Waiting for initial asset GPU uploads...\n";
    for (int i = 0; i < 5; ++i) { // Pump it a few times
        CrashoutsEngine::Assets::AssetManager::GetInstance().Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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
    CrashoutsEngine::Assets::AssetManager::GetInstance().Update();
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

    CrashoutsEngine::UpdateActiveHierarchy(ecs);
    CrashoutsEngine::UpdateHierarchy(ecs);

#if !USE_IMGUI_EDITOR 
    bool releaseCameraUpdated = false;
#endif

    if (engineState == EngineState::PLAY) {
#if !USE_IMGUI_EDITOR
        {
            using namespace CrashoutsEngine;
            using namespace Component;

            std::vector<CameraComponent*>& c = ecs.GetComponentArray<CameraComponent>().dense;
            if (c.size() > 0) {
                Entity camEntity = ecs.GetComponentArray<CameraComponent>().indexToEntity.back();
                RenderCamera::PlayerCameraInput(*c.back(), camEntity);
            }

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
            releaseCameraUpdated = true;
        }

        // Update mouse state early for gameplay scripts (Release mode)
        {
            int displayWidth, displayHeight;
            glfwGetFramebufferSize(GLApp::ptr_window, &displayWidth, &displayHeight);

            const float targetAspectRatio = 16.0f / 9.0f;
            float currentAspectRatio = (float)displayWidth / (float)displayHeight;
            int mVpX = 0, mVpY = 0, mVpW = displayWidth, mVpH = displayHeight;

            if (currentAspectRatio > targetAspectRatio) {
                mVpH = displayHeight;
                mVpW = static_cast<int>(displayHeight * targetAspectRatio);
                mVpX = (displayWidth - mVpW) / 2;
            } else if (currentAspectRatio < targetAspectRatio) {
                mVpW = displayWidth;
                mVpH = static_cast<int>(displayWidth / targetAspectRatio);
                mVpY = (displayHeight - mVpH) / 2;
            }

            double mx, my;
            glfwGetCursorPos(GLApp::ptr_window, &mx, &my);
            float localX = static_cast<float>(mx) - mVpX;
            float localY = static_cast<float>(my) - mVpY;
            bool insideVP = (localX >= 0.0f && localX <= mVpW && localY >= 0.0f && localY <= mVpH);

            InputManager::SetGameViewMouse(localX, localY, (float)mVpW, (float)mVpH);

            if (insideVP) {
                float ndcX = (localX / (float)mVpW) * 2.0f - 1.0f;
                float ndcY = 1.0f - (localY / (float)mVpH) * 2.0f;
                auto& cam = CrashoutsEngine::RenderCamera::gameCamera;
                glm::mat4 invProjView = glm::inverse(cam.GetProjectionMatrix() * cam.GetViewMatrix());
                glm::vec4 worldPos = invProjView * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
                InputManager::g_mouseWorldPos = CEVec2{ worldPos.x, worldPos.y };
            }
        }
#endif
        KeyframeUnity::UpdateAll(ecs, (float)dt);
        CrashoutsEngine::VideoSystem::Update((float)dt);
        CrashoutsEngine::ScriptSystem::UpdateAll();

        //fixed Update please work
        int steps = FrameRateController::GetInstance().StepCount();
        for (int i = 0; i < steps; ++i)
        {
            FrameRateController::GetInstance().IncrementFixedTick();
            {
                FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "ScriptFixedUpdate");
                CrashoutsEngine::ScriptSystem::FixedUpdateAll();
            }
            {
                FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Physics");
                physicsSystem.Update(FrameRateController::GetInstance().FixedDT());
            }
            {
                // ---> LateUpdate locked to the physics tick! <---
                FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "ScriptLateUpdate");
                CrashoutsEngine::ScriptSystem::LateUpdateAll();
            }
        }
        {
        FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Audio");

        // Update Listener Position (Follow Player)
        CEVec3 listenerPos = { 0.0f, 0.0f, 0.0f };
        bool foundPlayer = false;
        auto& tagArray = ecs.GetComponentArray<Component::TagComponent>();
        for (size_t i = 0; i < tagArray.dense.size(); ++i) {
            auto it = Component::TagComponent::tags.find("Player");
            if (it != Component::TagComponent::tags.end() && (tagArray.dense[i]->tagMask & it->second)) {

                CrashoutsEngine::Entity pID = tagArray.indexToEntity[i];

                if (ecs.HasComponent<Component::TransformComponent>(pID)) {
                    listenerPos = ecs.GetComponent<Component::TransformComponent>(pID)->position;
                    foundPlayer = true;
                }
                break;
            }
        }
        
        static int debugLogCounter = 0;
        if (debugLogCounter++ % 120 == 0) {
            if (!foundPlayer) LOGW(Audio, "Listener Warning: No entity with 'Player' tag found! 3D Audio may stay at max volume.");
            else {
                std::stringstream ss;
                ss << "Listener Tracking Player at: (" << listenerPos.x << ", " << listenerPos.y << ")";
                LOGI(Audio, ss.str());
            }
        }
        
        // For 2D games, forward is into the screen (0,0,1) and up is (0,1,0)
        CrashoutsEngine::Audio::AudioSystem::instance().SetListenerAttributes(listenerPos, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f });

        // Sync Audio Components (Mute/Unmute based on Active State & Update 3D Positions)
        auto& audioArray = ecs.GetComponentArray<Component::AudioEmitterComponent>();
        for (size_t i = 0; i < audioArray.dense.size(); ++i) {
            CrashoutsEngine::Entity entityID = audioArray.indexToEntity[i];
            auto* audioComp = audioArray.dense[i];

            // Check if entity is active in hierarchy
            bool isEntityActive = true;
            if (ecs.HasComponent<Component::ActiveComponent>(entityID)) {
                isEntityActive = IsEntityActive(ecs, entityID);
            }

            // Get transform for 3D spatial audio
            Component::TransformComponent* transform = nullptr;
            if (ecs.HasComponent<Component::TransformComponent>(entityID)) {
                transform = ecs.GetComponent<Component::TransformComponent>(entityID);
            }

            // Loop through ALL clips on this component
            for (auto& clip : audioComp->clips) {

                // Cleanup stale channel pointer if sound finished
                if (clip.channel) {
                    bool isPlaying = false;
                    clip.channel->isPlaying(&isPlaying);
                    if (!isPlaying) clip.channel = nullptr;
                }

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
                            bool p; clip.inst->setPaused(&p);
                            if (p) clip.inst->setPaused(false);
                        }
                    }

                    // Update 3D Position if applicable
                    if (clip.spatial3D && clip.channel && transform) {
                        CrashoutsEngine::Audio::AudioSystem::instance().SetChannel3DAttributes(clip.channel, transform->position, clip.minDistance, clip.maxDistance);
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
        if (releaseCameraUpdated) {
            // Already updated above for correct input mapping.
        } else {
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
        }
#endif
    }

    // Always run Gizmo drawing (Debug shapes)
    CrashoutsEngine::ScriptSystem::DrawGizmosAll();

    Scene::UpdateScene(ecs);

    InputManager::Update();
    
    // ==========================================
    // REPLAY DEBUG HOTKEYS
    // ==========================================
//#if !USE_IMGUI_EDITOR && !defined(DEBUG)
    // You already have F3 here, let's add the replay controls!
    if (InputManager::IsKeyTriggered(Key::F5)) {
        ReplayManager::Replay::GetInstance().StartRecording();
    }
    if (InputManager::IsKeyTriggered(Key::F6)) {
        ReplayManager::Replay::GetInstance().StopRecording();
    }
    if (InputManager::IsKeyTriggered(Key::F7)) {
        ReplayManager::Replay::GetInstance().StartPlayback();
    }
    if (InputManager::IsKeyTriggered(Key::F8)) {
        ReplayManager::Replay::GetInstance().StopPlayback();
    }
//#endif
    // ==========================================

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
    Component::Rendering::Get().drawCalls = 0;

#if !USE_IMGUI_EDITOR 
    (void)globalSelectedEntity;
#endif

    // --- 1: RENDER THE RAW GAME SCENE ---
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);

#if !USE_IMGUI_EDITOR
    int windowW, windowH;
    glfwGetFramebufferSize(GLApp::ptr_window, &windowW, &windowH);

    // 1. Calculate the 16:9 Aspect Ratio
    const float targetAspectRatio = 16.0f / 9.0f;
    float currentAspectRatio = (float)windowW / (float)windowH;

    int vpX = 0, vpY = 0, vpW = windowW, vpH = windowH;

    if (currentAspectRatio > targetAspectRatio) {
        // Screen is wider than 16:9 (Pillarbox - black bars on left/right)
        vpH = windowH;
        vpW = static_cast<int>(windowH * targetAspectRatio);
        vpX = (windowW - vpW) / 2;
    }
    else {
        // Screen is taller than 16:9 (Letterbox - black bars on top/bottom)
        vpW = windowW;
        vpH = static_cast<int>(windowW / targetAspectRatio);
        vpY = (windowH - vpH) / 2;
    }

    // 2. Update Camera Matrices to match the correctly scaled dimensions
    CrashoutsEngine::RenderCamera::gameCamera.UpdateMatrices(vpW, vpH);

    // 3. Resize internal textures if the actual gameplay window area changed
    if (vpW != gameTextureWidth || vpH != gameTextureHeight) {
        gameTextureWidth = vpW;
        gameTextureHeight = vpH;

        glBindTexture(GL_TEXTURE_2D, gameTextureColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        if (gameRboDepthStencil != 0) {
            glBindRenderbuffer(GL_RENDERBUFFER, gameRboDepthStencil);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gameTextureWidth, gameTextureHeight);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // 4. When drawing to the internal FBO, the viewport is exactly the texture size!
    glViewport(0, 0, gameTextureWidth, gameTextureHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#else
    // EDITOR MODE
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
    // ==========================================
    // 1. EDITOR MODE
    // ==========================================

    // Start ImGui Frame
    UI::BeginFrame();

    // Setup Dockspace 
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 dockPos = viewport->Pos;
    ImVec2 dockSize = viewport->Size;
    ImGui::SetNextWindowPos(dockPos);
    ImGui::SetNextWindowSize(dockSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    bool dockspaceVisible = ImGui::Begin("Dockspace", nullptr,
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);

    if (dockspaceVisible == true) {
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("Dockspace");
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);
        {
            FrameProfiler::Scoped s2(PerformanceSystem::Get().GetProfiler(), "UI");
            UI::Render(gProfiler, globalSelectedEntity, g_uiState);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        CrashoutsEngine::ECS::ComponentArray<Component::TileMapComponent>& tmArray =
            CrashoutsEngine::ECS::getInstance()->GetComponentArray<Component::TileMapComponent>();
        for (Component::TileMapComponent* tm : tmArray.dense) {
            CrashoutsEngine::Editor::DrawTileMapEditor(tm, &tm->pOpen);
        }
    }
    else {
        ImGui::PopStyleVar(3);
    }

    ImGui::End();

    // Now that UI::Render has drawn the debug colliders into the gameFbo,
    // we can safely process the image so ImGui can display it!
    glBindFramebuffer(GL_FRAMEBUFFER, CrashoutsEngine::PostProcessing::ppFboID);
    glViewport(0, 0, gameTextureWidth, gameTextureHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    CrashoutsEngine::PostProcessing::RenderPP(gameTextureColorBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    UI::EndFrame();

#else
    // ==========================================
    // 2. STANDALONE RELEASE MODE
    // ==========================================

    int displayWidth, displayHeight;
    glfwGetFramebufferSize(GLApp::ptr_window, &displayWidth, &displayHeight);

    currentAspectRatio = (float)displayWidth / (float)displayHeight;
    int mVpX = 0, mVpY = 0, mVpW = displayWidth, mVpH = displayHeight;

    if (currentAspectRatio > targetAspectRatio) {
        mVpH = displayHeight;
        mVpW = static_cast<int>(displayHeight * targetAspectRatio);
        mVpX = (displayWidth - mVpW) / 2;
    }
    else if (currentAspectRatio < targetAspectRatio) {
        mVpW = displayWidth;
        mVpH = static_cast<int>(displayWidth / targetAspectRatio);
        mVpY = (displayHeight - mVpH) / 2;
    }

    // Unproject Mouse
    double mx, my;
    glfwGetCursorPos(GLApp::ptr_window, &mx, &my);
    float localX = static_cast<float>(mx) - mVpX;
    float localY = static_cast<float>(my) - mVpY;
    bool insideVP = (localX >= 0.0f && localX <= mVpW && localY >= 0.0f && localY <= mVpH);

    InputManager::SetGameViewMouse(localX, localY, (float)mVpW, (float)mVpH);

    if (insideVP) {
        float ndcX = (localX / (float)mVpW) * 2.0f - 1.0f;
        float ndcY = 1.0f - (localY / (float)mVpH) * 2.0f;
        glm::mat4 invProjView = glm::inverse(proj * view);
        glm::vec4 worldPos = invProjView * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        InputManager::g_mouseWorldPos = CEVec2{ worldPos.x, worldPos.y };
    }

    // --- DRAW TO SCREEN ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Target the main monitor
    glViewport(0, 0, displayWidth, displayHeight);

    // 1. Fill the whole window with black to create the bars
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 2. Set the perfectly centered viewport
    glViewport(mVpX, mVpY, mVpW, mVpH);
    glDisable(GL_DEPTH_TEST);

    // 3. Render the Post Processing image directly into that centered box!
    CrashoutsEngine::PostProcessing::RenderPP(gameTextureColorBuffer);

#endif

    // end of frame profiling
    PerformanceSystem::Get().EndFrame();

    // swap buffers
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
    CrashoutsEngine::JobSystem::Get().Shutdown();
    CrashoutsEngine::Assets::AssetManager::GetInstance().Shutdown();
    GLApp::cleanup();
}

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
    // Free mem allocated 
    if (ppszPath) CoTaskMemFree(ppszPath);

    // --- Generate the path offset dynamically based on the current game! ---
    std::string gameName = std::filesystem::path(GetGameRoot()).filename().string();
    std::string dynamicPathOffset = "/Digipen/" + gameName + "/";

    return path + dynamicPathOffset;
}
