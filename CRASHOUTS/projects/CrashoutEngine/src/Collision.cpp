/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, AABB and Circle)


//__________________________________________________________________________*/

#include "managers/collisionmanager.h"
#include "components/rigidbodycomponent.h"
#include "math/CEMath.h"
#include <cmath>
#include <algorithm>

namespace Collision {
	Collider::Collider(ColliderType type) : type{ type } { /*EMPTY BY DESIGN*/ }

	Sphere::Sphere(f32 radius, CEVec3 const& center) : Collider{ SPHERE }, radius{ radius }, center{ center } { /*EMPTY BY DESIGN*/ }

	AABBRect::AABBRect(CEVec3 const& min, CEVec3 const& max) : Collider{ AABB }, min{ min }, max{ max } { /*EMPTY BY DESIGN*/ }
}

namespace Collision {

	/*--------------------------------------COLLISION MESSAGE DEFINITION---------------------------------------*/

	CollisionMessage::CollisionMessage(const std::string& id, CollisionManifold const& colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2) 
	: Messages::Messages{ id }, colManifold{colManifold}, entity1{entity1}, entity2{entity2}
	{ /*EMPTY BY DESIGN*/ }

	std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> CollisionMessage::getCollisionPair() {
		return std::pair{ entity1, entity2 };
	}

	CollisionManifold CollisionMessage::getCollisionManifold() {
		return colManifold;
	}
	/*----------------------------------------------------------------------------------------------------------*/


	/*----------------COLLISIONHANDLER CLASS DEFINITIONS--------------------------------------------------------*/

	CollisionHandler::CollisionHandler() : Messaging::Observable{"Collision"}, collideFuncts{
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

	void CollisionHandler::update(double deltaTime) {
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		std::vector<std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity>> currentCollisions{};

		std::vector<CrashoutsEngine::Entity> globalEntities = globalECS.GetComponentArray<Component::NameComponent>().indexToEntity;



		/*DETECT COLLISION AND UPDATE COLLISION EVENTS MEMBER*/
		for (CrashoutsEngine::Entity& entity1 : globalEntities) {

			if (!globalECS.HasComponent<Component::ColliderComponent>(entity1) || !globalECS.HasComponent<Component::TransformComponent>(entity1) || !globalECS.HasComponent<Component::RigidBodyComponent>(entity1)) continue;
			for (CrashoutsEngine::Entity const& entity2 : globalEntities) {
				if (entity1 == entity2) continue;
				if (!globalECS.HasComponent<Component::ColliderComponent>(entity2) || !globalECS.HasComponent<Component::TransformComponent>(entity2) || !globalECS.HasComponent<Component::RigidBodyComponent>(entity2)) continue;

				Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity1) };
				Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(entity1) };
				Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(entity1) };

				Component::ColliderComponent* collider2{ globalECS.GetComponent<Component::ColliderComponent>(entity2) };
				Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(entity2) };
				Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(entity2) };

				/*CHECK FOR COLLISION and call collision function*/
				Collision::CollisionManifold collisionManifold{};
				int tolerance{ 2 };

				Component::TransformComponent newTransform1{ *transform1 };
				Component::RigidBodyComponent newRigidBody1{ *rigidBody1 };

				newRigidBody1.setVelocity(newRigidBody1.getVelocity() / static_cast<float>(tolerance));

				for (int i{}; i < tolerance; ++i) {
					collisionManifold = callCollFunct(collider1->collider->type, collider2->collider->type)(
						Collision::CollisionProps{
							collider1->collider,
							CEVec3{},
							CEVec3{newTransform1.position.x, newTransform1.position.y, newTransform1.position.z}
						},
						Collision::CollisionProps{
							collider2->collider,
							CEVec3{},
							CEVec3{transform2->position.x, transform2->position.y, transform2->position.z}
						});

				}

				if (collisionManifold.collisionOccured) {
					CollisionMessage colMessage{ Messaging::getCollisionMsgID(), collisionManifold, entity1, entity2 };
					
					CrashoutsEngine::Entity const* entity1Ptr{ &entity1 };
					CrashoutsEngine::Entity const* entity2Ptr{ &entity2 };

					currentCollisions.push_back(std::pair{ *entity1Ptr, *entity2Ptr });
					sendMessage(colMessage);
					
				}

			}
		}

	}

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

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* aabbPtr{ &coll2 };
		if (spherePtr->collider->type > aabbPtr->collider->type) {
			std::swap(spherePtr, aabbPtr);
		}

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(spherePtr->collider));

		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));

		CEVec3 minAABB{ aabbPtr->position.x - aabb2Temp.max.x, aabbPtr->position.y - aabb2Temp.max.y, 0.f };
		CEVec3 maxAABB{ aabbPtr->position.x + aabb2Temp.max.x, aabbPtr->position.y + aabb2Temp.max.y, 0.f };

		
		CEVec3 pos{ spherePtr->position };

		CEVec3 closestPt{pos}; /*CLOSEST POINT OF THE CENTER OF THE CIRCLE TO THE EDGES OF THE AABB*/


		if (pos.x < minAABB.x) { closestPt.x = minAABB.x; }
		if (pos.x > maxAABB.x) { closestPt.x = maxAABB.x; }


		if (pos.y < minAABB.y) { closestPt.y = minAABB.y; }
		if (pos.y > maxAABB.y) { closestPt.y = maxAABB.y; }

		CEVec3 ClosestPtToCirc{ closestPt - pos };

		if (CEMath::LengthSq(ClosestPtToCirc) < sphere1Temp.radius*sphere1Temp.radius) {

			f32 distance{ CEMath::Length(ClosestPtToCirc) };

			result.collisionOccured = true;
			result.depth = sphere1Temp.radius - distance;
			result.normal = CEMath::Normalize(ClosestPtToCirc);

		}

		return result;
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

	/*----------------------------------------------------------------------------------------------------------*/
}