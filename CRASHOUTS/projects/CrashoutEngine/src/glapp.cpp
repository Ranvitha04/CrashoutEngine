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

*//*__________________________________________________________________________*/

/* _________________________________________________________________________
----------------------------------------------------------------------------- */

#include "glapp.h"
#include "rendering.h"
#include "managers/entitymanager.h"
#include "shapes.h"
#include "resources/textures.h"
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>

// ImGui headers for callbacks
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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
GLFWwindow* GLApp::ptr_window;

Component::Rendering model;
Component::Shapes shapes;
//Component::Texture texture;


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

static float TestRotation = 0.0f; // global rotation angle
static float RightTest = 0.0f;
static float TestScale = 0.0f;

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

    // size of viewport: width x height
    GLApp::ptr_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!GLApp::ptr_window) {
        std::cerr << "GLFW unable to create OpenGL context - abort program\n";
        LOGE(Core, "GLFW unable to create OpenGL context - abort program!!!");
        glfwTerminate();
        return false;
    }

    // make the previously created OpenGL context current ...
    glfwMakeContextCurrent(GLApp::ptr_window);

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

    model.Setup_VAO();
    model.Setup_Shdrpgm();


    Component::Rendering::Initialize();

    //Component::Texture::Initialize();

    Component::TextureRenderer::Initialize();

    //texture.LoadTexture("../../../projects/CrashoutEngine/images/duck-rgba-256.tex");

    //model.texobj = Component::Texture::LoadTexture("../../../projects/CrashoutEngine/images/duck-rgba-256.tex");
   

    return true;
}



/* _________________________________________________________________________ */
/*! update

@param none

@return none

For now, there is nothing to do except to write appropriate information
to window title bar.
*/
void GLApp::update() {
    // write window title with current fps ...
    std::stringstream sstr;
    sstr << std::fixed << std::setprecision(2) << GLApp::title << " | " << FrameRateController::GetInstance().FPS();
    glfwSetWindowTitle(GLApp::ptr_window, sstr.str().c_str());

    //GLsizei position_data_offset = 0;
    GLsizei position_attribute_size = sizeof(glm::vec3); // Changed from vec2 to vec3 to match rendering.cpp
    GLsizei position_data_size = position_attribute_size * 4;
    GLsizei color_data_offset = position_data_size;
    GLsizei color_attribute_size = sizeof(glm::vec3);
    GLsizei color_data_size = color_attribute_size * static_cast<GLsizei>(clr_vtx.size());

    glNamedBufferSubData(model.vbo_hdl, color_data_offset,
        color_data_size, clr_vtx.data());

    GLint w{ GLApp::width }, h{ GLApp::height };
    static GLint old_w{}, old_h{};
    // update viewport settings in vps only if window's dimension change
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

For now, there is nothing to do except set the back buffer fill color
as black.
*/
void GLApp::draw() {
    // clear colorbuffer with RGBA value in glClearColor ...
    //glClearColor(0, 0, 0, 0);
    //glClear(GL_COLOR_BUFFER_BIT);
    //model.Draw();

    // ---- choose which shape to draw by uncommenting ----
    CEVec3 pos(RightTest, 0.0f, 0.0f);
    //float rotation = 85.0f;
    //float scale = 45.f;
    //CEVec3 pos(0.0f, 0.0f, 0.0f);
    //float rotation = 35.0f;
    //float scale = 1.0f;

    // Testing for drawing Square
    //Component::Shapes::CreateSquare(pos, 0.5f, TestRotation, scale, CEVec3(.0f, 1.0f, 0.0f));

    // Testing for drawing Line
    //Component::Shapes::CreateLine(CEVec3(-0.5f, -0.5f, 0), CEVec3(0.5f, 0.5f, 0), 0.01f,TestRotation, scale, CEVec3(0, 1, 0));

    // Testing for drawing Rectangle
    //Component::Shapes::CreateRectangle(CEVec3(0.0f, 0.0f, 0.0f), 1.0f, 1.3f, TestRotation, 1.0f, CEVec3(1.0f, 0.0f, 0.0f));

    // Testing for drawing Triangle
    //Component::Shapes::CreateTriangle(pos, CEVec3(-0.4,-0.5,0), CEVec3(0.4,-0.5,0), TestRotation, scale, CEVec3(1,1,0));

    // Draw current model
    //model.Draw();

    //Component::Shapes::CreateTexturedTriangle("../../../projects/CrashoutEngine/assets/images/duck-rgba-256.tex", pos, CEVec3(-0.4, -0.5, 0), CEVec3(0.4, -0.5, 0), TestRotation, TestScale, CEVec3(1, 1, 0));


    //Component::TextureRenderer::CreateTexturedQuad(
    //    "../../../projects/CrashoutEngine/images/duck.jpg",
    //CEVec3(0.0f, 0.0f, 0.0f),  // position
    //    CEVec2(1.0f, 1.0f),        // size
    //    TestRotation,                       // rotation
    //    scale,                       // scale
    //    CEVec3(1.0f, 1.0f, 1.0f),  // tint
    //    false;                      // modulate
    //);

    
    

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

    //Component::Texture::Cleanup();

    Component::Rendering::Cleanup();

    Component::TextureRenderer::Cleanup();

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
When the ESC key is pressed, the close flag of the window is set.
*/
void GLApp::key_cb(GLFWwindow* pwin, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mod) {
    //ImGui_ImplGlfw_KeyCallback(pwin, key, scancode, action, mod);
    Key k = static_cast<Key>(key);
    if (GLFW_PRESS == action) {
#ifdef _DEBUG
        LOGI(Input, std::string(KeyToString(k)) + " is pressed");
#endif
    }
    else if (GLFW_REPEAT == action) {
#ifdef _DEBUG
        LOGI(Input, std::string(KeyToString(k)) + " is repeatedly pressed");
#endif
    }
    else if (GLFW_RELEASE == action) {
#ifdef _DEBUG
        LOGI(Input, std::string(KeyToString(k)) + " is released");
#endif
    }

    if (GLFW_PRESS == action) {
        if (key == GLFW_KEY_R) { // Rotate with 'R'
            TestRotation += 10.0f;  // rotate 10 degrees each press
            if (TestRotation >= 360.0f) TestRotation -= 360.0f;
        }
    }

    if (GLFW_PRESS == action) {
        if (key == GLFW_KEY_I) {
            RightTest += 25.0f;
            std::cout << RightTest << std::endl;
        }
    }

    if (GLFW_PRESS == action) {
        if (key == GLFW_KEY_S) { // Rotate with 'R'
            TestScale += 10.0f;  // rotate 10 degrees each press
            
        }
    }

    if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
        glfwSetWindowShouldClose(pwin, GLFW_TRUE);
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
void GLApp::mousebutton_cb([[maybe_unused]] GLFWwindow* pwin, int button, int action, [[maybe_unused]] int mod) {
    //ImGui_ImplGlfw_MouseButtonCallback(pwin, button, action, mod);
    Key k = static_cast<Key>(button);

#ifdef _DEBUG
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
    // use the entire framebuffer as drawing region
    glViewport(0, 0, fbWidth, fbHeight);;
    // later, if working in 3D, we'll have to set the projection matrix here ...

    GLApp::width = fbWidth;
    GLApp::height = fbHeight;
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

    // update fps at least every 10 seconds ...
    fps_calc_interval = (fps_calc_interval < 0.0) ? 0.0 : fps_calc_interval;
    fps_calc_interval = (fps_calc_interval > 10.0) ? 10.0 : fps_calc_interval;
    if (elapsed_time > fps_calc_interval) {
        GLApp::fps = count / elapsed_time;
        start_time = curr_time;
        count = 0.0;
    }
}
