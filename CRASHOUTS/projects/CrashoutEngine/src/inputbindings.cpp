#include "InputBindings.h"
#include "InputManager.h"
#include "Key.h"
#include "mono/metadata/loader.h"
#include "entitymanager.h"

using namespace CrashoutsEngine;
using namespace Component;


static bool InternalIsKeyPressed(int key)
{

    return InputManager::IsKeyPressed(static_cast<Key>(key));
}

void CrashoutsEngine::Bindings::RegisterInputBindings()
{
    mono_add_internal_call("CrashoutEngine.Input::IsKeyPressed", reinterpret_cast<const void*>(InternalIsKeyPressed));
}
