#include "InputBindings.h"
#include "InputManager.h"
#include "Key.h"
#include "mono/metadata/loader.h"
#include "entitymanager.h"

using namespace CrashoutsEngine;
using namespace Component;


static bool InternalIsKeyPressed(int key)
{
    std::cout << "[Mono] Input binding registered OK\n";
    return InputManager::IsKeyPressed(key);
}

void CrashoutsEngine::Bindings::RegisterInputBindings()
{
    mono_add_internal_call("CrashoutsEngine.Input::IsKeyPressed", reinterpret_cast<const void*>(InternalIsKeyPressed));
    std::cout << "[Mono] Registered: CrashoutsEngine.Input::IsKeyPressed" << std::endl;
}
