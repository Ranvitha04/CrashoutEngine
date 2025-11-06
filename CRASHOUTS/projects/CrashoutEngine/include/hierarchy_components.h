
#pragma once
#include <vector>
#include "entitymanager.h"
#include "CEVec3.h"   
#include "CEVec2.h"  

namespace Component {

    struct ParentComponent {
        CrashoutsEngine::Entity parent{ 0 };

    
        CEVec3 localPos{ 0.f, 0.f, 0.f };
        float  localRot{ 0.f };
        CEVec2 localScale{ 1.f, 1.f };
        
        bool   inheritColor{ false };
        CEVec3 colorMul{ 1.f, 1.f, 1.f };   
    };

    struct ChildrenComponent {
        std::vector<CrashoutsEngine::Entity> children;
    };

} 
