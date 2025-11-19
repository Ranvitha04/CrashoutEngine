/*!
@file		inputbindings.cpp
@author     m.biniswandi@digipen.edu
@date       06/11/2025
@brief		Definition of input bindings functions
*/
#include "inputbindings.h"
#include "inputmanager.h"
#include "Key.h"
#include "mono/metadata/loader.h"
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

void CrashoutsEngine::Bindings::RegisterInputBindings()
{
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyPressed", reinterpret_cast<const void*>(InternalIsKeyPressed));
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyTriggered", reinterpret_cast<const void*>(InternalIsKeyTriggered));
	mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyHeld", reinterpret_cast<const void*>(InternalIsKeyHeld));
}
