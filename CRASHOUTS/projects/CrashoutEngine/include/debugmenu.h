/*!
@file       debugmenu.h
@author     mohamadamirakmal.b@digipen.edu
@date       14/09/2025

@brief      This header defines the user interface (UI) panels and debug menu
            functionality for Crashout Engine's ImGUI system. It provides
            declarations for initialization, shutdown, rendering, and debugging
            panels, as well as a UI state structure to manage panel toggles.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

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
namespace DebugReflection { class PanelRegistry; }

namespace UI {

    /*!
    @struct UIState
    @brief Stores runtime toggles and layout states for the UI.
    */
    struct UIState {
        bool lockLayout = false;
        bool showPerf = false;
        bool showConsole = true;
        bool showInspector = true;
        bool showECS = true;
        bool showAssets = true;
        bool showGameState = true;
        bool showGameScene = true;
        bool showPhysics = true;
        bool showAudio = true;
        bool drawCollOverlay = true;
        bool showEngineControl = true;
        bool drawQuadTree = false;
        bool showProjectManager = true;

        bool freezeOnCollision = false;
        bool showAnimation = true; 
        bool showSpriteSheet = true;
        bool showVisualScripting = false;
        bool drawAllColliders = false;   // draw wireframes for every collider, not just colliding ones
        bool drawVelocities = false;    // draw velocity arrows if RigidBodyComponent exists
        float overlayThickness = .35f;  // line thickness for wireframes
        float fontScale = 1.0f;         // Set the font scale

        bool drawGameDebug = false;     // Toggle for script-based debug drawing (circles/lines)
    };

    // -- Lifecycle ----------------------------------------------------

    /*!
    @brief Initializes ImGui with a GLFW window.
    @param window Pointer to the GLFW window handle.
    */
    void Init(GLFWwindow* window, UI::UIState& uiState);

    /*!
    @brief Helper to check if script debug drawing is enabled.
    */
    bool IsGameDebugDrawEnabled();
    
    /*!
    @brief Shuts down ImGui and releases resources.
    */
    void Shutdown(UI::UIState& uiState);

    // -- Rendering entry points ---------------------------------------

    /*!
    @brief Renders the entire debug menu in one call.
    @param profiler Frame profiler for performance data.
    @param ecs Reference to ECS system.
    @param phys Reference to PhysicsHandler.
    @param entities List of entities currently tracked.
    @param selected Currently selected entity.
    */
    void Render(FrameProfiler& profiler, CrashoutsEngine::Entity& selected, UIState& ui);

    // -- Helper Functions ---------------------------------------------
    /*!
    @brief Starts a new ImGui frame. Must be called once per frame before rendering.
    */
    void BeginFrame();
    /*!
    @brief Ends the current ImGui frame and submits draw data.
    */
    void EndFrame();

    /*!
     @brief Renders the debugmenu panel.
     @param ui UI state toggles.
    */
    void RenderDebugMenuBar(UIState& ui);

    /*!
    @brief Renders all UI panels and overlays in sequence.
    @param profiler Frame profiler reference.
    @param ecs ECS reference.
    @param phys Physics handler reference.
    @param ui UI state toggles.
    @param entities List of ECS entities.
    @param selected Currently selected entity.
    */
    void RenderAll(FrameProfiler& profiler, UIState& ui, CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);

    void SetDarkGrayTheme();

} // namespace UI