#pragma once

#include "colliders.h"
#include <memory>

/*!
@file       collidercomponent.h
@author     miratristan.d@digipen.edu
@date       29/11/2025
@brief      Collider component to hold custom colliders for entities

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

namespace Component {
	/*!
	@struct	ColliderComponent
	@brief	Collider component in order to put custom colliders on an entity
	*/
	struct ColliderComponent {
		std::unique_ptr<Collision::Collider> collider;

		// Default constructor for empty component
		ColliderComponent() = default;

		// Custom Copy Constructor to build from unique_ptr
		// Explicitly handles calls like AddComponent(e, std::make_unique<...>)
		explicit ColliderComponent(std::unique_ptr<Collision::Collider> col)
			: collider(std::move(col)) {}

		// Custom Copy Constructor
		ColliderComponent(const ColliderComponent& other) {
			if (other.collider) {
				// Use the virtual clone method for a deep copy
				collider = other.collider->clone();
			}
		}

		// Custom Copy Assignment Operator for Deep Copies
		ColliderComponent& operator=(const ColliderComponent& other) {
			// Check for self-assignment
			if (this == &other) return *this;

			if (other.collider) {
				collider = other.collider->clone();	// Deep Copy
			}
			else {
				collider.reset();	// Clear if the other is empty
			}
			return *this;
		}

		ColliderComponent(ColliderComponent&&) noexcept = default;
		ColliderComponent& operator=(ColliderComponent&&) noexcept = default;
	};
}