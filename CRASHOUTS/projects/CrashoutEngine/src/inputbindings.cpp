#include "InputBindings.h"
#include "InputManager.h"
#include "Key.h"
#include "mono/metadata/loader.h"
#include "entitymanager.h"

using namespace CrashoutsEngine;
using namespace Component;


static bool InternalIsKeyPressed(int key)
{
    return InputManager::IsKeyPressed(key);
}

static bool InternalIsKeyTriggered(int key)
{
    return InputManager::IsKeyTriggered(key);
}

void CrashoutsEngine::Bindings::RegisterInputBindings()
{
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyPressed", reinterpret_cast<const void*>(InternalIsKeyPressed));
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyTriggered", reinterpret_cast<const void*>(InternalIsKeyTriggered));
}
