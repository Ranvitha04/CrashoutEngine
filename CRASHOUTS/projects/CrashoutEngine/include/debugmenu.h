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
namespace DebugReflection { class PanelRegistry; }

namespace UI {

    /*!
    @struct UIState
    @brief Stores runtime toggles and layout states for the UI.
    */
    struct UIState {
        bool lockLayout = false;
        bool showPerf = true;
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

        bool freezeOnCollision = false;
        bool showAnimation = true; 
        bool showSpriteSheet = false;

        bool drawAllColliders = true;   // draw wireframes for every collider, not just colliding ones
        bool drawVelocities = false;    // draw velocity arrows if RigidBodyComponent exists
        float overlayThickness = .35f;   // line thickness for wireframes
    };

    // -- Lifecycle ----------------------------------------------------

    static std::string currSceneName = "";

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
    void Render(FrameProfiler& profiler, Physics::PhysicsHandler& phys, CrashoutsEngine::Entity& selected, UIState& ui);

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
    @param ecs ECS reference.
    @param entities List of ECS entities.
    @param selected Currently selected entity.
    */
    static void RenderEcsPanel(CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected, UIState& ui);

    /*!
    @brief Renders the Asset Browser debug panel.
    @param ui UI state toggles.
    @param root
    @param outSelectedAsset
    */
    static void RenderAssetBrowserPanel(UIState& ui, const char* root, std::string& outSelectedAsset);

    /*!
    @brief Renders the Inspector debug panel.
    @param ecs ECS reference.
    @param selected Currently selected entity.
    @param ui UI state toggles.
    */
    static void RenderInspectorPanel(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity selected, UIState& ui);

    /*!
    @brief Renders the performance profiling panel.
    @param profiler Frame profiler reference.
    @param ui UI state toggles.
    */
    static void RenderPerformancePanel(FrameProfiler& profiler, UIState& ui);
    
    /*!
    @brief Renders the console log panel.
    @param ui UI state toggles.
    */
    static void RenderConsolePanel(UIState& ui);

    /*!
     @brief Renders the gameState panel
     @param state gameState reference.
     @param ui UI state toggles.
    */
    static void RenderGameStatePanel(UIState& ui);

    /*!
     @brief Renders the Scenes panel
     @param ui UI state toggles.
    */
    static void RenderGameScenePanel(UIState& ui);
    
    /*!
    @brief Renders the physics and collision panel.
    @param profiler Frame profiler reference.
    @param ecs ECS reference.
    @param phys Physics handler reference.
    @param ui UI state toggles.
    */
    static void RenderPhysicsPanel(FrameProfiler& profiler, CrashoutsEngine::ECS& ecs, Physics::PhysicsHandler& phys, UIState& ui);
    
    /*!
     @brief Renders the audio panel.
     @param profiler Frame profiler reference.
     @param ecs ECS reference.
     @param ui UI state toggles.
    */
    static void RenderAudioPanel(FrameProfiler&, CrashoutsEngine::ECS& ecs, UIState& ui);

    /*!
     @brief Renders the debugmenu panel.
     @param ui UI state toggles.
    */
    void RenderDebugMenuBar(UIState& ui);

    /*!
     @brief Renders the Scene control bar on top of game viewport
    */
    static void RenderEngineStateBar(UIState& ui);

    /*!
    @brief Renders all UI panels and overlays in sequence.
    @param profiler Frame profiler reference.
    @param ecs ECS reference.
    @param phys Physics handler reference.
    @param ui UI state toggles.
    @param entities List of ECS entities.
    @param selected Currently selected entity.
    */
    void RenderAll(FrameProfiler& profiler, Physics::PhysicsHandler& phys, UIState& ui, CrashoutsEngine::ECS& ecs, std::vector<CrashoutsEngine::Entity>& entities, CrashoutsEngine::Entity& selected);

    void SetDarkGrayTheme();

} // namespace UI