/*!
@file       inputmanager.cpp
@author     Renfred Lee
@author     Shahrin Idzuan
@author     Renfred (RTIS)
@date       11/09/2025
@brief      Implementation of the InputManager class

Handles keyboard mouse and scroll wheel input using GLFW

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "inputmanager.h"
#include "ControllerManager.h"
#include "engine.h"
#include "replaymanager.h"
#include "frameratecontroller.h"

#include <iostream>
#include <ostream>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <log.h>
#include "settings.h"
#include <glm/glm.hpp>

#include "CEVec2.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

namespace
{
    namespace fs = std::filesystem;
}

/// Static storage for the GLFW window pointer
GLFWwindow* InputManager::Window = nullptr;

/// Map of key states indexed by GLFW key codes
std::unordered_map<int, bool> InputManager::Keys;

/// Map of mouse button states indexed by GLFW button codes
std::unordered_map<int, bool> InputManager::MouseButtons;

std::unordered_map<int, bool> InputManager::s_lastMouseButtons;

/// Accumulated scroll wheel delta since last Update
float InputManager::ScrollYAccum = 0.0f;

// Scroll wheel delta captured during the current frame
float InputManager::ScrollYFrame = 0.0f;

CEVec2 InputManager::g_mouseWorldPos{ 0.f, 0.f };

std::string InputManager::s_typedChars;

std::unordered_map<int, bool> InputManager::s_lastKeys;

bool InputManager::s_isReplayMode = false;
std::unordered_map<int, bool> InputManager::s_replayKeys;
std::unordered_map<int, bool> InputManager::s_replayMouseButtons;
std::unique_ptr<CrashoutEngine::ControllerManager> InputManager::s_controllerManager;
std::unordered_map<std::string, std::vector<InputManager::DigitalBinding>> InputManager::s_actionBindings;
std::unordered_map<std::string, std::vector<InputManager::AxisBinding>> InputManager::s_axisBindings;
float InputManager::s_defaultDeadzone = 0.2f;

// Game View State
bool InputManager::s_usingGameView = false;
CEVec2 InputManager::s_gameMousePos{ 0.f, 0.f };
CEVec2 InputManager::s_gameWindowSize{ 0.f, 0.f };

// Change replay mode based on given boolean
void InputManager::SetReplayMode(bool enabled) {
    s_isReplayMode = enabled;
    if (!enabled) {
        // Clear ghost inputs when turning off replay mode
        s_replayKeys.clear();
        s_replayMouseButtons.clear();
    }
}

// Set value in replay key map based on given key and pressed state
void InputManager::InjectKey(Key key, bool isPressed) {
    s_replayKeys[static_cast<int>(key)] = isPressed;
}

// Set value in replay mouse button map
void InputManager::InjectMouseButton(Key button, bool isPressed) {
    s_replayMouseButtons[static_cast<int>(button)] = isPressed;
}

CEVec2 InputManager::GetMouseWorldPos() {
    return g_mouseWorldPos;
}

void InputManager::SetGameViewMouse(float x, float y, float w, float h) {
    s_usingGameView = true;
    s_gameMousePos = { x, y };
    s_gameWindowSize = { w, h };
}

CEVec2 InputManager::GetGameMousePos() {
    // If Editor is driving this, return the overridden pos
    if (s_usingGameView) return s_gameMousePos;
    
    // Fallback: Return raw window mouse pos (Release mode / Standalone)
    return GetMousePosition();
}

CEVec2 InputManager::GetGameWindowSize() {
    if (s_usingGameView && s_gameWindowSize.x > 0 && s_gameWindowSize.y > 0) 
        return s_gameWindowSize;

    // Fallback: Return actual window size from GLFW
    int w, h;
    if (Window) {
        glfwGetWindowSize(Window, &w, &h);
        return CEVec2{ (float)w, (float)h };
    }

    return CEVec2{ (float)Settings::GetScreenWidth(), (float)Settings::GetScreenHeight() };
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

    if (!s_controllerManager)
    {
        s_controllerManager = std::make_unique<CrashoutEngine::ControllerManager>();
        s_controllerManager->Initialize();
    }

    std::string bindingPath = GetBindingsPath();
    if (!LoadBindings(bindingPath))
    {
        bindingPath = GetGameRoot() + "/assets/json/InputBindings.json";
        if (!LoadBindings(bindingPath))
        {
            bindingPath = GetGameRoot() + "/assets/InputBindings.json";
            LoadBindings(bindingPath);
        }
    }
}

/*!
@brief Update input states for this frame

Snapshots scroll delta resets accumulator and refreshes key and mouse button states
*/
void InputManager::Update() {
    s_usingGameView = false;

    if (s_controllerManager)
    {
        s_controllerManager->BeginFrame();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            s_controllerManager->HandleEvent(event);
        }
    }

    // Clear typed chars and store last frame's key state
    s_typedChars.clear();
    s_lastKeys = Keys;
    s_lastMouseButtons = MouseButtons;

    ScrollYFrame = ScrollYAccum;
    ScrollYAccum = 0.0f;

    Keys.clear();
    MouseButtons.clear();

    // NORMAL MODE: Poll physical hardware as usual.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        bool isDown = glfwGetKey(Window, key) == GLFW_PRESS;
        Keys[key] = isDown;
    }

    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        bool isDown = glfwGetMouseButton(Window, button) == GLFW_PRESS;
        MouseButtons[button] = isDown;
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
    s_lastMouseButtons.clear();
    ScrollYAccum = 0.0f;
    ScrollYFrame = 0.0f;

    if (s_controllerManager)
    {
        s_controllerManager->Shutdown();
        s_controllerManager.reset();
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
@brief Check if a mouse button was clicked *this frame*
*/
bool InputManager::IsMouseButtonTriggered(int button) {
    bool pressedNow = MouseButtons.count(button) ? MouseButtons[button] : false;
    bool pressedLast = s_lastMouseButtons.count(button) ? s_lastMouseButtons[button] : false;

    // True if it is pressed NOW, but was NOT pressed last frame
    return pressedNow && !pressedLast;
}

/*!
@brief Check if a mouse button was released *this frame*
*/
bool InputManager::IsMouseButtonReleased(int button) {
    bool pressedNow = MouseButtons.count(button) ? MouseButtons[button] : false;
    bool pressedLast = s_lastMouseButtons.count(button) ? s_lastMouseButtons[button] : false;

    // True if it is NOT pressed now, but WAS pressed last frame
    return !pressedNow && pressedLast;
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

bool InputManager::IsKeyHeld(int key)
{
    return Keys.count(key) ? Keys.at(key) : false;
}

bool InputManager::IsKeyHeld(Key key)
{
    return IsKeyHeld(static_cast<int>(key));
}

float InputManager::GetAxis(const std::string& axisName)
{
    const auto axisIt = s_axisBindings.find(axisName);
    if (axisIt == s_axisBindings.end())
    {
        return 0.0f;
    }

    float axisValue = 0.0f;
    for (const AxisBinding& binding : axisIt->second)
    {
        axisValue = ChooseDominantAxis(axisValue, EvaluateAxisBinding(binding));
    }

    return ClampAxis(axisValue);
}

std::string InputManager::GetBindingsPath()
{
    const fs::path bindingsDir = fs::path(GetGameRoot()) / "assets" / "input";
    std::error_code errorCode;
    fs::create_directories(bindingsDir, errorCode);
    return (bindingsDir / "InputBindings.json").string();
}

bool InputManager::GetAction(const std::string& actionName)
{
    const auto actionIt = s_actionBindings.find(actionName);
    if (actionIt == s_actionBindings.end())
    {
        return false;
    }

    for (const DigitalBinding& binding : actionIt->second)
    {
        if (EvaluateDigitalBinding(binding))
        {
            return true;
        }
    }

    return false;
}

bool InputManager::GetActionTriggered(const std::string& actionName)
{
    const auto actionIt = s_actionBindings.find(actionName);
    if (actionIt == s_actionBindings.end())
    {
        return false;
    }

    for (const DigitalBinding& binding : actionIt->second)
    {
        if (EvaluateDigitalBindingTriggered(binding))
        {
            return true;
        }
    }

    return false;
}

bool InputManager::GetActionReleased(const std::string& actionName)
{
    const auto actionIt = s_actionBindings.find(actionName);
    if (actionIt == s_actionBindings.end())
    {
        return false;
    }

    for (const DigitalBinding& binding : actionIt->second)
    {
        if (EvaluateDigitalBindingReleased(binding))
        {
            return true;
        }
    }

    return false;
}

void InputManager::ClearInputs()
{
    Keys.clear();
    s_lastKeys.clear();
    MouseButtons.clear();
    s_lastMouseButtons.clear();
    ScrollYAccum = 0.0f;
    ScrollYFrame = 0.0f;
    s_typedChars.clear();
    if (s_controllerManager)
    {
        s_controllerManager->SyncState();
    }
}

float InputManager::GetKeyboardAxis(int negativeKey, int positiveKey)
{
    const float negative = IsKeyHeld(negativeKey) ? 1.0f : 0.0f;
    const float positive = IsKeyHeld(positiveKey) ? 1.0f : 0.0f;
    return positive - negative;
}

bool InputManager::EvaluateDigitalBinding(const DigitalBinding& binding)
{
    switch (binding.type)
    {
    case DigitalBinding::Type::Key:
        return IsKeyHeld(binding.key);

    case DigitalBinding::Type::MouseButton:
        return IsMouseButtonPressed(binding.mouseButton);

    case DigitalBinding::Type::ControllerButton:
        return s_controllerManager &&
            s_controllerManager->GetButton(static_cast<SDL_GameControllerButton>(binding.controllerButton));

    case DigitalBinding::Type::ControllerAxis:
        if (!s_controllerManager)
        {
            return false;
        }
        {
            const float axisValue = s_controllerManager->GetAxis(
                static_cast<SDL_GameControllerAxis>(binding.controllerAxis),
                s_defaultDeadzone);
            return binding.direction < 0 ? axisValue <= -binding.threshold : axisValue >= binding.threshold;
        }
    }

    return false;
}

bool InputManager::EvaluateDigitalBindingTriggered(const DigitalBinding& binding)
{
    switch (binding.type)
    {
    case DigitalBinding::Type::Key:
        return IsKeyTriggered(binding.key);

    case DigitalBinding::Type::MouseButton:
        return IsMouseButtonTriggered(binding.mouseButton);

    case DigitalBinding::Type::ControllerButton:
        return s_controllerManager &&
            s_controllerManager->GetButtonDown(static_cast<SDL_GameControllerButton>(binding.controllerButton));

    case DigitalBinding::Type::ControllerAxis:
        if (!s_controllerManager)
        {
            return false;
        }
        {
            const float currentAxis = s_controllerManager->GetAxis(
                static_cast<SDL_GameControllerAxis>(binding.controllerAxis),
                s_defaultDeadzone);
            const float previousAxis = s_controllerManager->GetPreviousAxis(
                static_cast<SDL_GameControllerAxis>(binding.controllerAxis),
                s_defaultDeadzone);
            return binding.direction < 0
                ? currentAxis <= -binding.threshold && previousAxis > -binding.threshold
                : currentAxis >= binding.threshold && previousAxis < binding.threshold;
        }
    }

    return false;
}

bool InputManager::EvaluateDigitalBindingReleased(const DigitalBinding& binding)
{
    switch (binding.type)
    {
    case DigitalBinding::Type::Key:
    {
        const bool nowPressed = IsKeyHeld(binding.key);
        const bool wasPressed = s_lastKeys.count(binding.key) ? s_lastKeys[binding.key] : false;
        return !nowPressed && wasPressed;
    }

    case DigitalBinding::Type::MouseButton:
        return IsMouseButtonReleased(binding.mouseButton);

    case DigitalBinding::Type::ControllerButton:
        return s_controllerManager &&
            s_controllerManager->GetButtonUp(static_cast<SDL_GameControllerButton>(binding.controllerButton));

    case DigitalBinding::Type::ControllerAxis:
        if (!s_controllerManager)
        {
            return false;
        }
        {
            const float currentAxis = s_controllerManager->GetAxis(
                static_cast<SDL_GameControllerAxis>(binding.controllerAxis),
                s_defaultDeadzone);
            const float previousAxis = s_controllerManager->GetPreviousAxis(
                static_cast<SDL_GameControllerAxis>(binding.controllerAxis),
                s_defaultDeadzone);
            return binding.direction < 0
                ? currentAxis > -binding.threshold && previousAxis <= -binding.threshold
                : currentAxis < binding.threshold && previousAxis >= binding.threshold;
        }
    }

    return false;
}

float InputManager::EvaluateAxisBinding(const AxisBinding& binding)
{
    float value = 0.0f;

    switch (binding.type)
    {
    case AxisBinding::Type::KeyAxis:
        value = GetKeyboardAxis(binding.negativeKey, binding.positiveKey);
        break;

    case AxisBinding::Type::ControllerAxis:
        if (s_controllerManager)
        {
            const float deadzone = binding.deadzone >= 0.0f ? binding.deadzone : s_defaultDeadzone;
            value = s_controllerManager->GetAxis(
                static_cast<SDL_GameControllerAxis>(binding.controllerAxis),
                deadzone);
        }
        break;
    }

    if (binding.invert)
    {
        value = -value;
    }

    return ClampAxis(value);
}

bool InputManager::LoadBindings(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    rapidjson::IStreamWrapper stream(file);
    rapidjson::Document document;
    document.ParseStream(stream);
    if (document.HasParseError() || !document.IsObject())
    {
        return false;
    }

    std::unordered_map<std::string, std::vector<DigitalBinding>> actionBindings;
    std::unordered_map<std::string, std::vector<AxisBinding>> axisBindings;
    float defaultDeadzone = 0.2f;

    if (document.HasMember("deadzone") && document["deadzone"].IsNumber())
    {
        defaultDeadzone = std::clamp(document["deadzone"].GetFloat(), 0.0f, 0.99f);
    }

    if (document.HasMember("actions") && document["actions"].IsObject())
    {
        for (auto member = document["actions"].MemberBegin(); member != document["actions"].MemberEnd(); ++member)
        {
            if (!member->value.IsArray())
            {
                continue;
            }

            for (const auto& bindingValue : member->value.GetArray())
            {
                DigitalBinding binding;
                if (ParseDigitalBinding(bindingValue, binding))
                {
                    actionBindings[member->name.GetString()].push_back(binding);
                }
            }
        }
    }

    if (document.HasMember("axes") && document["axes"].IsObject())
    {
        for (auto member = document["axes"].MemberBegin(); member != document["axes"].MemberEnd(); ++member)
        {
            if (!member->value.IsArray())
            {
                continue;
            }

            for (const auto& bindingValue : member->value.GetArray())
            {
                AxisBinding binding;
                if (ParseAxisBinding(bindingValue, binding))
                {
                    axisBindings[member->name.GetString()].push_back(binding);
                }
            }
        }
    }

    s_actionBindings = std::move(actionBindings);
    s_axisBindings = std::move(axisBindings);
    s_defaultDeadzone = defaultDeadzone;
    return true;
}

bool InputManager::ParseDigitalBinding(const rapidjson::Value& value, DigitalBinding& binding)
{
    if (!value.IsObject() || !value.HasMember("type") || !value["type"].IsString())
    {
        return false;
    }

    const std::string type = ToUpper(value["type"].GetString());
    if (type == "KEY")
    {
        if (!value.HasMember("key") || !value["key"].IsString())
        {
            return false;
        }

        binding.type = DigitalBinding::Type::Key;
        binding.key = ParseKeyName(value["key"].GetString());
        return binding.key != GLFW_KEY_UNKNOWN;
    }

    if (type == "MOUSE_BUTTON")
    {
        if (!value.HasMember("button") || !value["button"].IsString())
        {
            return false;
        }

        binding.type = DigitalBinding::Type::MouseButton;
        binding.mouseButton = ParseMouseButtonName(value["button"].GetString());
        return binding.mouseButton != 0;
    }

    if (type == "CONTROLLER_BUTTON")
    {
        if (!value.HasMember("button") || !value["button"].IsString())
        {
            return false;
        }

        binding.type = DigitalBinding::Type::ControllerButton;
        binding.controllerButton = ParseControllerButtonName(value["button"].GetString());
        return binding.controllerButton >= 0;
    }

    if (type == "CONTROLLER_AXIS")
    {
        if (!value.HasMember("axis") || !value["axis"].IsString())
        {
            return false;
        }

        binding.type = DigitalBinding::Type::ControllerAxis;
        binding.controllerAxis = ParseControllerAxisName(value["axis"].GetString());
        if (binding.controllerAxis < 0)
        {
            return false;
        }

        if (value.HasMember("threshold") && value["threshold"].IsNumber())
        {
            binding.threshold = std::clamp(value["threshold"].GetFloat(), 0.0f, 1.0f);
        }

        if (value.HasMember("direction") && value["direction"].IsString())
        {
            const std::string direction = ToUpper(value["direction"].GetString());
            binding.direction = (direction == "NEGATIVE" || direction == "LEFT" || direction == "DOWN") ? -1 : 1;
        }

        return true;
    }

    return false;
}

bool InputManager::ParseAxisBinding(const rapidjson::Value& value, AxisBinding& binding)
{
    if (!value.IsObject() || !value.HasMember("type") || !value["type"].IsString())
    {
        return false;
    }

    const std::string type = ToUpper(value["type"].GetString());
    if (type == "KEY_AXIS")
    {
        if (!value.HasMember("negative") || !value["negative"].IsString() ||
            !value.HasMember("positive") || !value["positive"].IsString())
        {
            return false;
        }

        binding.type = AxisBinding::Type::KeyAxis;
        binding.negativeKey = ParseKeyName(value["negative"].GetString());
        binding.positiveKey = ParseKeyName(value["positive"].GetString());
        return binding.negativeKey != GLFW_KEY_UNKNOWN && binding.positiveKey != GLFW_KEY_UNKNOWN;
    }

    if (type == "CONTROLLER_AXIS")
    {
        if (!value.HasMember("axis") || !value["axis"].IsString())
        {
            return false;
        }

        binding.type = AxisBinding::Type::ControllerAxis;
        binding.controllerAxis = ParseControllerAxisName(value["axis"].GetString());
        if (binding.controllerAxis < 0)
        {
            return false;
        }

        if (value.HasMember("deadzone") && value["deadzone"].IsNumber())
        {
            binding.deadzone = std::clamp(value["deadzone"].GetFloat(), 0.0f, 0.99f);
        }

        if (value.HasMember("invert") && value["invert"].IsBool())
        {
            binding.invert = value["invert"].GetBool();
        }

        return true;
    }

    return false;
}

int InputManager::ParseKeyName(const std::string& keyName)
{
    const std::string name = keyName;

    static const std::unordered_map<std::string, int> keyMap =
    {
        { "SPACE", GLFW_KEY_SPACE }, { "ESCAPE", GLFW_KEY_ESCAPE }, { "ENTER", GLFW_KEY_ENTER },
        { "TAB", GLFW_KEY_TAB }, { "BACKSPACE", GLFW_KEY_BACKSPACE }, { "LEFT SHIFT", GLFW_KEY_LEFT_SHIFT },
        { "RIGHT SHIFT", GLFW_KEY_RIGHT_SHIFT }, { "LEFT CTRL", GLFW_KEY_LEFT_CONTROL }, { "RIGHT CTRL", GLFW_KEY_RIGHT_CONTROL },
        { "LEFT", GLFW_KEY_LEFT }, { "RIGHT", GLFW_KEY_RIGHT }, { "UP", GLFW_KEY_UP }, { "DOWN", GLFW_KEY_DOWN },
        { "A", GLFW_KEY_A }, { "B", GLFW_KEY_B }, { "C", GLFW_KEY_C }, { "D", GLFW_KEY_D }, { "E", GLFW_KEY_E },
        { "F", GLFW_KEY_F }, { "G", GLFW_KEY_G }, { "H", GLFW_KEY_H }, { "I", GLFW_KEY_I }, { "J", GLFW_KEY_J },
        { "K", GLFW_KEY_K }, { "L", GLFW_KEY_L }, { "M", GLFW_KEY_M }, { "N", GLFW_KEY_N }, { "O", GLFW_KEY_O },
        { "P", GLFW_KEY_P }, { "Q", GLFW_KEY_Q }, { "R", GLFW_KEY_R }, { "S", GLFW_KEY_S }, { "T", GLFW_KEY_T },
        { "U", GLFW_KEY_U }, { "V", GLFW_KEY_V }, { "W", GLFW_KEY_W }, { "X", GLFW_KEY_X }, { "Y", GLFW_KEY_Y }, { "Z", GLFW_KEY_Z }
    };

    const auto keyIt = keyMap.find(ToUpper(name));
    return keyIt != keyMap.end() ? keyIt->second : GLFW_KEY_UNKNOWN;
}

int InputManager::ParseMouseButtonName(const std::string& buttonName)
{
    static const std::unordered_map<std::string, int> mouseMap =
    {
        { "LEFT", GLFW_MOUSE_BUTTON_LEFT },
        { "RIGHT", GLFW_MOUSE_BUTTON_RIGHT },
        { "MIDDLE", GLFW_MOUSE_BUTTON_MIDDLE },
        { "X1", GLFW_MOUSE_BUTTON_4 },
        { "X2", GLFW_MOUSE_BUTTON_5 }
    };

    const auto buttonIt = mouseMap.find(ToUpper(buttonName));
    return buttonIt != mouseMap.end() ? buttonIt->second : 0;
}

int InputManager::ParseControllerButtonName(const std::string& buttonName)
{
    static const std::unordered_map<std::string, int> buttonMap =
    {
        { "A", SDL_CONTROLLER_BUTTON_A }, { "B", SDL_CONTROLLER_BUTTON_B },
        { "X", SDL_CONTROLLER_BUTTON_X }, { "Y", SDL_CONTROLLER_BUTTON_Y },
        { "CROSS", SDL_CONTROLLER_BUTTON_A }, { "CIRCLE", SDL_CONTROLLER_BUTTON_B },
        { "SQUARE", SDL_CONTROLLER_BUTTON_X }, { "TRIANGLE", SDL_CONTROLLER_BUTTON_Y },
        { "BACK", SDL_CONTROLLER_BUTTON_BACK }, { "SELECT", SDL_CONTROLLER_BUTTON_BACK },
        { "START", SDL_CONTROLLER_BUTTON_START }, { "GUIDE", SDL_CONTROLLER_BUTTON_GUIDE },
        { "LEFT_STICK", SDL_CONTROLLER_BUTTON_LEFTSTICK }, { "RIGHT_STICK", SDL_CONTROLLER_BUTTON_RIGHTSTICK },
        { "LEFT_SHOULDER", SDL_CONTROLLER_BUTTON_LEFTSHOULDER }, { "L1", SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
        { "RIGHT_SHOULDER", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER }, { "R1", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
        { "DPAD_UP", SDL_CONTROLLER_BUTTON_DPAD_UP }, { "DPAD_DOWN", SDL_CONTROLLER_BUTTON_DPAD_DOWN },
        { "DPAD_LEFT", SDL_CONTROLLER_BUTTON_DPAD_LEFT }, { "DPAD_RIGHT", SDL_CONTROLLER_BUTTON_DPAD_RIGHT }
    };

    const auto buttonIt = buttonMap.find(ToUpper(buttonName));
    return buttonIt != buttonMap.end() ? buttonIt->second : -1;
}

int InputManager::ParseControllerAxisName(const std::string& axisName)
{
    static const std::unordered_map<std::string, int> axisMap =
    {
        { "LEFT_X", SDL_CONTROLLER_AXIS_LEFTX }, { "LEFT_Y", SDL_CONTROLLER_AXIS_LEFTY },
        { "RIGHT_X", SDL_CONTROLLER_AXIS_RIGHTX }, { "RIGHT_Y", SDL_CONTROLLER_AXIS_RIGHTY },
        { "LT", SDL_CONTROLLER_AXIS_TRIGGERLEFT }, { "RT", SDL_CONTROLLER_AXIS_TRIGGERRIGHT },
        { "LEFT_TRIGGER", SDL_CONTROLLER_AXIS_TRIGGERLEFT }, { "RIGHT_TRIGGER", SDL_CONTROLLER_AXIS_TRIGGERRIGHT }
    };

    const auto axisIt = axisMap.find(ToUpper(axisName));
    return axisIt != axisMap.end() ? axisIt->second : -1;
}

std::string InputManager::ToUpper(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

float InputManager::ClampAxis(float value)
{
    return std::clamp(value, -1.0f, 1.0f);
}

float InputManager::ChooseDominantAxis(float currentValue, float candidateValue)
{
    return std::fabs(candidateValue) > std::fabs(currentValue) ? candidateValue : currentValue;
}
