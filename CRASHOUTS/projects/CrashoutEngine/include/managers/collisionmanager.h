/*!
@file       Collision.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Basic Collision Functions and Colliders(Plane, AABB and Circle)


//__________________________________________________________________________*/

#pragma once

#include "math/CEVec3.h"
#include "components/transformcomponent.h"
#include "messaging/observable.h"
#include "log.h"
#include "managers/entitymanager.h"
#include <memory>
#include <iostream>
#include <cmath>
#include <array>
#include <algorithm>
#include <utility>


namespace Collision {

	/*!
	@enum	ColliderType
	@brief	Different types of colliders
	*/
	enum ColliderType {
		SPHERE,
		AABB
	};

	/*!
	@struct	CollisionManifold
	@brief	Contains information regarding Collision events between Entities
	*/
	struct CollisionManifold {
		CEVec3 normal;
		std::array<CEVec3, 2> contactPoints; /*Max contact point of 2 for AABB, 1 for Circle*/
		f32 depth;
		bool collisionOccured;
	};

	/*!
	@struct	CollisionEvent
	@brief	Contains which pairs of entity has collided along with their Collision manifold
	*/
	struct CollisionEvent {
		CollisionManifold colManifold;
		CrashoutsEngine::Entity entity1;
		CrashoutsEngine::Entity entity2;
	};

	/*!
	@class	Collider
	@brief	Base class for inheriting other colliders
	*/
	class Collider {
		public:
			ColliderType type;
			Collider(ColliderType type);
			virtual ~Collider() = default;
	};

	/*!
	@class	Sphere
	@brief	Basic Sphere collider with radius and position
	*/
	class Sphere : public Collider {
		public:
			f32 radius;
			CEVec3 center;
			Sphere(f32 radius, CEVec3 const& center);
			~Sphere() = default;
	};

	/*!
	@class	AABBRect
	@brief	Basic aabb rectangle with CEVec3 min and CEVec3 max
	*/
	class AABBRect : public Collider {
		public:
			CEVec3 min;
			CEVec3 max;
			AABBRect(CEVec3 const& min, CEVec3 const& max);
			~AABBRect() = default;
	};
}

namespace Collision {

	class CollisionMessage : public Messaging::Messages {
		private:
			CollisionManifold colManifold;
			CrashoutsEngine::Entity entity1;
			CrashoutsEngine::Entity entity2;
		public:
			CollisionMessage(const std::string& id, CollisionManifold const &colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2);
			std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> getCollisionPair();
			CollisionManifold getCollisionManifold();
	};

	/*!
	@struct	CollisionProps
	@brief	Used to pass data about entities(collider, velocity and position)
	*/
	struct CollisionProps {
		std::unique_ptr<Collider> const& collider;
		const CEVec3& vel;
		const CEVec3& position;
	};

	using FunctionDispatchType = CollisionManifold(*)(CollisionProps const &coll1, CollisionProps const &coll2);

	/*!
	@class	CollisionHandler
	@brief	Used to contain functions for checking collision and able to return a function pointer based on the type of collision passed in
	*/
	class CollisionHandler : public Messaging::Observable {
		private:

			static constexpr int TYPES_OF_COLLISION{ 2 };

			std::array<FunctionDispatchType, TYPES_OF_COLLISION * TYPES_OF_COLLISION> collideFuncts;

			static CollisionManifold SphereToSphereCollision(CollisionProps const& coll1, CollisionProps const& coll2);

			static CollisionManifold AABBToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2);
			
			static CollisionManifold SphereToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

		public:

			CollisionHandler();

			void update(double deltaTime);

			/*!
			@brief Return a function pointer to call the appropriate collision function
			@param type1 The first collider type
			@param type2 The second collider type
			@return FunctionDispatchType which is a function pointer
			*/
			FunctionDispatchType callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2);

	};
}

namespace Component {

	/*!
	@struct	ColliderComponent
	@brief	Collider component in order to put custom colliders on an entity
	*/
	struct ColliderComponent {
		std::unique_ptr<Collision::Collider> collider;
	};
}

