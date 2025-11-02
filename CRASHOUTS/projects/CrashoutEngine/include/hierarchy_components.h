// hierarchy_components.h
#pragma once
#include <vector>
#include "entitymanager.h" // for CrashoutsEngine::Entity

namespace Component {

    struct ParentComponent {
        CrashoutsEngine::Entity parent{ 0 };
    };

    struct ChildrenComponent {
        std::vector<CrashoutsEngine::Entity> children;
    };

} // namespace Component
