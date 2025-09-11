/*!
@file       Collision.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Basic Collision Functions and Colliders(Plane, AABB and Circle)


//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include "transformcomponent.h"
#include <memory>
#include <iostream>
#include <cmath>

namespace Collision {

	enum ColliderType {
		PLANE,
		AABB
	};

	/*!
	@class	Collider
	@brief	Base class for inheriting other colliders
	*/
	class Collider {
		public:
			ColliderType type;

			Collider(ColliderType type) : type{ type } {}

			virtual ~Collider() = default;

	};

	/*!
	@class	Plane
	@brief	Basic plane collider with distance, normal and position
	*/
	class Plane : public Collider {

		public:
			float distance;
			CEVec3 normal;
			CEVec3 position;

			Plane(float distance, CEVec3 const& normal, CEVec3 const& position) : Collider{ PLANE }, distance{ distance }, normal{ normal }, position{ position } { /*EMPTY BY DESIGN*/ }
			~Plane() = default;
	};

	/*!
	@class	AABBRect
	@brief	Basic aabb rectangle with CEVec3 min and CEVec3 max
	*/
	class AABBRect : public Collider {
		public:
			CEVec3 min;
			CEVec3 max;

			AABBRect(CEVec3 const& min, CEVec3 const& max) : Collider{ColliderType::AABB}, min{ min }, max{ max } { /*EMPTY BY DESIGN*/ }
			~AABBRect() = default;
	};
}

namespace Collision {


	/*struct CollisionProperties {
		Component::TransformComponent transform;
		Collider* collider;
	};*/


	using FunctionDispatchType = bool (*)(std::unique_ptr<Collider> const& aabb1, const CEVec3& vel1, std::unique_ptr<Collider> const& aabb2, const CEVec3& vel2, float dt);

	class CollisionHandler {
	private:
		static bool collisionIntersectRectRectStatic(std::unique_ptr<Collider> const& aabb1, std::unique_ptr<Collider> const& aabb2) {
			AABBRect const& aabb1Temp = dynamic_cast<AABBRect const&>(*aabb1);
			AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*aabb2);


			if (aabb1Temp.max.x < aabb2Temp.min.x) {
				return false;
			} if (aabb1Temp.min.x > aabb2Temp.max.x) {
				return false;
			} if (aabb1Temp.max.y < aabb2Temp.min.y) {
				return false;
			} if (aabb1Temp.min.y > aabb2Temp.max.y) {
				return false;
			}

			return true;
		}

		/*COLLISION BETWEEN AABB*/
		static bool AABBCollision(std::unique_ptr<Collider> const& aabb1, const CEVec3& vel1, std::unique_ptr<Collider> const& aabb2, const CEVec3& vel2, float dt) {

			if (collisionIntersectRectRectStatic(aabb1, aabb2)) {
				return true;
			};

			AABBRect const& aabb1Temp{ dynamic_cast<AABBRect const&>(*aabb1) };
			AABBRect const& aabb2Temp{ dynamic_cast<AABBRect const&>(*aabb2) };

			//Step 2: Initialize and calculate the new "relative" velocity of Vb
			float tFirst{};
			float tLast{ static_cast<float>(dt) };
			CEVec3 vB1{ vel1.x - vel2.x, vel1.y - vel2.y, 0.f }; //relative velocity of 1 with 2


			//STEP 3 X-AXIS RELATIVE VELOCITY CHECK-------------------------------------------------------
			if (vB1.x < 0.0f) {

				if (aabb1Temp.max.x < aabb2Temp.min.x) { // CASE 1
					return false;
				}

				// CASE 4, MOVING OBJECT IS GOING FROM RIGHT TO LEFT

				if (aabb2Temp.max.x < aabb1Temp.min.x) { // CASE 4 - REVISITED
					tFirst = std::max((aabb2Temp.max.x - aabb1Temp.min.x) / vB1.x, tFirst);
				}

				if (aabb2Temp.min.x < aabb1Temp.max.x) { // CASE 4 - REVISITED
					tLast = std::min((aabb2Temp.min.x - aabb1Temp.max.x) / vB1.x, tLast);
				}

			} if (vB1.x > 0.0f) {

				if (aabb2Temp.min.x > aabb1Temp.max.x) {
					tFirst = std::max((aabb2Temp.min.x - aabb1Temp.max.x) / vB1.x, tFirst);
				}

				if (aabb2Temp.max.x > aabb1Temp.min.x) {
					tLast = std::min((aabb2Temp.max.x - aabb1Temp.min.x) / vB1.x, tLast);
				}

				if (aabb1Temp.min.x > aabb2Temp.max.x) { // CASE 3
					return false;
				}

			} if (vB1.x == 0.0f) {

				if (aabb2Temp.max.x < aabb1Temp.min.x) {
					return false;
				}
				else if (aabb2Temp.min.x > aabb1Temp.max.x) {
					return false;
				}

			} if (tFirst > tLast) {

				return false;

			}
			//--------------------------------------------------------------------------------------------


			//STEP 4 Y-AXIS RELATIVE VELOCITY CHECK-------------------------------------------------------
			if (vB1.y < 0.0f) {
				if (aabb1Temp.max.y < aabb2Temp.min.y) { // CASE 1
					return false;
				}

				// CASE 4, MOVING OBJECT IS GOING FROM RIGHT TO LEFT

				if (aabb2Temp.max.y < aabb1Temp.min.y) { // CASE 4 - REVISITED
					tFirst = std::max((aabb2Temp.max.y - aabb1Temp.min.y) / vB1.y, tFirst);
				}

				if (aabb2Temp.min.y < aabb1Temp.max.y) { // CASE 4 - REVISITED
					tLast = std::min((aabb2Temp.min.y - aabb1Temp.max.y) / vB1.y, tLast);
				}

			} if (vB1.y > 0.0f) {

				if (aabb2Temp.min.y > aabb1Temp.max.y) { // CASE 2 - REVISITED
					tFirst = std::max((aabb2Temp.min.y - aabb1Temp.max.y) / vB1.y, tFirst);
				}

				if (aabb2Temp.max.y > aabb1Temp.min.y) { // CASE 2 - REVISITED
					tLast = std::min((aabb2Temp.max.y - aabb1Temp.min.y) / vB1.y, tLast);
				}

				if (aabb1Temp.min.y > aabb2Temp.max.y) { // CASE 3
					return false;
				}

			} if (vB1.y == 0.0f) {

				if (aabb2Temp.max.y < aabb1Temp.min.y) {
					return false;
				}
				else if (aabb2Temp.min.y > aabb1Temp.max.y) {
					return false;
				}

			} if (tFirst > tLast) {

				return false;

			}

			float tOfCollision{ tFirst };
		}

		static FunctionDispatchType constexpr collideFuncts[2][2]{
			{AABBCollision, nullptr},
			{nullptr, nullptr}
		};

	public:

		//static void callCollFunct(CollisionProperties* coll1, CollisionProperties* coll2) {
		//	/*if (coll1->collider->type > coll2->collider->type) {
		//		std::swap(coll1, coll2);
		//	}

		//	int idx1{ static_cast<int>(coll1->collider->type) };
		//	int idx2{ static_cast<int>(coll2->collider->type) };

		//	collideFuncts[idx1][idx2](coll1, coll2);*/
		//}

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
