/*!
@file        glapp.cpp
@author      Renfred Lee
@author      Ranvitha
@author      Shahrin Idzuan
@co-author   parminder.singh@digipen.edu
@date        05/05/2025

This file implements functionality useful and necessary to build OpenGL
applications including use of external APIs such as GLFW to create a
window and start up an OpenGL context and to extract function pointers
to OpenGL implementations. It also includes new functions for ImGui
integration.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*//*__________________________________________________________________________*/

/* _________________________________________________________________________
----------------------------------------------------------------------------- */

#include "glapp.h"
#include "rendering.h"
#include "camera.h"
#include "entitymanager.h"
#include "engine.h"
#include "shapes.h"
#include "textureasset.h"
#include <glm/glm.hpp>
#include <iostream>      
#include <sstream>
#include <iomanip>
#include <array>
#include "assetmanager.h"

// ImGui headers for callbacks
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <algorithm>
#include "frameratecontroller.h"
#include "key.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// --- Custom Window Procedure Hook to disable drag-resizing ---
static WNDPROC originalWndProc = nullptr;

static LRESULT CALLBACK EngineWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_NCHITTEST) {
        LRESULT hit = CallWindowProc(originalWndProc, hwnd, uMsg, wParam, lParam);

        // If the OS says the mouse is on a resizing edge or corner, lie to it!
        if (hit == HTLEFT || hit == HTRIGHT || hit == HTTOP || hit == HTBOTTOM ||
            hit == HTTOPLEFT || hit == HTTOPRIGHT || hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT) {
            return HTBORDER; // Tells Windows it's just a static border, not grabbable
        }
        return hit;
    }
    return CallWindowProc(originalWndProc, hwnd, uMsg, wParam, lParam);
}
// -------------------------------------------------------------
#endif

// Before asking GLFW to create an OpenGL context, we specify the minimum constraints
// in that context:
constexpr int GL_MINOR{ 5 };
constexpr int GL_MAJOR{ 4 };

/* _________________________________________________________________________
----------------------------------------------------------------------------- */

GLint GLApp::width;
GLint GLApp::height;
GLdouble GLApp::fps;
GLdouble GLApp::delta_time;
std::string GLApp::title;
GLFWwindow* GLApp::ptr_window{ nullptr };

Component::Rendering model;
Component::Shapes shapes;
//Component::TextureAsset texture;

std::array<CEVec3, 4> clr_vtx{
    CEVec3(1.f, 1.f, 1.f), // Center - white
    CEVec3(1.f, 0.f, 0.f), // Bottom-right - red
    CEVec3(0.f, 1.f, 0.f), // Top-right - green
    CEVec3(0.f, 0.f, 1.f)  // Top-left - blue
};

std::array<CEVec3, 4> InitVtx = clr_vtx;

std::vector<CEVec3> current_positions;
std::vector<CEVec3> current_colors;
std::vector<GLApp::GLViewport> GLApp::vps;

// --- Initialize Static Variables ---
bool GLApp::is_fullscreen = false;
bool GLApp::is_maximized = false;
int GLApp::windowed_x = 0;
int GLApp::windowed_y = 0;
int GLApp::windowed_w = 1280;
int GLApp::windowed_h = 720;


/* _________________________________________________________________________ */
/*! init

@param GLint width
@param GLint height
Dimensions of window requested by program

@param std::string title_str
String printed to window's title bar

@return bool
true if OpenGL context and GLEW were successfully initialized.
false otherwise.
*/
bool GLApp::init(GLint w, GLint h, std::string t) {
    GLApp::width = w;
    GLApp::height = h;
    GLApp::title = t;

    GLApp::vps.resize(4);

    if (!glfwInit()) {
        LOGE(Core, "GLFW init has failed - abort program!!!");
        return false;
    }
    // In case a GLFW function fails, an error is reported to callback function
    glfwSetErrorCallback(GLApp::error_cb);

    // specify OpenGL version 4.5
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_MINOR);
    // specify modern OpenGL only - no compatibility with "old" OpenGL
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // applications will be double-buffered ...
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    // default behavior: colorbuffer is 32-bit RGBA, depthbuffer is 24-bits

    // KEEP THIS TRUE so the maximize button works
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    // Keep the window hidden until the engine has submitted its first valid frame.
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

#if !USE_IMGUI_EDITOR
    // Force 1920x1080 default for Standalone Game
    w = 1920;
    h = 1080;
#endif

    GLApp::ptr_window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);

    if (!GLApp::ptr_window) {
        LOGE(Core, "GLFW create window failed");
        glfwTerminate();
        return false;
    }
    glfwSetWindowAttrib(GLApp::ptr_window, GLFW_DECORATED, GLFW_TRUE);

    // CENTER THE WINDOW FIRST (For when it is un-maximized)
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int startX = (mode->width - w) / 2;
    int startY = (mode->height - h) / 2;
    glfwSetWindowPos(GLApp::ptr_window, startX, startY);

    // Store initial windowed state
    windowed_x = startX;
    windowed_y = startY;
    windowed_w = w;
    windowed_h = h;

    // make the previously created OpenGL context current ...
    glfwMakeContextCurrent(GLApp::ptr_window);

    // --- Inject our Windows hook here ---
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(GLApp::ptr_window);
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(BLACK_BRUSH));
    originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)EngineWindowProc);
#endif
    // ------------------------------------

    glfwSetDropCallback(ptr_window, [](GLFWwindow*, int count, const char** paths) {
        namespace fs = std::filesystem;

        // Use the current Game's asset folder as the destination for drops
        const fs::path assetsRoot = fs::path(GetGameRoot()) / "assets";

        auto route = [&](const fs::path& p)->fs::path {
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            fs::path sub = assetsRoot;
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") sub /= "images";
            else if (ext == ".vert" || ext == ".frag" || ext == ".glsl")                               sub /= "shaders";
            else if (ext == ".json" || ext == ".scene" || ext == ".prefab")                            sub /= "scenes";
            else if (ext == ".bank" || ext == ".mp3" || ext == ".ogg" || ext == ".wav" || ext == ".flac") sub /= "audio";
            else sub /= "misc";
            return sub;
            };

        auto copy_entry = [&](const fs::path& src) {
            if (!fs::exists(src) || !fs::is_regular_file(src)) return;
            std::error_code ec;
            fs::path dstDir = route(src);
            fs::create_directories(dstDir, ec);
            fs::copy_file(src, dstDir / src.filename(), fs::copy_options::overwrite_existing, ec);
            };

        for (int i = 0; i < count; ++i) {
            fs::path dropped(paths[i]);
            if (!fs::exists(dropped)) continue;

            if (fs::is_directory(dropped)) {
                for (auto& e : fs::recursive_directory_iterator(dropped))
                    if (e.is_regular_file()) copy_entry(e.path());
            }
            else {
                copy_entry(dropped);
            }
        }
        using namespace CrashoutsEngine::Assets;
        GenerateAssetManifest(assetManifestPath);
        AssetManager::GetInstance().LoadManifestAssets(assetManifestPath);
        });

    // set callback for events associated with window size changes; keyboard;
    // mouse buttons, cursor position, and scroller
    glfwSetFramebufferSizeCallback(GLApp::ptr_window, GLApp::fbsize_cb);
    glfwSetKeyCallback(GLApp::ptr_window, GLApp::key_cb);
    glfwSetMouseButtonCallback(GLApp::ptr_window, GLApp::mousebutton_cb);
    glfwSetCursorPosCallback(GLApp::ptr_window, GLApp::mousepos_cb);
    glfwSetScrollCallback(GLApp::ptr_window, GLApp::mousescroll_cb);

    // this is the default setting ...
    glfwSetInputMode(GLApp::ptr_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // initialize OpenGL (and extension) function loading library
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Unable to initialize GLEW - error: "
            << glewGetErrorString(err) << " abort program" << std::endl;
        return false;
    }
    std::cout << "Using glew version: " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    const GLubyte* glVersion = glGetString(GL_VERSION);
    std::cout << "OpenGL Version String: " << glVersion << std::endl;

    int major, minor;
    if (sscanf(reinterpret_cast<const char*>(glVersion), "%d.%d", &major, &minor) != 2) {
        std::cerr << "Failed to parse OpenGL version string." << std::endl;
        return false;
    }

    if (major == GL_MAJOR && minor == GL_MINOR) {
        std::cout << "OpenGL 4.5 is supported." << std::endl;
    }
    else {
        std::cerr << "OpenGL 4.5 is not supported." << std::endl;
        return false;
    }

    // we'll use the entire window as viewport ...
    glm::ivec2 vp_width_height(0, 0);
    glfwGetFramebufferSize(ptr_window, &vp_width_height.x, &vp_width_height.y);
    GLApp::fbsize_cb(ptr_window, vp_width_height.x, vp_width_height.y);

    // Enable transparency/blending for sprites
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth testing but allow transparent objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

#ifdef USE_IMGUI_EDITOR

    glGenFramebuffers(1, &gameFboID);
    glBindFramebuffer(GL_FRAMEBUFFER, gameFboID);

    // Color Texture
    glGenTextures(1, &gameTextureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, gameTextureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gameTextureWidth, gameTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gameTextureColorBuffer, 0);

    // Depth / Stencil Renderbuffer
    glGenRenderbuffers(1, &gameRboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, gameRboDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gameTextureWidth, gameTextureHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gameRboDepthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE(Core, "Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

#endif

    return true;
}

void GLApp::ToggleFullscreen() {
    if (is_fullscreen) {
        // Switch TO Windowed Mode
        glfwSetWindowMonitor(ptr_window, nullptr, windowed_x, windowed_y, windowed_w, windowed_h, 0);
        is_fullscreen = false;
    }
    else {
        // Switch TO Fullscreen
        // 1. Save current windowed state so we can restore it later
        glfwGetWindowPos(ptr_window, &windowed_x, &windowed_y);
        glfwGetWindowSize(ptr_window, &windowed_w, &windowed_h);

        // 2. Get primary monitor info
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // 3. Switch to borderless fullscreen using the monitor's current resolution/refresh rate
        glfwSetWindowMonitor(ptr_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        is_fullscreen = true;
    }
    // Force VSync off again if lost during switch
    glfwSwapInterval(0);
}

void GLApp::ToggleMaximize() {
    // Maximizing only makes sense in windowed mode
    if (is_fullscreen) return;

    if (is_maximized) {
        glfwRestoreWindow(ptr_window);
        is_maximized = false;
    }
    else {
        glfwMaximizeWindow(ptr_window);
        is_maximized = true;
    }
}

void GLApp::Minimize() {
    glfwIconifyWindow(ptr_window);
}

void GLApp::Show() {
    if (!ptr_window) return;
    glfwShowWindow(ptr_window);
#if USE_IMGUI_EDITOR
    glfwMaximizeWindow(ptr_window);
    is_maximized = true;
#endif
    glfwFocusWindow(ptr_window);
}

/* _________________________________________________________________________ */
/*! Update

@param none

@return none
*/
void GLApp::Update() {
    // write window title with current fps ...
    std::stringstream sstr;
    sstr << std::fixed << std::setprecision(2) << GLApp::title;
    glfwSetWindowTitle(GLApp::ptr_window, sstr.str().c_str());

    GLint w{ GLApp::width }, h{ GLApp::height };
    static GLint old_w{}, old_h{};
    if (w != old_w || h != old_h) {
        GLint hs{ h / 3 }, ws{ hs };
        GLApp::vps[0] = { 0, 0, w - ws, h };
        GLApp::vps[1] = { w - ws, 2 * hs, ws, hs };
        GLApp::vps[2] = { w - ws, hs, ws, hs };
        GLApp::vps[3] = { w - ws, 0, ws, hs };
        old_w = w;
        old_h = h;
    }
}

/* _________________________________________________________________________ */
/*! draw

@param none

@return none
*/
void GLApp::draw() {
    // Empty for now, logic handled in Scene::RenderScene
}

/* _________________________________________________________________________ */
/*! cleanup

@param none

@return none
*/
void GLApp::cleanup() {
    Component::Rendering::Cleanup();
    Component::TextureRenderer::Cleanup();

#ifdef USE_IMGUI_EDITOR
    if (gameFboID != 0) {
        glDeleteFramebuffers(1, &gameFboID);
        gameFboID = 0;
    }
    if (gameTextureColorBuffer != 0) {
        glDeleteTextures(1, &gameTextureColorBuffer);
        gameTextureColorBuffer = 0;
    }
    if (gameRboDepthStencil != 0) {
        glDeleteRenderbuffers(1, &gameRboDepthStencil);
        gameRboDepthStencil = 0;
    }
#endif

    glfwTerminate();
}

/* _________________________________________________________________________*/
/*! key_cb

@param GLFWwindow*
Handle to window that is receiving event

@param int
the keyboard key that was pressed or released
*/
void GLApp::key_cb(GLFWwindow* pwin, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mod) {
    (void)pwin;

    // Ignore invalid keys
    if (key < 0 || key > GLFW_KEY_LAST) return;

#ifdef _DEBUG
    Key k = static_cast<Key>(key);

    if (action == GLFW_PRESS) {
        LOGI(Input, std::string(KeyToString(k)) + " is pressed");
    }
    else if (action == GLFW_REPEAT) {
        LOGI(Input, std::string(KeyToString(k)) + " is repeatedly pressed");
    }
    else if (action == GLFW_RELEASE) {
        LOGI(Input, std::string(KeyToString(k)) + " is released");
    }
#endif

    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_F11: ToggleFullscreen(); break;
        //case GLFW_KEY_F10: ToggleMaximize(); break;
        //case GLFW_KEY_F9:  Minimize(); break;
    default: break; // ignore all other keys
    }
}

/* _________________________________________________________________________*/
/*! mousebutton_cb
*/
void GLApp::mousebutton_cb([[maybe_unused]] GLFWwindow* pwin, [[maybe_unused]] int button, [[maybe_unused]] int action, [[maybe_unused]] int mod) {
#ifdef DEBUG
    Key k = static_cast<Key>(button);

    if (action == GLFW_PRESS) {
        LOGI(Input, (KeyToString(k) + " is pressed").c_str());
    }
    else if (action == GLFW_RELEASE) {
        LOGI(Input, (KeyToString(k) + " is released").c_str());
    }
#endif
}

/* _________________________________________________________________________*/
/*! mousepos_cb
*/
void GLApp::mousepos_cb([[maybe_unused]] GLFWwindow* pwin, [[maybe_unused]] double xpos, [[maybe_unused]] double ypos) {
#ifdef _DEBUG
    //std::cout << "Mouse cursor position: (" << xpos << ", " << ypos << ")" << std::endl;
#endif
}

/* _________________________________________________________________________*/
/*! mousescroll_cb
*/
void GLApp::mousescroll_cb([[maybe_unused]] GLFWwindow* pwin, [[maybe_unused]] double xoffset, [[maybe_unused]] double yoffset) {
#ifdef _DEBUG
    //std::cout << "Mouse scroll wheel offset: ("
    //    << xoffset << ", " << yoffset << ")" << std::endl;
#endif
}

/* _________________________________________________________________________ */
/*! error_cb
*/
void GLApp::error_cb([[maybe_unused]] int error, char const* description) {
    LOGE(Core, description);
}

/* _________________________________________________________________________ */
/*! fbsize_cb
*/
void GLApp::fbsize_cb([[maybe_unused]] GLFWwindow* ptr_win, int fbWidth, int fbHeight) {
    // 1. Update Global State
    GLApp::width = fbWidth;
    GLApp::height = fbHeight;

#if !USE_IMGUI_EDITOR
    // GAME MODE: Handle Aspect Ratio Resize immediately to prevent visual glitches

    const float targetAspectRatio = 16.0f / 9.0f;
    float currentAspectRatio = (float)fbWidth / (float)fbHeight;

    int vpW, vpH;

    if (currentAspectRatio > targetAspectRatio) {
        // Wide window: Clamp height
        vpH = fbHeight;
        vpW = static_cast<int>(fbHeight * targetAspectRatio);
    }
    else {
        // Tall window: Clamp width
        vpW = fbWidth;
        vpH = static_cast<int>(fbWidth / targetAspectRatio);
    }

    // Update Camera with the CORRECTED aspect ratio dimensions
    using namespace CrashoutsEngine::RenderCamera;
    engineCamera.UpdateMatrices(vpW, vpH);
    gameCamera.UpdateMatrices(vpW, vpH);

#else
    // EDITOR MODE: Just use full size
    using namespace CrashoutsEngine::RenderCamera;
    engineCamera.UpdateMatrices(fbWidth, fbHeight);
#endif
}

/* _________________________________________________________________________*/
/*! update_time
*/
void GLApp::update_time(double fps_calc_interval) {
    // get elapsed time (in seconds) between previous and current frames
    static double prev_time = glfwGetTime();
    double curr_time = glfwGetTime();
    GLApp::delta_time = curr_time - prev_time;
    prev_time = curr_time;

    // fps calculations
    static double count = 0.0; // number of game loop iterations
    static double start_time = glfwGetTime();
    // get elapsed time since very beginning (in seconds) ...
    double elapsed_time = curr_time - start_time;

    ++count;

    // Update fps at least every 10 seconds ...
    fps_calc_interval = (fps_calc_interval < 0.0) ? 0.0 : fps_calc_interval;
    fps_calc_interval = (fps_calc_interval > 10.0) ? 10.0 : fps_calc_interval;
    if (elapsed_time > fps_calc_interval) {
        GLApp::fps = count / elapsed_time;
        start_time = curr_time;
        count = 0.0;
    }
}
