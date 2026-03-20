/*!
@file       inputmanager.h
@author     Renfred Lee
@author     Shahrin Idzuan
@author     Renfred (RTIS)
@date       11/09/2025
@brief      Declaration of the InputManager class

Provides static functions to handle keyboard mouse and scroll wheel input using GLFW

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <rapidjson/document.h>
#include "key.h"
#include "CEVec2.h"
#include <string>

namespace CrashoutEngine
{
    class ControllerManager;
}

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

    static bool IsKeyHeld(Key key);

    static bool IsKeyHeld(int key);
    /*!
    @brief Check if a mouse button is pressed
    @param button GLFW mouse button code
    @return True if pressed false otherwise
    */
    static bool IsMouseButtonPressed(int button);
    static bool IsMouseButtonTriggered(int button);
    static bool IsMouseButtonReleased(int button);

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

    // Gets all characters typed this frame (e.g., "abc")
    static std::string GetTypedCharacters();

    // Checks if a key was pressed *this frame* (not held)
    static bool IsKeyTriggered(int key);

    static bool IsKeyTriggered(Key key);

    // Unified gameplay-facing queries. Keyboard/mouse still come from GLFW,
    // while controllers are sourced from SDL2's GameController API.
    static bool GetAction(const std::string& actionName);
    static bool GetActionTriggered(const std::string& actionName);
    static bool GetActionReleased(const std::string& actionName);
    static float GetAxis(const std::string& axisName);

    static void ClearInputs();

    // --- EDITOR / GAME VIEWPORT SUPPORT ---
    // Called by the Editor (DebugViewport) to override mouse pos relative to the game image
    static void SetGameViewMouse(float x, float y, float w, float h);

    // Returns mouse position relative to the Game View (Editor Panel or Full Window)
    static CEVec2 GetGameMousePos();

    // Returns the size of the Game View (Editor Panel or Full Window)
    static CEVec2 GetGameWindowSize();

    // --- REPLAY SYSTEM HOOKS ---
    static bool s_isReplayMode;
    static void SetReplayMode(bool enabled);
    static void InjectKey(Key key, bool isPressed);
    static void InjectMouseButton(Key button, bool isPressed);

private:
    struct DigitalBinding
    {
        enum class Type
        {
            Key,
            MouseButton,
            ControllerButton,
            ControllerAxis
        };

        Type type = Type::Key;
        int key = 0;
        int mouseButton = 0;
        int controllerButton = -1;
        int controllerAxis = -1;
        float threshold = 0.5f;
        int direction = 1;
    };

    struct AxisBinding
    {
        enum class Type
        {
            KeyAxis,
            ControllerAxis
        };

        Type type = Type::KeyAxis;
        int negativeKey = 0;
        int positiveKey = 0;
        int controllerAxis = -1;
        float deadzone = -1.0f;
        bool invert = false;
    };

    /// Pointer to the GLFW window
    static GLFWwindow* Window;

    // Game View State
    static bool s_usingGameView;
    static CEVec2 s_gameMousePos;
    static CEVec2 s_gameWindowSize;

    /// Map of key states indexed by GLFW key code
    static std::unordered_map<int, bool> Keys;

    /// Map of mouse button states indexed by GLFW button code
    static std::unordered_map<int, bool> MouseButtons;

    static std::unordered_map<int, bool> s_lastMouseButtons;

    /// Accumulated scroll value between updates
    static float ScrollYAccum;

    /// Scroll delta value for the current frame only
    static float ScrollYFrame;

    // Stores chars from the GLFW callback
    static std::string s_typedChars;
    
    // Stores the key state from the *previous* frame
    static std::unordered_map<int, bool> s_lastKeys;

    // The GLFW callback for character input
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);

    /*!
    @brief GLFW callback function for mouse scroll events
    @param window The window that received the event
    @param yoffset The vertical scroll offset
    */
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    static float GetKeyboardAxis(int negativeKey, int positiveKey);
    static bool EvaluateDigitalBinding(const DigitalBinding& binding);
    static bool EvaluateDigitalBindingTriggered(const DigitalBinding& binding);
    static bool EvaluateDigitalBindingReleased(const DigitalBinding& binding);
    static float EvaluateAxisBinding(const AxisBinding& binding);
    static std::string GetBindingsPath();
    static bool LoadBindings(const std::string& path);
    static bool ParseDigitalBinding(const rapidjson::Value& value, DigitalBinding& binding);
    static bool ParseAxisBinding(const rapidjson::Value& value, AxisBinding& binding);
    static int ParseKeyName(const std::string& keyName);
    static int ParseMouseButtonName(const std::string& buttonName);
    static int ParseControllerButtonName(const std::string& buttonName);
    static int ParseControllerAxisName(const std::string& axisName);
    static std::string ToUpper(std::string value);
    static float ClampAxis(float value);
    static float ChooseDominantAxis(float currentValue, float candidateValue);

    // This holds the "Ghost's" currently held buttons across multiple frames
    static std::unordered_map<int, bool> s_replayKeys;
    static std::unordered_map<int, bool> s_replayMouseButtons;

    static std::unique_ptr<CrashoutEngine::ControllerManager> s_controllerManager;
    static std::unordered_map<std::string, std::vector<DigitalBinding>> s_actionBindings;
    static std::unordered_map<std::string, std::vector<AxisBinding>> s_axisBindings;
    static float s_defaultDeadzone;
};
