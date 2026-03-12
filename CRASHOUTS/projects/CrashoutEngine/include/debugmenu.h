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
#include <imgui.h>
#include <vector>
#include <string>
class FrameProfiler;

namespace CrashoutsEngine {
    class ECS;
    using Entity = unsigned;
}

namespace Physics { class PhysicsHandler; }
namespace Collision { struct CollisionEvent; }
namespace DebugReflection { class PanelRegistry; }

namespace UI {

    enum class VSNodeType
    {
        Start = 0,
        Move,
        Attack,
        Wait,
        Branch,
        ScriptCall,
        BoolConstant,
        IntConstant,
        FloatConstant,
        GetVariable,
        SetVariable,
        Add,
        Subtract,
        Multiply,
        Divide,
        Equal,
        NotEqual,
        GreaterThan,
        LessThan,
        GreaterEqual,
        LessEqual,
        And,
        Or,
        Not
    };

    enum class VSPinKind
    {
        Input = 0,
        Output
    };

    enum class VSDataType
    {
        Exec = 0,
        Bool,
        Int,
        Float,
        String,
        Entity,
        Vec3
    };

    struct VSPin
    {
        int id = 0;
        std::string name;
        VSPinKind kind = VSPinKind::Input;
        VSDataType type = VSDataType::Exec;
    };

    struct VSNode
    {
        int id = 0;
        VSNodeType type = VSNodeType::Start;
        std::string title;
        ImVec2 pos{ 0.0f, 0.0f };
        ImVec2 size{ 160.0f, 90.0f };

        std::vector<VSPin> inputs;
        std::vector<VSPin> outputs;

        std::string scriptName;
        std::string functionName;

        float waitTime = 1.0f;
        bool boolValue = false;
        int intValue = 0;
        float floatValue = 0.0f;
        std::string variableName;
    };

    struct VSLink
    {
        int fromNode = 0;
        int fromPin = 0;
        int toNode = 0;
        int toPin = 0;
    };

    struct VisualScriptingGraph
    {
        std::vector<VSNode> nodes;
        std::vector<VSLink> links;
        int nextNodeId = 1;
        int nextPinId = 1;
    };

    struct UIState
    {
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
        bool drawAllColliders = false;
        bool drawVelocities = false;
        float overlayThickness = .35f;
        float fontScale = 1.0f;

        VisualScriptingGraph visualGraph;
        bool drawGameDebug = false;
        bool boolValue = false;
        int intValue = 0;
        float floatValue = 0.0f;
        std::string variableName;
        std::string openedVisualScriptPath;
        bool openVisualScriptPanel{ false };
    };

    UIState* GetUIState();

    void Init(GLFWwindow* window, UIState& uiState);
    bool IsGameDebugDrawEnabled();
    void Shutdown(UIState& uiState);

    void Render(FrameProfiler& profiler, CrashoutsEngine::Entity& selected, UIState& ui);
    void BeginFrame();
    void EndFrame();

    void RenderDebugMenuBar(UIState& ui);
    void RenderAll(FrameProfiler& profiler,
        UIState& ui,
        CrashoutsEngine::ECS& ecs,
        std::vector<CrashoutsEngine::Entity>& entities,
        CrashoutsEngine::Entity& selected);

    VisualScriptingGraph& GetVisualScriptingGraph();
    void ClearVisualScriptingGraph();

    bool SaveVisualScriptingGraph(const std::string& path, const VisualScriptingGraph& graph);
    bool LoadVisualScriptingGraph(const std::string& path, VisualScriptingGraph& graph);

    void SetDarkGrayTheme();

} // namespace UI