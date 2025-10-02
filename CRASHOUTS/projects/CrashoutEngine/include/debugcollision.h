/*!
@file       debugcollision.h
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025
@brief      Collision debug visualization utilities for Crashout Engine.
@details    This header provides functions for rendering collision-related
            debug overlays within the engine's ImGui-based debug tools.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "managers/entitymanager.h"
#include "managers/collisionmanager.h"
#include "components/transformcomponent.h"
#include "components/shapescomponent.h"
#include "shapes.h"

namespace CollisionGizmos {

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
    void DrawAllColliders(CrashoutsEngine::ECS& ecs, const std::vector<Collision::CollisionEvent>* events = nullptr, float wireThickness = 0.02f);

    // -- Contact Point & Normal Visualization ---------------------------------

    /*!
     @brief Draws collision contact points and their corresponding normals.
     @param events Reference to a list of collision events produced by the physics system.
     @param normalLen Length of the drawn normal vectors at contact points.
     @param wireThickness Thickness of the line used for rendering normals.
     @note Use this function to debug collision resolution behavior and validate
           contact point accuracy and normal direction calculations.
    */
    void DrawContactsAndNormals(const std::vector<Collision::CollisionEvent>& events, float normalLen = 0.25f, float wireThickness = 0.02f);

} // namespace CollisionGizmos
