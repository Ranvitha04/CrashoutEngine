/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, AABB and Circle)


//__________________________________________________________________________*/

#include "managers/collisionmanager.h"
#include "math/CEMath.h"
#include <cmath>
#include <algorithm>

namespace Collision {
	Collider::Collider(ColliderType type) : type{ type } { /*EMPTY BY DESIGN*/ }

	Sphere::Sphere(f32 radius, CEVec3 const& center) : Collider{ SPHERE }, radius{ radius }, center{ center } { /*EMPTY BY DESIGN*/ }

	AABBRect::AABBRect(CEVec3 const& min, CEVec3 const& max) : Collider{ AABB }, min{ min }, max{ max } { /*EMPTY BY DESIGN*/ }
}

namespace Collision {
	CollisionManifold CollisionHandler::SphereToSphereCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionManifold result;

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(coll1.collider));

		Sphere const& sphere2Temp = dynamic_cast<Sphere const&>(*(coll2.collider));

		float totalRadius{ sphere1Temp.radius + sphere2Temp.radius };
		CEVec3 circ1ToCirc2{ coll2.position - coll1.position };
		if (CEMath::LengthSq(circ1ToCirc2) - (totalRadius * totalRadius) > 0.f) {
			result.collisionOccured = false;
			return result;
		}

		/*HALF OF ACTUAL DEPTH TO ACCOUNT FOR THE TWO BODIES*/
		float depth{ std::abs(CEMath::Length(circ1ToCirc2) - totalRadius) * 0.5f };

		CEVec3 normal{ CEMath::Normalize(circ1ToCirc2) };
		float distanceToPoint{ sphere1Temp.radius - depth };
		CEVec3 contactPoint{ (normal * distanceToPoint) + coll1.position };

		result.depth = depth;
		result.normal = normal;
		result.contactPoints[0] = contactPoint;
		result.collisionOccured = true;

		return result;
	}

	CollisionManifold CollisionHandler::AABBToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		
		CollisionManifold result;
		
		AABBRect const& aabb1Temp = dynamic_cast<AABBRect const&>(*(coll1.collider));

		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(coll2.collider));

		CEVec3 posDelta{ coll2.position - coll1.position }; /*Vector showing distance + the direction*/

		f32 xDiff{ aabb1Temp.max.x + aabb2Temp.max.x - std::abs(posDelta.x)};

		if (xDiff > 0.f) {
			f32 yDiff{ aabb1Temp.max.y + aabb2Temp.max.y - std::abs(posDelta.y) };

			if (yDiff > 0.f) {

				if (xDiff < yDiff) {
					result.collisionOccured = true;
					result.normal = posDelta.x < 0 ? CEVec3{ -1.f, 0.f, 0.f } : CEVec3{ 1.f, 0.f, 0.f };
					result.depth = xDiff;

				} else {
					result.collisionOccured = true;
					result.normal = posDelta.y < 0 ? CEVec3{ 0.f, -1.f, 0.f } : CEVec3{ 0.f, 1.f, 0.f };
					result.depth = yDiff;
				}

			}
		}		
		return result;
	}

	CollisionManifold CollisionHandler::SphereToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{};

		CollisionProps const* coll1Ptr{ &coll1 };
		CollisionProps const* coll2Ptr{ &coll2 };
		if (coll1Ptr->collider->type > coll2Ptr->collider->type) {
			std::swap(coll1Ptr, coll2Ptr);
		}

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(coll1Ptr->collider));

		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(coll2Ptr->collider));

		CEVec3 minAABB{ CEVec3 {coll2Ptr->position.x - aabb2Temp.max.x, coll2Ptr->position.y - aabb2Temp.max.y, 0.f} };
		CEVec3 maxAABB{ CEVec3 {coll2Ptr->position.x + aabb2Temp.max.x, coll2Ptr->position.y + aabb2Temp.max.y, 0.f} };

		CEVec3 bounds{coll1Ptr->position};
		if (coll1Ptr->position.x < minAABB.x) bounds.x = minAABB.x;
		if (coll1Ptr->position.x > maxAABB.x) bounds.x = maxAABB.x;

		if (coll1Ptr->position.y < minAABB.y) bounds.y = minAABB.y;
		if (coll1Ptr->position.y > maxAABB.y) bounds.y = maxAABB.y;

		CEVec3 delta{ coll1Ptr->position - bounds };
		f32 squaredDistance{ CEMath::LengthSq(delta) };

		if (squaredDistance < sphere1Temp.radius * sphere1Temp.radius) {

			if (squaredDistance == 0.f) {
				CEVec3 posDelta{ coll1Ptr->position - coll2Ptr->position };

				float xd{ sphere1Temp.radius + aabb2Temp.max.x - std::abs(posDelta.x) };
				float yd{ sphere1Temp.radius + aabb2Temp.max.y - std::abs(posDelta.y) };

				CEVec3 normal{};
				f32 depth{};

				if (xd < yd) {
					normal = posDelta.x < 0 ? CEVec3{ -1.f, 0.f, 0.f } : CEVec3{ 1.f, 0.f, 0.f };
					depth = xd;
				} else {
					normal = posDelta.y < 0 ? CEVec3{ 0.f, -1.f, 0.f } : CEVec3{ 0.f, 1.f, 0.f };
					depth = yd;
				}
				result.depth = depth;
				result.normal = normal;
				result.collisionOccured = true;

			} else {

				f32 depth{CEMath::Length(delta)};
				delta = CEMath::Normalize(delta);
				result.normal = -delta;
				result.depth = -(depth - sphere1Temp.radius);
				result.collisionOccured = true;

			}
		}

		return result;
	}

	CollisionHandler::CollisionHandler() : collideFuncts {
		std::array<Collision::FunctionDispatchType, TYPES_OF_COLLISION * TYPES_OF_COLLISION> {
			nullptr, nullptr,
			nullptr, nullptr
		}
	} {
		int sphereIdx{ static_cast<int>(SPHERE) };
		int AABBIdx{ static_cast<int>(AABB) };

		/*Fill in the table of collisions, e.g collideFuncts[0] will be SPHERE TO SPHERE collision (2 total collider types will have 2^2 permutations(different types of collisions possible) ) */
		collideFuncts[sphereIdx] = SphereToSphereCollision; 
		collideFuncts[AABBIdx * TYPES_OF_COLLISION + AABBIdx] = AABBToAABBCollision;
		collideFuncts[sphereIdx * TYPES_OF_COLLISION + AABBIdx] = SphereToAABBCollision;
	}
	FunctionDispatchType CollisionHandler::callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2) {

		/*Swap the type according to enum order, e.g function exists for sphere to aabb but not aabb to sphere*/
		if (type1 > type2) {
			std::swap(type1, type2);
		}

		int idx1{ static_cast<int>(type1) };
		int idx2{ static_cast<int>(type2) };

		return collideFuncts[idx1 * TYPES_OF_COLLISION + idx2];
	}
}