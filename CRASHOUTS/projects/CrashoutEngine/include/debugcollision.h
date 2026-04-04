/*!
@file   debugcollision.h
@author Shahrin Idzuan (50%), Renfred Lee (50%)
@date   28/09/2025

@brief  Collision debug visualization utilities for Crashout Engine.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "entitymanager.h"
#include "collisionmanager.h"

namespace CollisionGizmos {

    // Container for line valuess
    struct DebugLineCmd {
        CEVec3 p1;
        CEVec3 p2;
        float thickness;
        CEColor color;
    };

    // Collection of debuglinecmd
    extern std::vector<DebugLineCmd> g_DebugLineQueue;

    // -- Collider Visualization -----------------------------------------------

    /*!
     @brief Draws wireframe overlays for all colliders in the ECS.
     @param ecs Reference to the ECS instance managing entities and components.
     @param events Optional pointer to a list of collision events. If provided,
                   colliding colliders will be tinted for visual distinction.
     @param wireThickness Thickness of the wireframe lines for collider outlines.
     @note This function is useful for visualizing the spatial configuration of
           all active colliders, and highlighting those currently involved in
           collisions when collision event data is available.
    */
    void DrawAllColliders(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>& events, float wireThickness = 0.3f);

    // -- Contact Point & Normal Visualization ---------------------------------

    /*!
     @brief Draws collision contact points and their corresponding normals.
     @param events Reference to a list of collision events produced by the physics system.
     @param normalLen Length of the drawn normal vectors at contact points.
     @param wireThickness Thickness of the line used for rendering normals.
     @note Use this function to debug collision resolution behavior and validate
           contact point accuracy and normal direction calculations.
    */
    void DrawContactsAndNormals(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>& events, float baseLen, float wireThickness);

} // namespace CollisionGizmos
