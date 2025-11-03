// hierarchy_components.h
#pragma once
#include <vector>
#include "entitymanager.h"
#include "CEVec3.h"   // for CEVec3
#include "CEVec2.h"   // for CEVec2 (or whatever your 2D vector is called)

namespace Component {

    struct ParentComponent {
        CrashoutsEngine::Entity parent{ 0 };

        // Local (relative-to-parent) transform captured at Attach time:
        CEVec3 localPos{ 0.f, 0.f, 0.f };
        float  localRot{ 0.f };
        CEVec2 localScale{ 1.f, 1.f };
    };

    struct ChildrenComponent {
        std::vector<CrashoutsEngine::Entity> children;
    };

} // namespace Component
