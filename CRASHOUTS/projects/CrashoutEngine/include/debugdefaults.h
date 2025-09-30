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
#include "math/CEVec3.h"

struct DebugDefaults {
    // Name
    const char* name_default = "Unnamed Entity";

    // RigidBody
    CEVec3 rb_init_velocity{ 0.f, 0.f, 0.f };
    float  rb_mass = 5.f;
    float  rb_restitution = 0.f;

    // Colliders
    float  sphere_radius = 1.f;
    float  aabb_half = 0.5f; // half extent around transform
    bool   gFollowColliderToTransform = true;

    // MeshComponent UI
    // If you want this data-driven, store the display names here:
    const char* shape_types[4] = { "Square","Rectangle","Line","Triangle" };
    int         shape_type_count = 4;

    // UI drag speeds / ranges (so designers can tune UX without code)
    float drag_small = 0.05f;
    float drag_medium = 0.1f;
    float drag_rotation = 1.0f;
    float drag_big = 0.5f;
    float min_zero = 0.f;
    float max_large = 1e6f;
};

// Singleton-style accessor (replace with your DI as needed)
DebugDefaults& GetDebugDefaults();