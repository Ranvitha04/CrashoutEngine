/*!
@file       inputmanager.cpp
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Implementation of the InputManager class

Handles keyboard mouse and scroll wheel input using GLFW
*/
#include "managers/inputmanager.h"

#include <iostream>
#include <ostream>
#include <log.h>
#include "settings.h"
#include <glm/glm.hpp>

#include "math/CEVec2.h"
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

/*!
@brief Initialize the input manager with a GLFW window
@param w Pointer to the GLFW window
*/
void InputManager::Init(GLFWwindow* ptr_window) {
    Window = ptr_window;
}

/*!
@brief Update input states for this frame

Snapshots scroll delta resets accumulator and refreshes key and mouse button states
*/
void InputManager::Update() {
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

    float ndcY = static_cast<float>(y / Settings::GetScreenHeight()) * 2.0f - 1.0f;

    return CEVec2(ndcX, ndcY);
}


