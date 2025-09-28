/*!
@file       editorregistry.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

Implements lightweight reflection registries used by the Debug Menu:
- ComponentRegistry: lists ECS component adapters for add/remove/inspect
- PanelRegistry:     lists on-demand ImGui panels
- GameStateProvider: global pointer to a runtime-installed provider
- PhysicsRegistry:   lists physics debug providers (e.g., global gravity UI)

These registries allow the UI to be data-driven instead of hardcoded.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "editorregistry.h"

//----------------------------------------------------------------------------- file-local state & helpers
/*!
 @brief Static storage for registries and provider pointer.

 Each registry is a Meyers singleton so it’s safe to call from any module
 without manual initialization. The Game State provider is a single pointer
 the game installs once at startup.
*/
namespace EditorRegistry {

    // COMPONENT
    ComponentRegistry& ComponentRegistry::instance() {
        static ComponentRegistry R;
        return R;
    }
    void ComponentRegistry::registerAdapter(IComponentAdapter* adapter) {
        list.push_back(adapter);
    }

    // GSM
    static IGameStateProvider* g_gsProvider = nullptr;

    void SetGameStateProvider(IGameStateProvider* p) { g_gsProvider = p; }
    IGameStateProvider* GetGameStateProvider() { return g_gsProvider; }

    PanelRegistry& PanelRegistry::instance() {
        static PanelRegistry R;
        return R;
    }
    void PanelRegistry::registerPanel(const std::string& name, PanelFn fn, bool* toggleFlag) {
        Panel p{ name, toggleFlag, std::move(fn) };
        list.push_back(std::move(p));
    }

    // PHYSICS
    PhysicsRegistry& PhysicsRegistry::instance() {
        static PhysicsRegistry R;
        return R;
    }
    void PhysicsRegistry::registerProvider(IPhysicsProvider* p) {
        if (!p) return;
        providers_.push_back(p);
    }
    const std::vector<IPhysicsProvider*>& PhysicsRegistry::all() const {
        return providers_;
    }

} // namespace EditorRegistry