#pragma once
#include <string>
#include "debugmenu.h"
#include <memory>

namespace Component {
    struct VisualScriptComponent {
        bool enabled{ true };
        std::string graphPath{ "" };

        std::shared_ptr<UI::VisualScriptingGraph> graph{ nullptr };
        bool graphLoaded{ false };
        int currentNodeId{ 0 };
        float waitTimer{ 0.0f };

        VisualScriptComponent() = default;
        VisualScriptComponent(bool enabled, const std::string& path = "")
            : enabled(enabled), graphPath(path) {}
    };
}