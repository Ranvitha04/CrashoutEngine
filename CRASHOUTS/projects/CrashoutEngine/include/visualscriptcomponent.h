/*!
@file       visualscriptcomponent.h
@author     Afiq
@date       13/03/2026
@brief      ECS component data for visual scripting graphs and runtime variables.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once
#include <string>
#include "debugmenu.h"
#include <memory>
#include <unordered_map>
namespace Component {
    /*!
    @struct VisualScriptComponent
    @brief Stores a graph reference/path and runtime variable state for visual scripting.
    */
    /** @brief VisualScriptComponent */
    struct VisualScriptComponent {
        bool enabled{ true };
        std::string graphPath{ "" };
        std::unordered_map<std::string, bool> boolVariables;
        std::unordered_map<std::string, int> intVariables;
        std::unordered_map<std::string, float> floatVariables;
        std::shared_ptr<UI::VisualScriptingGraph> graph{ nullptr };
        bool graphLoaded{ false };
        int currentNodeId{ 0 };
        float waitTimer{ 0.0f };

        /*!
        @brief Construct a visual script component with default values.
        */
        VisualScriptComponent() = default;

        /*!
        @brief Construct a visual script component with an enabled state and optional graph path.
        @param enabled Whether the visual script is enabled.
        @param path Path to the graph asset.
        */
        VisualScriptComponent(bool enabled, const std::string& path = "")
            : enabled(enabled), graphPath(path) {}
    };
}
