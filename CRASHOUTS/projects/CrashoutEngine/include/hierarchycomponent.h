/*!
@file       hierarchycomponent.h
@author     Renfred Lee
@author     Shahrin Idzuan
@date       30/12/2025
@brief      Component definitions for hierarchy system.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/

#pragma once
#include <vector>
#include "entitymanager.h" 
#include "CEVec2.h" 
#include "CEVec3.h" 
#include "CEColor.h" 

/*!
All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

namespace Component {

    struct ParentComponent {
        CrashoutsEngine::Entity parent{ 0 };

        CEVec3 localPos{ 0.f, 0.f, 0.f };
        float  localRot{ 0.f };
        CEVec2 localScale{ 1.f, 1.f };
        
        bool   inheritColor{ false };
        CEColor colorMul{ 1.f, 1.f, 1.f, 1.f };

    };

    struct ChildrenComponent {
        std::vector<CrashoutsEngine::Entity> children;
    };

} 

