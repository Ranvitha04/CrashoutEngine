/*!
@file   hierarchycomponent.h
@author Shahrin Idzuan (94%), Renfred Lee (6%)
@date   30/12/2025

@brief  Component definitions for hierarchy system.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once
namespace Component {

    /** @brief ParentComponent */
    struct ParentComponent {
        CrashoutsEngine::Entity parent{ 0 };

        CEVec3 localPos{ 0.f, 0.f, 0.f };
        float  localRot{ 0.f };
        CEVec2 localScale{ 1.f, 1.f };
        
        bool   inheritColor{ false };
        CEColor colorMul{ 1.f, 1.f, 1.f, 1.f };

    };

    /** @brief ChildrenComponent */
    struct ChildrenComponent {
        std::vector<CrashoutsEngine::Entity> children;
    };

} 

