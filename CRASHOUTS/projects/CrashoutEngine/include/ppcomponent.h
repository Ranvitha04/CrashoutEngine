/*!
@file   ppcomponent.h
@author Ranvitha
@author Shahrin Idzuan
@author Renfred (RTIS)
@date   05/11/2025

@brief  This file defines the post-processing component.

All content ? 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "CEVec2.h"
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
        float shadowDarkness{ 1.0f };
        CEVec3 shadowColor{ 0.0f, 0.0f, 0.0f };
        float shadowColorStrength{ 1.0f };
        float shadowMapStrength{ 1.0f };
        float raymarchShadowStrength{ 1.0f };

        bool heatDistortionEnabled{ false };
        bool heatDebugMarker{ false };
        float heatStrength{ 0.003f };
        float heatSpeed{ 2.0f };
        float heatScale{ 20.0f };
        CEVec3 heatTintColor{ 1.0f, 0.5f, 0.1f };
        float heatTintStrength{ 0.0f };

        bool distortionEnabled{ false };
        bool distortionDebugMarker{ false };
        float distortionStrength{ 0.003f };
        float distortionSpeed{ 2.0f };
        float distortionWaveScale1{ 20.0f };
        float distortionWaveScale2{ 10.0f };
        float distortionNoiseStrength{ 0.8f };
        float distortionLavaHeight{ 0.3f };
        float distortionLavaBoost{ 2.5f };
        CEVec2 distortionPlayerPos{ 0.5f, 0.5f };
        float distortionPlayerRadius{ 0.1f };
        float distortionPlayerStrength{ 0.01f };
        CEVec3 distortionGlowColor{ 1.0f, 0.4f, 0.1f };
        float distortionGlowStrength{ 1.0f };
        bool distortionShockEnabled{ false };
        CEVec2 distortionShockPos{ 0.5f, 0.0f };
        float distortionShockTime{ 999.0f };
        float distortionShockStrength{ 0.02f };
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
        bool ignoreParentOccluders{ true };
        bool ignoreOccluders{ true };
        int shadowTechnique{ 0 }; // 0 = Shadow Map (per-light), 1 = Raymarch (screen mask)
        int raymarchSteps{ 24 };
        float raymarchThreshold{ 0.5f };
        bool flickerEnabled{ false };
        float flickerStrength{ 0.5f };
        float flickerSpeed{ 6.0f };
    };
}
