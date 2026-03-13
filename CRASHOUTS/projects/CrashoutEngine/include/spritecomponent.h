#pragma once
#include "CEVec2.h"      
#include "CEVec3.h"
#include <string>       
#include "entitymanager.h" 

/*!
@file   spriteAnimation.h
@author Ranvitha
@author Afiq
@author Renfred Lee
@date   30/11/2025
@brief  This file contains the declaration of sprite-related components

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/


namespace Component {
    struct DuckMotionComponent {
        /*!
        @brief Current normalized movement direction (z unused for 2D).
        @details Updated with mild noise and steering; multiplied by `speed` for velocity integration.
        */
        CEVec3 velocity{ 0.0f, 0.0f, 0.0f };

        /*!
        @brief Movement speed in world units per second.
        */
        float  speed{ 0.0f };

        /*!
        @brief Turn rate in degrees per second used by steering noise.
        */
        float  turnRate{ 1.0f };

        /*!
        @brief Oscillation amount for steering; higher values cause more pronounced weaving.
        */
        float  oscillation{ 8.0f };

        /*!
        @brief Time accumulator for noise-based steering.
        */
        float  noiseTime{ 0.0f };

        /*!
        @brief Minimum bounds (world space) for clamping.
        */
        CEVec3 boundsMin{ -100.0f, -60.0f, -1.0f };

        /*!
        @brief Maximum bounds (world space) for clamping.
        */
        CEVec3 boundsMax{  100.0f,  60.0f,  1.0f };

        /*!
        @brief Manual animation mode (0 = auto, 1 = idle_normal, 2 = walk_normal, 3 = idle_bounce, 4 = walk_bounce).
        @details When non-zero, the updater uses this mode instead of auto-selecting based on speed.
        */
        int   animationMode{ 1 };

        /*!
        @brief Set explicit world-space bounds for motion clamping.
        @param minB Minimum bound
        @param maxB Maximum bound
        */
        void SetBounds(const CEVec3& minB, const CEVec3& maxB) {
            boundsMin = minB;
            boundsMax = maxB;
        }

        /*!
        @brief Set bounds using a centered rectangle from world width/height.
        @param worldWidth Total width of the world
        @param worldHeight Total height of the world
        @param zMin Optional min z bound (default -1)
        @param zMax Optional max z bound (default 1)
        */
        void SetBoundsFromWorld(float worldWidth, float worldHeight, float zMin = -1.0f, float zMax = 1.0f) {
            float halfW = worldWidth * 0.5f;
            float halfH = worldHeight * 0.5f;
            boundsMin = CEVec3(-halfW, -halfH, zMin);
            boundsMax = CEVec3( halfW,  halfH, zMax);
        }

        /*!
        @brief Expand or contract current bounds by a uniform margin.
        @param margin Positive expands; negative contracts
        */
        void ExpandBounds(float margin) {
            boundsMin.x -= margin;
            boundsMin.y -= margin;
            boundsMax.x += margin;
            boundsMax.y += margin;
        }

        /*!
        @brief Clamp a position to current bounds.
        @param pos Position to clamp; modified in-place
        @return True if any component was clamped
        */
        bool ClampToBounds(CEVec3& pos) const {
            bool clamped = false;
            if (pos.x < boundsMin.x) { pos.x = boundsMin.x; clamped = true; }
            if (pos.x > boundsMax.x) { pos.x = boundsMax.x; clamped = true; }
            if (pos.y < boundsMin.y) { pos.y = boundsMin.y; clamped = true; }
            if (pos.y > boundsMax.y) { pos.y = boundsMax.y; clamped = true; }
            if (pos.z < boundsMin.z) { pos.z = boundsMin.z; clamped = true; }
            if (pos.z > boundsMax.z) { pos.z = boundsMax.z; clamped = true; }
            return clamped;
        }
    };

    // Per-entity sprite sheet configuration
    struct SpriteSheetComponent {
        // Asset identifier or path; when using a cached handle, you may leave this as empty.
        std::string textureAsset;
        int cols{ 1 };
        int rows{ 1 };
        CEVec2 worldSize{ 1.0f, 1.0f };
    };

    // Per-entity animation runtime state
    struct SpriteAnimComponent {
        int   startFrame{ 0 };
        int   frameCount{ 1 };
        float frameTime{ 0.1f };
        float timer{ 0.0f };
        int   frameIndex{ 0 };
        bool  flip{ false };
    };

    void SetSpriteSheet(CrashoutsEngine::Entity e, const std::string& assetName, int cols, int rows, CEVec2 worldSize);
    void UpdateSpriteAnimationsECS();
    void RenderSpriteAnimationsECS();

}
