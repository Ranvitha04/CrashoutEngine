/*!
@file       Collision.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Basic Collision Functions and Colliders(Plane, AABB and Circle)


//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include "observable.h"
#include "log.h"
#include "colliders.h"
#include "entitymanager.h"
#include "quadtree.h"
#include <memory>
#include <array>
#include <utility>


namespace Collision {

	/*!
	@class	CollisionMessage
	@brief	Used as the data to collision messages
	*/
	class CollisionMessage : public Messaging::Messages {
	private:
		CollisionManifold colManifold;
		CrashoutsEngine::Entity entity1;
		CrashoutsEngine::Entity entity2;
	public:
		CollisionMessage(const std::string& id, CollisionManifold const& colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2);
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

	using FunctionDispatchType = CollisionManifold(*)(CollisionProps const& coll1, CollisionProps const& coll2);

	/*!
	@class	CollisionHandler
	@brief	Used to contain functions for checking collision and able to return a function pointer based on the type of collision passed in
	*/
	class CollisionHandler : public Messaging::Observable {
	private:


		static constexpr int TYPES_OF_COLLISION{ 3 };

		std::array<FunctionDispatchType, TYPES_OF_COLLISION* TYPES_OF_COLLISION> collideFuncts;

		static CollisionManifold SphereToSphereCollision(CollisionProps const& coll1, CollisionProps const& coll2);

		static CollisionManifold AABBToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

		static CollisionManifold SphereToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

		static CollisionManifold AABBToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

		static CollisionManifold SphereToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

		static CollisionManifold OBBToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2);

	public:

		static DataStructure::QuadTree quadTree;

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

