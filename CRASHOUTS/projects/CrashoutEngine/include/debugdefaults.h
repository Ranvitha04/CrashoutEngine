/*!
@file       debugdefaults.h
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

Declare struct defaults used across Debug Menu adapters
(e.g., drag speeds, initial rigidbody values, default collider sizes).
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#pragma once
#include "CEVec3.h"

//__________________________________________________________________________*/

/*!
 @struct DebugDefaults
 @brief Stores default settings and configuration values used throughout the debug UI.
 @details
 This struct consolidates initial values and common constants used by debug
 adapters, including physics defaults, collider sizes, UI drag speeds, and
 default display names. Modifying these values can tune the user experience
 and adjust the default behavior of the debug interface.
*/
struct DebugDefaults {
    // -- Entity Metadata ------------------------------------------------------

    /*! @brief Default name assigned to new entities in debug tools. */
    const char* name_default = "Unnamed Entity";

    // -- Rigidbody Defaults ---------------------------------------------------

    /*! @brief Initial velocity applied to new rigidbody components. */
    CEVec3 rb_init_velocity{ 0.f, 0.f, 0.f };

    /*! @brief Default mass for rigidbodies. */
    float rb_mass = 5.f;

    /*! @brief Default restitution (bounciness) for rigidbodies. */
    float rb_restitution = 0.f;

    // -- Collider Defaults ----------------------------------------------------

    /*! @brief Default radius for sphere colliders. */
    float sphere_radius = 1.f;

    /*! @brief Default half-extent for AABB colliders (centered on transform). */
    float aabb_half = 0.5f;

    /*! @brief Whether colliders should follow their transform automatically. */
    bool gFollowColliderToTransform = true;

    // -- Mesh Component Defaults ----------------------------------------------

    /*!
     @brief Available shape types for mesh components.
     @details
     These strings are used for display in debug UI panels.
    */
    const char* shape_types[4] = { "Square", "Rectangle", "Line", "Triangle" };

    /*! @brief Number of shape types available. */
    int shape_type_count = 4;

    // -- UI Interaction Settings ----------------------------------------------

    /*! @brief Drag speed for fine adjustments. */
    float drag_small = 0.05f;

    /*! @brief Drag speed for medium-range adjustments. */
    float drag_medium = 0.1f;

    /*! @brief Drag speed for rotation fields. */
    float drag_rotation = 1.0f;

    /*! @brief Drag speed for large value changes. */
    float drag_big = 0.5f;

    /*! @brief Minimum clamped value (commonly used as zero bound). */
    float min_zero = 0.f;

    /*! @brief Maximum clamped value for large ranges. */
    float max_large = 1e6f;
};

// -----------------------------------------------------------------------------
// Global Accessor
// -----------------------------------------------------------------------------

/*!
 @brief Provides global access to the `DebugDefaults` singleton instance.
 @return Reference to the global `DebugDefaults` instance.
 @note This allows adapters and panels to retrieve default settings consistently.
*/
DebugDefaults& GetDebugDefaults();