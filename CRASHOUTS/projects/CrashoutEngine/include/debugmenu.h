/*!
@file       debugmenu.h
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

@brief      This header defines the user interface (UI) panels and debug menu
            functionality for Crashout Engine's ImGUI system. It provides
            declarations for initialization, shutdown, rendering, and debugging
            panels, as well as a UI state structure to manage panel toggles.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include <GLFW/glfw3.h>
#include <vector>

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------
/*!
 @class FrameProfiler
 @brief Forward declaration for the engine's frame profiler.
 @details
 Used to collect timing and performance statistics per frame.
 The full definition is provided in the performance profiling module.
*/
class FrameProfiler;
namespace CrashoutsEngine { 
    class ECS;
    using Entity = unsigned;
}
namespace Physics { class PhysicsHandler; }
namespace Collision { struct CollisionEvent; }

namespace UI {

    /*!
    @struct UIState
    @brief Stores runtime toggles and layout states for the UI.
    */
    struct UIState {
        bool lockLayout = false;
        bool showPerf = true;
        bool showConsole = true;
        bool showECS = true;
        bool showGameState = true;
        bool showPhysics = true;
        bool drawCollOverlay = true;
    };

    // -- Lifecycle ----------------------------------------------------

    /*!
    @brief Initializes ImGui with a GLFW window.
    @param window Pointer to the GLFW window handle.
    */
    void Init(GLFWwindow* window);
    /*!
    @brief Shuts down ImGui and releases resources.
    */
    void Shutdown();

    // -- Rendering entry points ---------------------------------------

    /*!
    @brief Renders the entire debug menu in one call.
    @param profiler Frame profiler for performance data.
    @param ecs Reference to ECS system.
    @param phys Reference to PhysicsHandler.
    @param entities List of entities currently tracked.
    @param selected Currently selected entity.
    */
    void Render(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);

    // -- Helper Functions ---------------------------------------------
    /*!
    @brief Starts a new ImGui frame. Must be called once per frame before rendering.
    */
    void BeginFrame();
    /*!
    @brief Ends the current ImGui frame and submits draw data.
    */
    void EndFrame();

    // -- Panels (Easier to implement more Panel)-----------------------
    /*!
    @brief Renders the ECS debug panel.
    @param profiler Frame profiler reference.
    @param ecs ECS reference.
    @param entities List of ECS entities.
    @param selected Currently selected entity.
    */
    void RenderEcsPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected, UIState& ui);
    
    /*!
    @brief Renders the performance profiling panel.
    @param profiler Frame profiler reference.
    @param ui UI state toggles.
    */
    void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui);
    
    /*!
    @brief Renders the console log panel.
    @param ui UI state toggles.
    */
    void RenderConsolePanel(UIState& ui);

    void RenderGameStatePanel(UIState& ui);
    
    /*!
    @brief Renders the physics and collision panel.
    @param profiler Frame profiler reference.
    @param ecs ECS reference.
    @param phys Physics handler reference.
    @param ui UI state toggles.
    */
    void RenderPhysicsPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui);
    
    /*!
    @brief Draws collision overlays in the viewport.
    @param ecs ECS reference.
    @param events List of collision events.
    @param ui UI state toggles.
    */
    void DrawCollisionOverlay(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>& events, const UIState& ui);

    /*!
    @brief Renders all UI panels and overlays in sequence.
    @param profiler Frame profiler reference.
    @param ecs ECS reference.
    @param phys Physics handler reference.
    @param ui UI state toggles.
    @param entities List of ECS entities.
    @param selected Currently selected entity.
    */
    void RenderAll(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);

} // namespace UI