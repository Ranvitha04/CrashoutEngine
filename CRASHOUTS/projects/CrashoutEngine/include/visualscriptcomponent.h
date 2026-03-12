#pragma once
#include <string>
#include "debugmenu.h"
#include <memory>
#include <unordered_map>
namespace Component {
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

        VisualScriptComponent() = default;
        VisualScriptComponent(bool enabled, const std::string& path = "")
            : enabled(enabled), graphPath(path) {}
    };
}