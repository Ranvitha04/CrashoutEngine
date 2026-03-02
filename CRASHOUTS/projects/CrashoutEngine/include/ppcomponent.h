/*!
@file   ppcomponent.h
@author lee.r@digipen.edu
@date   05/11/2025

@brief  This file defines the post-processing component.

All content ? 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "CEVec3.h"

/*                                                                   
----------------------------------------------------------------------------- */
namespace Component {

    /*!
    @struct  PPComponent
    @brief   A component that holds the holds modifiable data for post-processing.
    */
    struct PPComponent {
        // Active bool
        bool active{ true };
        // Brightness
        float brightness{ 0.f };
        // Contrast
        float contrast{ 0.f };
        // Saturation
        float saturation{ 0.f };
        // Ambient light multiplier for scene
        float ambient{ 0.15f };
        // Bloom settings
        bool bloomEnabled{ true };
        float bloomThreshold{ 1.0f };
        float bloomIntensity{ 0.8f };
        int bloomIterations{ 6 };
        // Raycast shadow toggle
        bool shadowsEnabled{ true };
        // Raycast shadow quality
        int shadowRays{ 128 };
    };

    /*!
    @struct  LightComponent
    @brief   Stores light properties for 2D lighting pass.
    */
    struct LightComponent {
        CEVec3 position{ 0.0f, 0.0f, 0.0f };
        CEVec3 color{ 1.0f, 1.0f, 1.0f };
        float intensity{ 1.0f };
        float radius{ 200.0f }; // In world units
        bool affectBloom{ true };
        bool useTransform{ true };
        bool renderSelf{ true };
        bool renderBehind{ false };
        bool active{ true };
    };
}
