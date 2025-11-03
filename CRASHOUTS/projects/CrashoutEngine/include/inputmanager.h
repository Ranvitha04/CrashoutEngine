/*!
@file       inputmanager.h
@author     m.biniswandi@digipen.edu
@date       11/09/2025
@brief      Declaration of the InputManager class

Provides static functions to handle keyboard mouse and scroll wheel input using GLFW
*/
#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>
#include "key.h"
#include "CEVec2.h"
/*!
@class InputManager
@brief Handles input state management for keys mouse buttons and scroll wheel

InputManager is a fully static class that polls GLFW and tracks state per frame
*/
class InputManager {
public:
    /*!
    @brief Initialize the input manager with a GLFW window
    @param ptr_window Pointer to the GLFW window
    */
    static void Init(GLFWwindow* ptr_window);

    /*!
    @brief Shutdown and cleanup the input manager
    */
    static void Shutdown();

    /*!
    @brief Update input states for the current frame
    */
    static void Update();

    /*!
    @brief Check if a key is pressed using the Key enum
    @param key Key enum value
    @return True if pressed false otherwise
    */
    static bool IsKeyPressed(Key key);

    /*!
    @brief Check if a key is pressed using a GLFW key code
    @param glfwKey GLFW key code
    @return True if pressed false otherwise
    */
    static bool IsKeyPressed(int glfwKey);

    /*!
    @brief Check if a mouse button is pressed
    @param button GLFW mouse button code
    @return True if pressed false otherwise
    */
    static bool IsMouseButtonPressed(int button);

    /// Get all currently pressed keys and mouse buttons
    static std::vector<Key> GetPressedKeys();

    /*!
    @brief Get the mouse position in pixels
    @param x Reference to store X position
    @param y Reference to store Y position
    */
    static void GetMousePosition(double& x, double& y);

    /*!
    @brief Get the mouse position in pixels as a 2D vector
    @return Mouse position in pixels
    */
    static CEVec2 GetMousePosition();

    /*!
    @brief Get the mouse scroll delta for the current frame
    @return Positive for scroll up negative for scroll down
    */
    static float GetMouseWheelDelta();

    /*!
    @brief Get the mouse position in normalized device coordinates
    @return Mouse position mapped to NDC range [-1 1]
    */
    static CEVec2 GetMouseNDC();

    static CEVec2 GetMouseWorldPos();

    static CEVec2 g_mouseWorldPos;

private:
    /// Pointer to the GLFW window
    static GLFWwindow* Window;

    /// Map of key states indexed by GLFW key code
    static std::unordered_map<int, bool> Keys;

    /// Map of mouse button states indexed by GLFW button code
    static std::unordered_map<int, bool> MouseButtons;

    /// Accumulated scroll value between updates
    static float ScrollYAccum;

    /// Scroll delta value for the current frame only
    static float ScrollYFrame;


    /*!
    @brief GLFW callback function for mouse scroll events
    @param window The window that received the event
    @param yoffset The vertical scroll offset
    */
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};