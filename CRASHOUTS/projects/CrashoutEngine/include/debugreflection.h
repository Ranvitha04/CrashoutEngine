/*!
@file       debugreflection.h
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

@brief      Lightweight registry hooks for the Debug Menu. Provides:
            - IComponentAdapter + ComponentRegistry: ECS inspectors (add/remove/edit)
            - IGameStateProvider setter/getter: data-driven Game State panel
            - PanelRegistry: register custom ImGui panels at runtime
            - IPhysicsProvider + PhysicsRegistry: physics debug controls

These APIs let the debug UI be data-driven without hardcoding per-project UI.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

//__________________________________________________________________________*/

namespace CrashoutsEngine {
    class ECS;
    using Entity = unsigned;
}

namespace DebugReflection {

    // -------------------------------------------------------------------------
    // Component Adapters
    // -------------------------------------------------------------------------

    /*!
     @struct IComponentAdapter
     @brief Interface for ECS component adapters.
     @details
     Each adapter provides operations to check if a component exists on an entity,
     add or remove it, and render its inspector UI. Implementations are registered
     with the `ComponentRegistry` and exposed automatically to the debug UI.
    */
    struct IComponentAdapter {
        virtual ~IComponentAdapter() = default;

        /*! @brief Returns the component's display name. */
        virtual const char* name() const = 0;

        /*!
         @brief Checks if the entity has this component.
         @param ecs Reference to the ECS system.
         @param e   Target entity.
         @return True if the entity has the component.
        */
        virtual bool has(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) const = 0;

        /*!
         @brief Adds the component to an entity.
         @param ecs Reference to the ECS system.
         @param e   Target entity.
        */
        virtual void add(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) = 0;

        /*!
         @brief Removes the component from an entity.
         @param ecs Reference to the ECS system.
         @param e   Target entity.
        */
        virtual void remove(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) = 0;

        /*!
         @brief Draws the ImGui inspector UI for this component.
         @param ecs Reference to the ECS system.
         @param e   Target entity.
        */
        virtual void drawInspector(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) = 0;
    };

    /*!
     @class ComponentRegistry
     @brief Global registry for ECS component adapters.
     @details
     Stores all registered `IComponentAdapter` implementations. These are used
     by the debug UI to display component inspector panels, add/remove buttons,
     and more.
    */
    class ComponentRegistry {
    public:
        /*! @brief Returns the singleton instance. */
        static ComponentRegistry& instance();

        /*!
         @brief Registers a new component adapter.
         @param adapter Pointer to the adapter (ownership is retained by caller).
        */
        void registerAdapter(IComponentAdapter* adapter);

        /*! @brief Returns a list of all registered adapters. */
        const std::vector<IComponentAdapter*>& all() const { return list; }

        void clear() { list.clear(); }

    private:
        std::vector<IComponentAdapter*> list;
    };

    // -------------------------------------------------------------------------
    // Game State Provider
    // -------------------------------------------------------------------------

    /*!
     @struct IGameStateProvider
     @brief Interface for exposing runtime game state to the debug panel.
     @details
     Implement this interface to expose game state data (e.g., current/next
     states) and provide control over state changes from the debug UI.
    */
    struct IGameStateProvider {
        virtual ~IGameStateProvider() = default;

        /*! @brief Returns the ID of the current game state. */
        virtual int current() const = 0;

        /*! @brief Returns the ID of the previous game state. */
        virtual int previous() const = 0;

        /*! @brief Returns the ID of the next game state. */
        virtual int next() const = 0;

        /*!
         @brief Changes the current game state to the specified ID.
         @param id ID of the target game state.
        */
        virtual void changeTo(int id) = 0;

        /*!
         @brief Returns a list of available game states.
         @return A vector of `(name, id)` pairs.
        */
        virtual const std::vector<std::pair<const char*, int>>& states() const = 0;
    };

    /*!
     @brief Sets the global game state provider.
     @param p Pointer to the provider instance.
     @note Should be called once at game startup.
    */
    void SetGameStateProvider(IGameStateProvider* p);

    /*!
     @brief Retrieves the global game state provider.
     @return Pointer to the provider, or nullptr if none is set.
    */
    IGameStateProvider* GetGameStateProvider();

    // -------------------------------------------------------------------------
    // Panel Registry
    // -------------------------------------------------------------------------

    /*! @brief Function type used for rendering custom ImGui panels. */
    using PanelFn = std::function<void()>;

    /*!
     @struct Panel
     @brief Represents a single registered debug panel.
     @details
     A panel consists of a display name, an optional toggle flag (for visibility),
     and a rendering callback.
    */
    struct Panel {
        std::string name;     //!< Display name shown in the UI.
        bool* toggleFlag;     //!< Optional toggle state pointer for visibility.
        PanelFn render;       //!< Function used to render the panel's contents.
    };

    /*!
     @class PanelRegistry
     @brief Registry for custom debug panels.
     @details
     Panels can be registered at runtime and are displayed automatically
     in the debug UI. If a toggle flag is provided, the panel will also
     appear in the toggles window.
    */
    class PanelRegistry {
    public:
        /*! @brief Returns the singleton instance. */
        static PanelRegistry& instance();

        /*!
         @brief Registers a new custom panel.
         @param name       Display name of the panel.
         @param fn         Rendering callback for the panel.
         @param toggleFlag Optional pointer to a visibility toggle variable.
        */
        void registerPanel(const std::string& name, PanelFn fn, bool* toggleFlag = nullptr);

        /*! @brief Returns all registered panels. */
        const std::vector<Panel>& panels() const { return list; }

        void clear() { list.clear(); }

    private:
        std::vector<Panel> list;
    };

    // -------------------------------------------------------------------------
    // Physics Provider
    // -------------------------------------------------------------------------

    /*!
     @struct IPhysicsProvider
     @brief Interface for registering physics debug tools.
     @details
     Implement this interface to expose physics-related debug UIs, such as
     global gravity control or collision system visualization.
    */
    struct IPhysicsProvider {
        virtual ~IPhysicsProvider() = default;

        /*! @brief Returns the provider's display name. */
        virtual const char* name() const = 0;

        /*!
         @brief Renders the provider's debug UI.
         @param ecs Reference to the ECS.
        */
        virtual void draw(CrashoutsEngine::ECS& ecs) = 0;
    };

    /*!
     @class PhysicsRegistry
     @brief Registry for physics debug providers.
     @details
     Manages all registered `IPhysicsProvider` implementations and exposes
     them to the debug UI.
    */
    class PhysicsRegistry {
    public:
        /*! @brief Returns the singleton instance. */
        static PhysicsRegistry& instance();

        /*!
         @brief Registers a new physics provider.
         @param p Pointer to the provider (caller retains ownership).
        */
        void registerProvider(IPhysicsProvider* p);

        /*! @brief Returns all registered providers. */
        const std::vector<IPhysicsProvider*>& all() const;

        void clear() { providers_.clear(); }

    private:
        std::vector<IPhysicsProvider*> providers_;
    };

} // namespace DebugReflection