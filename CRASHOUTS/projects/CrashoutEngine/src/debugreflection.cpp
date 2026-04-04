/*!
@file   debugreflection.cpp
@author Mohamad Amir Akmal (58%), Ranvitha (42%)
@date   25/09/2025

@brief  Static storage for registries and provider pointer.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "debugreflection.h"

//----------------------------------------------------------------------------- file-local state & helpers
/*!
 @brief Static storage for registries and provider pointer.

 Each registry is a Meyers singleton so it�s safe to call from any module
 without manual initialization. The Game State provider is a single pointer
 the game installs once at startup.
*/
namespace DebugReflection {

    // -- Component Registry ---------------------------------------------------

    /*!
     @brief Retrieves the singleton instance of the ComponentRegistry.
     @return Reference to the global ComponentRegistry instance.
     @note Uses a Meyers singleton to ensure safe global access without manual initialization.
    */
    ComponentRegistry& ComponentRegistry::instance() {
        static ComponentRegistry R;
        return R;
    }

    /*!
     @brief Registers a new ECS component adapter with the registry.
     @param adapter Pointer to an adapter implementing `IComponentAdapter`.
     @note Registered adapters are used to render component add/remove/inspect UIs dynamically.
    */
    void ComponentRegistry::registerAdapter(IComponentAdapter* adapter) {
        list.push_back(adapter);
    }

    // -- Panel Registry -------------------------------------------------------

    /*!
     @brief Retrieves the singleton instance of the PanelRegistry.
     @return Reference to the global PanelRegistry instance.
    */
    PanelRegistry& PanelRegistry::instance() {
        static PanelRegistry R;
        return R;
    }

    /*!
     @brief Registers a new custom ImGui panel in the registry.
     @param name       Name of the panel as shown in the UI.
     @param fn         Function that renders the panel�s ImGui contents.
     @param toggleFlag Optional pointer to a boolean controlling panel visibility.
     @note Registered panels can be toggled on-demand and are integrated into the debug UI system.
    */
    void PanelRegistry::registerPanel(const std::string& name, PanelFn fn, bool* toggleFlag) {
        Panel p{ name, toggleFlag, std::move(fn) };
        list.push_back(std::move(p));
    }

    // -- Physics Registry -----------------------------------------------------

    /*!
     @brief Retrieves the singleton instance of the PhysicsRegistry.
     @return Reference to the global PhysicsRegistry instance.
    */
    PhysicsRegistry& PhysicsRegistry::instance() {
        static PhysicsRegistry R;
        return R;
    }

    /*!
     @brief Registers a new physics provider with the registry.
     @param p Pointer to a class implementing `IPhysicsProvider`.
     @note Providers expose physics debug interfaces (e.g., global gravity) to the debug menu.
    */
    void PhysicsRegistry::registerProvider(IPhysicsProvider* p) {
        if (!p) return;
        providers_.push_back(p);
    }

    /*!
     @brief Returns all registered physics providers.
     @return Constant reference to a vector of registered `IPhysicsProvider*`.
    */
    const std::vector<IPhysicsProvider*>& PhysicsRegistry::all() const {
        return providers_;
    }

} // namespace DebugReflection
