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

void CrashoutsEngine::Bindings::RegisterInputBindings()
{
    mono_add_internal_call("CrashoutsEngine.Input::InternalIsKeyPressed", reinterpret_cast<const void*>(InternalIsKeyPressed));
}
