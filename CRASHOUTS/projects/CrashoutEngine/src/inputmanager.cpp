/*!
@file       inputmanager.cpp
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Implementation of the InputManager class

Handles keyboard mouse and scroll wheel input using GLFW
*/
#include "inputmanager.h"

#include <iostream>
#include <ostream>
#include <log.h>
#include "settings.h"
#include <glm/glm.hpp>

#include "CEVec2.h"
//ENTIRE INPUTMANAGER IS TO BE REFACTORED/REMOVED LATER TO ADHERE TO ECS - Rain
/// Static storage for the GLFW window pointer
GLFWwindow* InputManager::Window = nullptr;

/// Map of key states indexed by GLFW key codes
std::unordered_map<int, bool> InputManager::Keys;

/// Map of mouse button states indexed by GLFW button codes
std::unordered_map<int, bool> InputManager::MouseButtons;

/// Accumulated scroll wheel delta since last update
float InputManager::ScrollYAccum = 0.0f;

/// Scroll wheel delta captured during the current frame
float InputManager::ScrollYFrame = 0.0f;

CEVec2 InputManager::g_mouseWorldPos{ 0.f, 0.f };

std::string InputManager::s_typedChars;

std::unordered_map<int, bool> InputManager::s_lastKeys;

CEVec2 InputManager::GetMouseWorldPos() {
    return g_mouseWorldPos;
}

/*!
@brief Initialize the input manager with a GLFW window
@param w Pointer to the GLFW window
*/
void InputManager::Init(GLFWwindow* ptr_window) {
    Window = ptr_window;
    glfwSetScrollCallback(Window, ScrollCallback);
    // Tell GLFW to call CharCallback when a key is typed
    glfwSetCharCallback(Window, CharCallback);
}

/*!
@brief Update input states for this frame

Snapshots scroll delta resets accumulator and refreshes key and mouse button states
*/
void InputManager::Update() {
    // Clear typed chars and store last frame's key state
    s_typedChars.clear();
    s_lastKeys = Keys;

    // Snapshot scroll delta for this frame and reset accumulator
    ScrollYFrame = ScrollYAccum;
    ScrollYAccum = 0.0f;

    Keys.clear();
    MouseButtons.clear();

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        Keys[key] = glfwGetKey(Window, key) == GLFW_PRESS;
    }

    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        MouseButtons[button] = glfwGetMouseButton(Window, button) == GLFW_PRESS;
    }
}

void InputManager::Shutdown() {
    // 1. Decouple callbacks from the window
    if (Window != nullptr) {
        // Set all callbacks back to NULL or a safe stub
        glfwSetKeyCallback(Window, nullptr);
        glfwSetMouseButtonCallback(Window, nullptr);
        glfwSetScrollCallback(Window, nullptr);

        // Clear the window pointer
        Window = nullptr;
    }

    // 2. Clear internal state (frees memory used by the maps)
    Keys.clear();
    MouseButtons.clear();
    ScrollYAccum = 0.0f;
    ScrollYFrame = 0.0f;
}

/*!
@brief Check if a specific key is pressed using Key enum
@param key The Key enum value
@return True if pressed false otherwise
*/
bool InputManager::IsKeyPressed(Key key) {
    return IsKeyPressed(static_cast<int>(key));
}

/*!
@brief Check if a specific key is pressed using a GLFW key code
@param glfwKey The GLFW key code
@return True if pressed false otherwise
*/
bool InputManager::IsKeyPressed(int glfwKey) {
    return glfwGetKey(Window, glfwKey) == GLFW_PRESS;
}

std::vector<Key> InputManager::GetPressedKeys() {
    std::vector<Key> pressed;

    // keys
    for (const auto& [key, state] : Keys) {
        if (state) {
            pressed.push_back(static_cast<Key>(key));
        }
    }

    // mouse buttons
    for (const auto& [button, state] : MouseButtons) {
        if (state) {
            pressed.push_back(static_cast<Key>(button));
        }
    }

    return pressed;
}

/*!
@brief Check if a mouse button is pressed
@param button The GLFW mouse button code
@return True if pressed false otherwise
*/
bool InputManager::IsMouseButtonPressed(int button) {
    return MouseButtons[button];
}

/*!
@brief Get the current mouse position in pixels
@param x Reference to store X position
@param y Reference to store Y position
*/
void InputManager::GetMousePosition(double& x, double& y) {
    glfwGetCursorPos(Window, &x, &y);
}

/*!
@brief Get the current mouse position in pixels
@return 2D vector with X and Y position
*/
CEVec2 InputManager::GetMousePosition() {
    double x, y;
    GetMousePosition(x, y);
    return CEVec2(static_cast<float>(x), static_cast<float>(y));
}

/*!
@brief Get the scroll wheel delta for the current frame
@return Positive for scroll up negative for scroll down
*/
float InputManager::GetMouseWheelDelta() {
    return ScrollYFrame;
}

/*!
@brief Get the mouse position in normalized device coordinates

Converts screen pixel coordinates to NDC range [-1 1]
@return 2D vector with X and Y in NDC space
*/
CEVec2 InputManager::GetMouseNDC() {
    double x, y;
    GetMousePosition(x, y);

    float ndcX = static_cast<float>(x / Settings::GetScreenWidth()) * 2.0f - 1.0f;

    float ndcY = 1.0f - (static_cast<float>(y) / Settings::GetScreenHeight()) * 2.0f;

    return CEVec2(ndcX, ndcY);
}

/*!
@brief GLFW callback function for mouse scroll events

This function is called by GLFW asynchronously whenever the mouse wheel is moved
It adds the vertical scroll delta to the accumulator
*/
void InputManager::ScrollCallback(GLFWwindow * window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    // We only care about the vertical scroll for now
    // We accumulate the delta value. Update() will handle snapshotting it.
    ScrollYAccum += static_cast<float>(yoffset);
}

/*!
@brief GLFW callback for character (typing) events
*/
void InputManager::CharCallback([[maybe_unused]] GLFWwindow* window, unsigned int codepoint) {
    // This is called by GLFW automatically
    s_typedChars += static_cast<char>(codepoint);
}

/*!
@brief Get all characters typed since the last frame
*/
std::string InputManager::GetTypedCharacters() {
    return s_typedChars;
}

/*!
@brief Check if a key was pressed *this frame*
@return True if pressed this frame, false if held or not pressed
*/
bool InputManager::IsKeyTriggered(int key) {
    // Check if the key IS pressed now
    bool pressedNow = Keys.count(key) ? Keys[key] : false;
    // Check if it was NOT pressed last frame
    bool pressedLast = s_lastKeys.count(key) ? s_lastKeys[key] : false;

    return pressedNow && !pressedLast;
}

bool InputManager::IsKeyTriggered(Key key) {
    return IsKeyTriggered(static_cast<int>(key));
}