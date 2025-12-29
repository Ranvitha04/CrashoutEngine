
/*!
@file        glapp.cpp
@author      pghali@digipen.edu
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
#include "ranscene.h"
#include <glm/glm.hpp>
#include <iostream>      
#include <sstream>
#include <iomanip>
#include <array>

// ImGui headers for callbacks
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <algorithm>
#include "assetmanager.h" 
#include "frameratecontroller.h"
#include "key.h"

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

Uses GLFW to create OpenGL context. GLFW's initialization follows from here:
http://www.glfw.org/docs/latest/quick.html
a window of size width x height pixels
and its associated OpenGL context that matches a core profile that is
compatible with OpenGL 4.5 and doesn't support "old" OpenGL, has 32-bit RGBA,
double-buffered color buffer, 24-bit depth buffer and 8-bit stencil buffer
with each buffer of size width x height pixels
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
    // don't change size of window
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    GLApp::ptr_window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);
    glfwMaximizeWindow(GLApp::ptr_window);


    if (!GLApp::ptr_window) {
        LOGE(Core, "GLFW create window failed");
        glfwTerminate();
        return false;
    }
    glfwSetWindowAttrib(GLApp::ptr_window, GLFW_DECORATED, GLFW_TRUE);


    // Center the window initially
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int startX = (mode->width - w) / 2;
    int startY = (mode->height - h) / 2 +32;
    glfwSetWindowPos(GLApp::ptr_window, startX, startY);

    // Store initial windowed state
    windowed_x = startX;
    windowed_y = startY;
    windowed_w = w;
    windowed_h = h;

    // Borderless fullscreen (windowed)
    //glfwSetWindowMonitor(GLApp::ptr_window,NULL,0, 0,mode->width,mode->height,0);
    // Move window to exact fullscreen position (0,0)
    //glfwSetWindowPos(GLApp::ptr_window, 0, 0);

    //// Override engine width/height
    //GLApp::width = mode->width;
    //GLApp::height = mode->height;

    // make the previously created OpenGL context current ...
    glfwMakeContextCurrent(GLApp::ptr_window);
    glfwSetDropCallback(ptr_window, [](GLFWwindow*, int count, const char** paths) {
        namespace fs = std::filesystem;

        const fs::path assetsRoot = CrashoutsEngine::Assets::rootPath;

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
        CrashoutsEngine::Assets::GenerateAssetManifest(
            CrashoutsEngine::Assets::rootPath,
            CrashoutsEngine::Assets::assetManifestPath
        );
        CrashoutsEngine::Assets::AssetManager::GetInstance().LoadManifestAssets(
            CrashoutsEngine::Assets::rootPath,
            CrashoutsEngine::Assets::assetManifestPath
        );
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gameTextureWidth, gameTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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
    //model.Setup_VAO();
    
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

/* _________________________________________________________________________ */
/*! Update

@param none

@return none

For now, there is nothing to do except to write appropriate information
to window title bar.
*/
// method: void GLApp::Update()
void GLApp::Update() {
    // write window title with current fps ...
    std::stringstream sstr;
    sstr << std::fixed << std::setprecision(2) << GLApp::title;// << " | " << FrameRateController::GetInstance().FPS();
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

    // NEW: Update scene per-frame (handles duck movement)
    //UpdateRan();

    // Sprite animation Update logic is now handled in ran.cpp
}

/* _________________________________________________________________________ */
/*! draw

@param none

@return none

For now, there is nothing to do except set the back buffer fill color
as black.
*/
void GLApp::draw() {

    //// Calculate offsets and sizes (can be done here or cached if static)
    //GLsizei position_attribute_size = sizeof(glm::vec3);
    //GLsizei position_data_size = position_attribute_size * 4;
    //GLsizei color_data_offset = position_data_size;
    //GLsizei color_attribute_size = sizeof(glm::vec3);
    //GLsizei color_data_size = color_attribute_size * static_cast<GLsizei>(clr_vtx.size());
    //
    //// Update the color data in the VBO right before drawing
    //glNamedBufferSubData(model.vbo_hdl, color_data_offset,
    //    color_data_size, clr_vtx.data());
}

/* _________________________________________________________________________ */
/*! cleanup

@param none

@return none

For now, there are no resources allocated by the application program.
The only task is to have GLFW return resources back to the system and
gracefully terminate.
*/
void GLApp::cleanup() {

    //Component::TextureAsset::Cleanup();

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

@parm int
Platform-specific scancode of the key

@parm int
GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE
action will be GLFW_KEY_UNKNOWN if GLFW lacks a key token for it,
for example E-mail and Play keys.

@parm int
bit-field describing which modifier keys (shift, alt, control)
were held down

@return none

This function is called when keyboard buttons are pressed.
*/
void GLApp::key_cb(GLFWwindow* pwin, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mod) {
    (void)pwin;

    // Ignore invalid keys
    if (key < 0 || key > GLFW_KEY_LAST) return;

#ifdef _DEBUG
    // Only cast if within valid Key enum range
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

    // Safely delegate to sprite input handler (ignore unknown keys inside function)
    HandleSpriteKeyInput(key, action);

    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_F11: ToggleFullscreen(); break;
    case GLFW_KEY_F10: ToggleMaximize(); break;
    case GLFW_KEY_F9:  Minimize(); break;
        // case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(pwin, GLFW_TRUE); break;
    default: break; // ignore all other keys
    }
}

/* _________________________________________________________________________*/
/*! mousebutton_cb

@param GLFWwindow*
Handle to window that is receiving event

@param int
the mouse button that was pressed or released
GLFW_MOUSE_BUTTON_LEFT and GLFW_MOUSE_BUTTON_RIGHT specifying left and right
mouse buttons are most useful

@parm int
action is either GLFW_PRESS or GLFW_RELEASE

@parm int
bit-field describing which modifier keys (shift, alt, control)
were held down

@return none

This function is called when mouse buttons are pressed.
*/
void GLApp::mousebutton_cb([[maybe_unused]] GLFWwindow* pwin, [[maybe_unused]] int button, [[maybe_unused]] int action, [[maybe_unused]] int mod) {
    //ImGui_ImplGlfw_MouseButtonCallback(pwin, button, action, mod);
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
	/*
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
#ifdef _DEBUG
        //LOGI(Input, "Left Mouse button ");
#endif
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
#ifdef _DEBUG
        //LOGI(Input, "Right Mouse button ");
#endif
        break;
    }
    switch (action) {
    case GLFW_PRESS:
#ifdef _DEBUG
        //LOGI(Input, "is pressed!");
#endif
        break;
    case GLFW_RELEASE:
#ifdef _DEBUG
        //LOGI(Input, "is released!");
#endif
        break;
    }
}
*/

/* _________________________________________________________________________*/
/*! mousepos_cb

@param GLFWwindow*
Handle to window that is receiving event

@param double
new cursor x-coordinate, relative to the left edge of the client area

@param double
new cursor y-coordinate, relative to the top edge of the client area

@return none

This functions receives the cursor position, measured in screen coordinates but
relative to the top-left corner of the window client area.
*/
void GLApp::mousepos_cb([[maybe_unused]] GLFWwindow* pwin, [[maybe_unused]] double xpos, [[maybe_unused]] double ypos) {
    //ImGui_ImplGlfw_CursorPosCallback(pwin, xpos, ypos);
#ifdef _DEBUG
    //std::cout << "Mouse cursor position: (" << xpos << ", " << ypos << ")" << std::endl;
#endif
}

/* _________________________________________________________________________*/
/*! mousescroll_cb

@param GLFWwindow*
Handle to window that is receiving event

@param double
Scroll offset along X-axis

@param double
Scroll offset along Y-axis

@return none

This function is called when the user scrolls, whether with a mouse wheel or
touchpad gesture. Although the function receives 2D scroll offsets, a simple
mouse scroll wheel, being vertical, provides offsets only along the Y-axis.
*/
void GLApp::mousescroll_cb([[maybe_unused]] GLFWwindow* pwin, [[maybe_unused]] double xoffset, [[maybe_unused]] double yoffset) {
    //ImGui_ImplGlfw_ScrollCallback(pwin, xoffset, yoffset);
#ifdef _DEBUG
    //std::cout << "Mouse scroll wheel offset: ("
    //    << xoffset << ", " << yoffset << ")" << std::endl;
#endif
}

/* _________________________________________________________________________ */
/*! error_cb

@param int
GLFW error code

@parm char const*
Human-readable description of the code

@return none

The error callback receives a human-readable description of the error and
(when possible) its cause.
*/
void GLApp::error_cb([[maybe_unused]] int error, char const* description) {
    LOGE(Core, description);
}

/* _________________________________________________________________________ */
/*! fbsize_cb

@param GLFWwindow*
Handle to window that is being resized

@parm int
Width in pixels of new window size

@parm int
Height in pixels of new window size

@return none

This function is called when the window is resized - it receives the new size
of the window in pixels.
*/
void GLApp::fbsize_cb([[maybe_unused]] GLFWwindow* ptr_win, int fbWidth, int fbHeight) {
    // 1. Update Global State
    GLApp::width = fbWidth;
    GLApp::height = fbHeight;

#if !USE_IMGUI_EDITOR
    // GAME MODE: Handle Aspect Ratio Resize immediately to prevent visual glitches
    // This ensures that if a render happens mid-resize, the camera matrix is correct.

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

    // Force viewport Update immediately so the very next draw call is correct
    // (Optional, but helps with flicker on some drivers)
    // glViewport(0, 0, fbWidth, fbHeight); 

#else
    // EDITOR MODE: Just use full size
    using namespace CrashoutsEngine::RenderCamera;
    engineCamera.UpdateMatrices(fbWidth, fbHeight);
#endif
}


/* _________________________________________________________________________*/
/*! update_time

@param double
fps: computed frames per second

@param double
fps_calc_interval: the interval (in seconds) at which fps is to be
calculated

@return none

This function must be first called once during initialization and once
per game loop. It uses GLFW's time functions to compute:
1. interval in seconds between each frame
2. frames per second every "fps_calc_interval" seconds
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

