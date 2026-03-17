/*!
@file		inputbindings.cpp
@author     Shahrin Idzuan
@author     Renfred (RTIS)
@author     Renfred Lee
@date       06/11/2025
@brief		Definition of input bindings functions

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "inputbindings.h"
#include "inputmanager.h"
#include "Key.h"
#include "mono/metadata/loader.h"
#include "mono/metadata/object.h"
#include "entitymanager.h"

using namespace CrashoutsEngine;

static bool InternalIsKeyPressed(int key)
{
    return InputManager::IsKeyPressed(key);
}

static bool InternalIsKeyTriggered(int key)
{
    return InputManager::IsKeyTriggered(key);
}

static bool InternalIsKeyHeld(int key)
{
    return InputManager::IsKeyHeld(key);
}

static float InternalGetAxis(MonoString* axisName)
{
    if (axisName == nullptr)
    {
        return 0.0f;
    }

    char* utf8 = mono_string_to_utf8(axisName);
    const std::string axis = utf8 != nullptr ? utf8 : "";
    if (utf8 != nullptr)
    {
        mono_free(utf8);
    }

    return InputManager::GetAxis(axis);
}

static bool InternalGetAction(MonoString* actionName)
{
    if (actionName == nullptr)
    {
        return false;
    }

    char* utf8 = mono_string_to_utf8(actionName);
    const std::string action = utf8 != nullptr ? utf8 : "";
    if (utf8 != nullptr)
    {
        mono_free(utf8);
    }

    return InputManager::GetAction(action);
}

static bool InternalGetActionTriggered(MonoString* actionName)
{
    if (actionName == nullptr)
    {
        return false;
    }

    char* utf8 = mono_string_to_utf8(actionName);
    const std::string action = utf8 != nullptr ? utf8 : "";
    if (utf8 != nullptr)
    {
        mono_free(utf8);
    }

    return InputManager::GetActionTriggered(action);
}

static bool InternalGetActionReleased(MonoString* actionName)
{
    if (actionName == nullptr)
    {
        return false;
    }

    char* utf8 = mono_string_to_utf8(actionName);
    const std::string action = utf8 != nullptr ? utf8 : "";
    if (utf8 != nullptr)
    {
        mono_free(utf8);
    }

    return InputManager::GetActionReleased(action);
}

static void InternalGetMousePosition(CEVec2* outPos)
{
    *outPos = InputManager::GetMousePosition();
}

static void InternalGetMouseWorldPosition(CEVec2* outPos)
{
    *outPos = InputManager::GetMouseWorldPos();
}

static bool InternalIsMouseButtonDown(int button)
{
    return InputManager::IsMouseButtonPressed(button);
}

static bool InternalIsMouseButtonTriggered(int button)
{
    return InputManager::IsMouseButtonTriggered(button);
}

static bool InternalIsMouseButtonReleased(int button)
{
    return InputManager::IsMouseButtonReleased(button);
}

static void InternalGetGameMousePos(CEVec2* outPos) {
    *outPos = InputManager::GetGameMousePos(); // Uses ImGui-relative mouse
}

static void InternalGetGameWindowSize(CEVec2* outSize) {
    *outSize = InputManager::GetGameWindowSize(); // Uses Viewport dimensions
}

void CrashoutsEngine::Bindings::RegisterInputBindings()
{
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyPressed", reinterpret_cast<const void*>(InternalIsKeyPressed));
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyTriggered", reinterpret_cast<const void*>(InternalIsKeyTriggered));
	mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyHeld", reinterpret_cast<const void*>(InternalIsKeyHeld));
    mono_add_internal_call("CrashoutsEngine.Input::InternalGetAxis", reinterpret_cast<const void*>(InternalGetAxis));
    mono_add_internal_call("CrashoutsEngine.Input::InternalGetAction", reinterpret_cast<const void*>(InternalGetAction));
    mono_add_internal_call("CrashoutsEngine.Input::InternalGetActionTriggered", reinterpret_cast<const void*>(InternalGetActionTriggered));
    mono_add_internal_call("CrashoutsEngine.Input::InternalGetActionReleased", reinterpret_cast<const void*>(InternalGetActionReleased));
    mono_add_internal_call("CrashoutsEngine.Input::InternalGetMousePosition", reinterpret_cast<const void*>(InternalGetMousePosition));
    mono_add_internal_call("CrashoutsEngine.Input::InternalGetMouseWorldPosition", reinterpret_cast<const void*>(InternalGetMouseWorldPosition));
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsMouseButtonDown", reinterpret_cast<const void*>(InternalIsMouseButtonDown));
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsMouseButtonTriggered", reinterpret_cast<const void*>(InternalIsMouseButtonTriggered));
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsMouseButtonReleased", reinterpret_cast<const void*>(InternalIsMouseButtonReleased));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::GetGameMousePos", reinterpret_cast<const void*>(InternalGetGameMousePos));
    mono_add_internal_call("CrashoutsEngine.InternalCalls::GetGameWindowSize", reinterpret_cast<const void*>(InternalGetGameWindowSize));
}
 
