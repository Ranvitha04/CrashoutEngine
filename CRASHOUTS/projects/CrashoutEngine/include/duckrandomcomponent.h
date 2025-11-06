#pragma once
#include "CEVec3.h"

namespace Component {
    struct DuckMotionComponent {
        CEVec3 velocity{ 0.0f, 0.0f, 0.0f };
        float  speed{ 0.0f };

        float  turnRate{ 1.0f };
        float  oscillation{ 8.0f };
        float  noiseTime{ 0.0f };

        
        CEVec3 boundsMin{ -100.0f, -60.0f, -1.0f };
        CEVec3 boundsMax{  100.0f,  60.0f,  1.0f };

        int   animationMode{ 1 };

        // Set explicit bounds
        void SetBounds(const CEVec3& minB, const CEVec3& maxB) {
            boundsMin = minB;
            boundsMax = maxB;
        }

        // Set bounds using a centered world rectangle (worldWidth x worldHeight).
        // zMin/zMax are optional and default to -1..1.
        void SetBoundsFromWorld(float worldWidth, float worldHeight, float zMin = -1.0f, float zMax = 1.0f) {
            float halfW = worldWidth * 0.5f;
            float halfH = worldHeight * 0.5f;
            boundsMin = CEVec3(-halfW, -halfH, zMin);
            boundsMax = CEVec3( halfW,  halfH, zMax);
        }

        // Expand current bounds by a uniform margin (positive expands, negative contracts)
        void ExpandBounds(float margin) {
            boundsMin.x -= margin;
            boundsMin.y -= margin;
            boundsMax.x += margin;
            boundsMax.y += margin;
        }

        // Clamp a position into the bounds and return whether clamping occurred
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
}