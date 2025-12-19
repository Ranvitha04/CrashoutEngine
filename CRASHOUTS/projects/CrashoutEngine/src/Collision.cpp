/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, ColliderType::AABB and Circle)

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.


//__________________________________________________________________________*/

#include "collisionmanager.h"
#include "rigidbodycomponent.h"
#include "glapp.h"
#include "collidercomponent.h"
#include "CEMath.h"
#include "CEMtx33.h"
#include "hierarchy.h"
#include <cmath>
#include <algorithm>
#include "camera.h"
#include "colliders.h"

/*UTILS RIGHT HERE*/
namespace Collision {
	std::pair<f32, f32> projOBBOnAxis(std::array<CEVec3, 4> const& vertices, CEVec3 const& axis) {
		f32 min{ CEMath::Dot(vertices[0], axis) };
		f32 max{ min };

		for (CEVec3 const& vertex : vertices) {
			f32 dot{ CEMath::Dot(vertex, axis) };

			if (dot < min) {
				min = dot;
			} if (dot > max) {
				max = dot;
			}
		}
		return { min, max };
	}

	CEVec3 const* findClosestVertex(std::array<CEVec3, 4> const& vertices, CEVec3 const& point) {
		CEVec3 const* closestPt{ &vertices[0] };
		f32 minDist{ std::numeric_limits<f32>::max() };

		for (CEVec3 const& vertex : vertices) {
			CEVec3 vertexToPoint{ point - vertex };

			f32 dist{ CEMath::LengthSq(vertexToPoint) };

			if (dist < minDist) {
				closestPt = &vertex;
				minDist = dist;
			}
		}

		return closestPt;
	}

	std::pair<f32, f32> projectCircleOntoAxis(Sphere const& sphere2Temp, CEVec3 const& center, CEVec3 const& axis) {

		CEVec3 outerPoint1{ center + axis * sphere2Temp.radius };
		CEVec3 outerPoint2{ center - axis * sphere2Temp.radius };

		f32 min{ CEMath::Dot(outerPoint1, axis) };
		f32 max{ CEMath::Dot(outerPoint2, axis) };

		if (min > max) std::swap(min, max);

		return std::pair<f32, f32>{min, max};

	}

	Polygon::SupportPoint findSupportPoint(CEVec3 const& edgeNormal, CEVec3 const&pointOnEdge, const Polygon &otherPolygon) {
		f32 currentDeepestPenetration{ -std::numeric_limits<f32>::max() };
		Polygon::SupportPoint supportPoint{ std::nullopt };

		for (int i{}; i < otherPolygon.vertexCount; ++i) {
			CEVec3 verticeWorld = otherPolygon.rotatedVertices[i];

			// Recalculate vector using WORLD space
			CEVec3 verticeToPointEdge{ verticeWorld - pointOnEdge };

			f32 penetrationDepth{ CEMath::Dot(verticeToPointEdge, -1.f * edgeNormal) };

			if (penetrationDepth > currentDeepestPenetration) {
				currentDeepestPenetration = penetrationDepth;
				supportPoint = std::make_optional<std::pair<CEVec3, f32>>({ verticeWorld, currentDeepestPenetration });
			}
		}

		if (currentDeepestPenetration <= 0.0f) return std::nullopt;

		return supportPoint;
	}

	CollisionHandler::Contact getContactPoint(const Polygon& a, const Polygon& b) {
		CollisionHandler::Contact contact{ std::nullopt };
		f32 minimumPenetration{std::numeric_limits<f32>::max()};

		for (int i{}; i < a.vertexCount; ++i) {
			CEVec3 pointOnEdge{ a.rotatedVertices[i] };
			CEVec3 normalOnEdge{ a.normalEdges[i] };

			Polygon::SupportPoint supportPoint{ findSupportPoint(normalOnEdge, pointOnEdge, b) };
			if (!supportPoint) return std::nullopt;
			if (supportPoint->second < minimumPenetration) {
				minimumPenetration = supportPoint->second;
				contact = std::make_optional(CollisionManifold{ normalOnEdge, {supportPoint->first}, minimumPenetration, true, 1 });
			}

		}

		return contact;

	}

	CollisionHandler::Contact sphereVsPolyEdges(const Sphere& a, const Polygon& b) {

		CEVec3 sphereCenter{a.position};

		std::optional<CEVec3> nearestEdgeNormal{ std::nullopt };
		std::optional<CEVec3> nearestEdgeVertex{ std::nullopt };

		for (int i{}; i < b.vertexCount; ++i) {
			int idx{ (i + 1) % b.vertexCount };
			CEVec3 vertex{ b.rotatedVertices[i] };
			CEVec3 nextVertex{ b.rotatedVertices[idx]  };

			CEVec3 vertToCirc{ sphereCenter - vertex };
			CEVec3 dirToNext{ nextVertex - vertex };
			f32 dirToNextLength{ CEMath::Length(dirToNext) };
			dirToNext = CEMath::Normalize(dirToNext);

			f32 dirToNextProjection{ CEMath::Dot(vertToCirc, dirToNext) };
			float minProj = std::numeric_limits<float>::max();

			if (dirToNextProjection > 0.f && dirToNextProjection < dirToNextLength) {
				float dist = std::abs(CEMath::Dot(b.normalEdges[i], vertToCirc));
				if (dist < minProj) {
					minProj = dist;
					nearestEdgeNormal = b.normalEdges[i];
					nearestEdgeVertex = vertex;
				}
			}
		}

		if (!nearestEdgeNormal || !nearestEdgeVertex) return std::nullopt;

		CEVec3 vertexToCircle{sphereCenter - *nearestEdgeVertex};
		f32 projToEdgeNormal{ CEMath::Dot(*nearestEdgeNormal, vertexToCircle) };
		if (projToEdgeNormal - a.radius < 0.f) {
			f32 depth{ a.radius - projToEdgeNormal };
			CEVec3 penetrationPt{ sphereCenter + (*nearestEdgeNormal * -a.radius) };
			return std::make_optional(CollisionManifold{ *nearestEdgeNormal * -1.f, {penetrationPt}, depth, true, 1 });
		}

		return std::nullopt;

	}

	CollisionHandler::Contact sphereVsPolyCorner(const Sphere& a, const Polygon& b) {

		CEVec3 sphereCenter{ a.position };

		for (int i{}; i < b.vertexCount; ++i) {
			CEVec3 vertex{ b.rotatedVertices[i] };
			CEVec3 dirToSphereCenter = CEMath::Normalize(sphereCenter - vertex);
			if (CEMath::LengthSq(dirToSphereCenter) < a.radius * a.radius) {
				f32 depth{ a.radius - CEMath::Length(dirToSphereCenter) };
				dirToSphereCenter = CEMath::Normalize(dirToSphereCenter);
				return std::make_optional(CollisionManifold{ dirToSphereCenter, {vertex}, depth, true, 1 }); /*COME BACK TO THIS LATER TOO*/
			}
		}

		return std::nullopt;
	}
}

/*ALL THE COLLISION DETECTIONS RIGHT HERE*/
namespace Collision {
	
	// SPHERE to SPHERE
	CollisionManifold CollisionHandler::SphereToSphereCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionManifold result{ .collisionOccured = false };

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(coll1.collider));

		Sphere const& sphere2Temp = dynamic_cast<Sphere const&>(*(coll2.collider));

		float totalRadius{ sphere1Temp.radius + sphere2Temp.radius };
		CEVec3 circ1ToCirc2{ coll2.position - coll1.position };
		if (CEMath::LengthSq(circ1ToCirc2) - (totalRadius * totalRadius) > 0.f) {
			result.collisionOccured = false;
			return result;
		}

		float depth = totalRadius - CEMath::Length(circ1ToCirc2);

		CEVec3 normal{ CEMath::Normalize(circ1ToCirc2) };
		float distanceToPoint{ sphere1Temp.radius - depth };
		CEVec3 contactPoint{ (normal * distanceToPoint) + coll1.position };

		result.depth = depth;
		result.normal = normal;
		result.contactPoints[0] = contactPoint;
		result.collisionOccured = true;

		return result;
	}

	// AABB to AABB
	CollisionManifold CollisionHandler::AABBToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionManifold result{ .collisionOccured = false };

		AABBRect const& aabb1Temp = dynamic_cast<AABBRect const&>(*(coll1.collider));
		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(coll2.collider));

		CEVec3 posDelta{ (coll2.position) - (coll1.position) }; /*Vector showing distance + the direction*/

		f32 xDiff{ aabb1Temp.extents.x + aabb2Temp.extents.x - std::abs(posDelta.x) };

		if (xDiff > 0.f) {
			f32 yDiff{ aabb1Temp.extents.y + aabb2Temp.extents.y - std::abs(posDelta.y) };

			if (yDiff > 0.f) {

				if (xDiff < yDiff) {
					result.collisionOccured = true;
					result.normal = posDelta.x < 0 ? CEVec3{ -1.f, 0.f, 0.f } : CEVec3{ 1.f, 0.f, 0.f };
					result.depth = xDiff;

				}
				else {
					result.collisionOccured = true;
					result.normal = posDelta.y < 0 ? CEVec3{ 0.f, -1.f, 0.f } : CEVec3{ 0.f, 1.f, 0.f };
					result.depth = yDiff;
				}

				// Get World Space Min/Max for both boxes
				CEVec3 min1 = coll1.position + aabb1Temp.offset - aabb1Temp.extents;
				CEVec3 max1 = coll1.position + aabb1Temp.offset + aabb1Temp.extents;
				CEVec3 min2 = coll2.position + aabb2Temp.offset - aabb2Temp.extents;
				CEVec3 max2 = coll2.position + aabb2Temp.offset + aabb2Temp.extents;

				// Intersect Rect
				float intersectMinX = std::max(min1.x, min2.x);
				float intersectMaxX = std::min(max1.x, max2.x);
				float intersectMinY = std::max(min1.y, min2.y);
				float intersectMaxY = std::min(max1.y, max2.y);

				// Contact is the center of that intersection
				result.contactPoints[0] = CEVec3{
					(intersectMinX + intersectMaxX) * 0.5f,
					(intersectMinY + intersectMaxY) * 0.5f,
					0.0f
				};
				result.numOfContacts = 1;
			}
		}
		return result;
	}
	
	// OBB to OBB (polygon to polygon)
	CollisionManifold CollisionHandler::PolygonToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* polygonPtr1{ &coll1 };
		CollisionProps const* polygonPtr2{ &coll2 };

		Polygon const& polygon1 = dynamic_cast<Polygon const&>(*(polygonPtr1->collider));

		Polygon const& polygon2 = dynamic_cast<Polygon const&>(*(polygonPtr2->collider));

		Contact contactA{ getContactPoint(polygon1, polygon2) };
		if (!contactA) return result;

		Contact contactB{ getContactPoint(polygon2, polygon1) };
		if (!contactB) return result;

		CollisionManifold finalManifold;

		if (contactA->depth < contactB->depth) {
			finalManifold = *contactA;
		} else {
			finalManifold = CollisionManifold{ -1.f * contactB->normal, contactB->contactPoints, contactB->depth, true, contactB->numOfContacts };
		}

		// Calculate vector from A to B
		CEVec3 centerVec = coll2.position - coll1.position;

		// If the normal points in the opposite direction of the center vector, flip it.
		if (CEMath::Dot(finalManifold.normal, centerVec) < 0.0f) {
			finalManifold.normal = -1.f * finalManifold.normal;
		}

		return finalManifold;
	}

	// MIX
	CollisionManifold CollisionHandler::SphereToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* polygonPtr{ &coll2 };

		if (spherePtr->collider->type > polygonPtr->collider->type) {
			std::swap(spherePtr, polygonPtr);
		}

		Sphere const& sphere = dynamic_cast<Sphere const&>(*(spherePtr->collider));

		Polygon const& polygon = dynamic_cast<Polygon const&>(*(polygonPtr->collider));

		std::optional<CollisionManifold> contact{ sphereVsPolyEdges(sphere, polygon) };
		if (contact) return *contact;
		else {
			std::optional<CollisionManifold> contact2{ sphereVsPolyCorner(sphere, polygon) };
			if (contact2) return *contact2;
		}

		
		return result;
	}

	CollisionManifold CollisionHandler::AABBToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* aabbPtr{ &coll1 };
		CollisionProps const* polyPtr{ &coll2 };

		// Sort inputs so we know which is which (Enum Order: AABB < POLYGON)
		if (aabbPtr->collider->type > polyPtr->collider->type) {
			std::swap(aabbPtr, polyPtr);
		}

		AABBRect const& aabb = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));
		Polygon const& existingPoly = dynamic_cast<Polygon const&>(*(polyPtr->collider));

		// Cannot use aabb.min/max directly if they are local. We use Position +/- Extents
		CEVec3 aabbPos = aabbPtr->position + aabb.offset;
		CEVec3 minWorld = aabbPos - aabb.extents;
		CEVec3 maxWorld = aabbPos + aabb.extents;

		// Create the temporary polygon in World Space
		Polygon tempPoly = Polygon::createAABB(minWorld, maxWorld, CEVec3{ 0,0,0 });

		// Ensure the temp polygon has valid normals!
		if (tempPoly.normalEdges[0] == CEVec3{ 0,0,0 }) {
			tempPoly.normalEdges[0] = CEVec3{ 0.f, -1.f, 0.f }; // Bottom Edge Normal
			tempPoly.normalEdges[1] = CEVec3{ 1.f, 0.f, 0.f };  // Right Edge Normal
			tempPoly.normalEdges[2] = CEVec3{ 0.f, 1.f, 0.f };  // Top Edge Normal
			tempPoly.normalEdges[3] = CEVec3{ -1.f, 0.f, 0.f }; // Left Edge Normal
		}

		// 3. SAT Checks
		Contact contactA{ getContactPoint(tempPoly, existingPoly) };
		if (!contactA) return result;

		Contact contactB{ getContactPoint(existingPoly, tempPoly) };
		if (!contactB) return result;

		CollisionManifold finalManifold;

		if (contactA->depth < contactB->depth) {
			finalManifold = *contactA;
		}
		else {
			finalManifold = CollisionManifold{ -1.f * contactB->normal, contactB->contactPoints, contactB->depth, true, contactB->numOfContacts };
		}

		// Calculate the vector between the actual physical centers
		CEVec3 aabbCenter = aabbPos;
		CEVec3 polyCenter = polyPtr->position + existingPoly.offset;
		CEVec3 centerToCenter = polyCenter - aabbCenter;

		// If the normal points opposite to the center-to-center vector, flip it.
		if (CEMath::Dot(finalManifold.normal, centerToCenter) < 0.0f) {
			finalManifold.normal = -1.f * finalManifold.normal;
		}

		return finalManifold;
	}

	CollisionManifold CollisionHandler::SphereToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* aabbPtr{ &coll2 };

		bool swapped = false;

		if (spherePtr->collider->type > aabbPtr->collider->type) {
			std::swap(spherePtr, aabbPtr);
			swapped = true;
		}

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(spherePtr->collider));
		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));

		// Apply Offsets (Use the collider's effective center, not just entity position)
		CEVec3 spherePos = spherePtr->position;
		CEVec3 aabbPos = aabbPtr->position;

		CEVec3 minAABB{ aabbPos.x - aabb2Temp.extents.x, aabbPos.y - aabb2Temp.extents.y, 0.f };
		CEVec3 maxAABB{ aabbPos.x + aabb2Temp.extents.x, aabbPos.y + aabb2Temp.extents.y, 0.f };

		CEVec3 closestPt{ spherePos };

		// CLAMP
		if (spherePos.x < minAABB.x) { closestPt.x = minAABB.x; }
		if (spherePos.x > maxAABB.x) { closestPt.x = maxAABB.x; }

		if (spherePos.y < minAABB.y) { closestPt.y = minAABB.y; }
		if (spherePos.y > maxAABB.y) { closestPt.y = maxAABB.y; }

		CEVec3 ClosestPtToCirc{ closestPt - spherePos };
		float closestPtSquared{ CEMath::LengthSq(ClosestPtToCirc) };

		if (closestPtSquared < sphere1Temp.radius * sphere1Temp.radius) {

			result.collisionOccured = true;
			result.numOfContacts = 1;

			// Sphere Center is INSIDE the AABB
			if (closestPtSquared == 0.f) {
				float left = spherePos.x - minAABB.x;
				float right = maxAABB.x - spherePos.x;
				float bottom = spherePos.y - minAABB.y;
				float top = maxAABB.y - spherePos.y;

				float minDist = left;
				result.normal = CEVec3{ -1.f, 0.f, 0.f };

				if (right < minDist) {
					minDist = right;
					result.normal = CEVec3{ 1.f, 0.f, 0.f };
				}
				if (bottom < minDist) {
					minDist = bottom;
					result.normal = CEVec3{ 0.f, -1.f, 0.f };
				}
				if (top < minDist) {
					minDist = top;
					result.normal = CEVec3{ 0.f, 1.f, 0.f };
				}

				result.depth = sphere1Temp.radius + minDist;
				result.contactPoints[0] = spherePos - result.normal * minDist;
			}
			// Sphere Center is OUTSIDE (touching edge/corner)
			else {
				f32 distance{ CEMath::Length(ClosestPtToCirc) };

				result.depth = sphere1Temp.radius - distance;
				result.normal = -1.f * CEMath::Normalize(ClosestPtToCirc);
				result.contactPoints[0] = closestPt;
			}

			// Handle swapped normal
			if (swapped) {
				result.normal = -1.f * result.normal;
			}


			CEVec3 centerDir = aabbPos - spherePos;
			if (CEMath::Dot(result.normal, centerDir) < 0.f) {
				result.normal = -result.normal;
			}
		}

		return result;
	}
}

/*MAIN UPDATE STUFF!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
namespace Collision {

	/*--------------------------------------COLLISION MESSAGE DEFINITION---------------------------------------*/

	CollisionMessage::CollisionMessage(const Messaging::EventType& type, CollisionManifold const& colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2)
		: Messages::Messages{ type }, colManifold{ colManifold }, entity1{ entity1 }, entity2{ entity2 }
	{ /*EMPTY BY DESIGN*/ }

	std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> CollisionMessage::GetCollisionPair() {
		return std::pair{ entity1, entity2 };
	}

	CollisionManifold CollisionMessage::GetCollisionManifold() {
		return colManifold;
	}
	/*----------------------------------------------------------------------------------------------------------*/


	/*----------------COLLISIONHANDLER CLASS DEFINITIONS--------------------------------------------------------*/

	CollisionHandler::CollisionHandler() : Messaging::Observable{}, collideFuncts{}
	{
		int sphere = static_cast<int>(ColliderType::SPHERE);   // 0
		int aabb = static_cast<int>(ColliderType::AABB);     // 1
		int poly = static_cast<int>(ColliderType::POLYGON);  // 4

		quadTree.Resize(AABBRect{ CEVec3{-2000.f, -2000.f, 0.f}, CEVec3{2000.f, 2000.f, 0.f} });

		/*Fill in the table of collisions, e.g collideFuncts[0] will be ColliderType::SPHERE TO ColliderType::SPHERE collision (2 total collider types will have 2^2 permutations(different types of collisions possible) ) */
		
		// Sphere vs Sphere
		collideFuncts[sphere * SHAPE_COUNT + sphere] = SphereToSphereCollision;

		// AABB vs AABB
		collideFuncts[aabb * SHAPE_COUNT + aabb] = AABBToAABBCollision;

		// Sphere vs AABB (Sphere is 0, AABB is 1. 0 < 1, so index is 0 * N + 1)
		collideFuncts[sphere * SHAPE_COUNT + aabb] = SphereToAABBCollision;

		// AABB vs Polygon (AABB is 1, Poly is 4. 1 < 4)
		collideFuncts[aabb * SHAPE_COUNT + poly] = AABBToPolygonCollision;

		// Sphere vs Polygon (Sphere is 0, Poly is 4. 0 < 4)
		collideFuncts[sphere * SHAPE_COUNT + poly] = SphereToPolygonCollision;

		// Polygon vs Polygon
		collideFuncts[poly * SHAPE_COUNT + poly] = PolygonToPolygonCollision;

	}

	void CollisionHandler::Update([[maybe_unused]] double deltaTime, EngineState state) {

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		// 1. Get ALL entities with Colliders (not just RigidBodies)
		std::vector<CrashoutsEngine::Entity> colliderEntities =
			globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity;

		if (state == EngineState::PAUSE) return;
		if (state == EngineState::STOP) {
			lastFrameCollision.clear();
			return;
		}

		/*BROADPHASE*/
		// 2. Insert into QuadTree
		quadTree.Clear();
		for (CrashoutsEngine::Entity& entity1 : colliderEntities) { // Changed from globalEntities
			if (!globalECS.HasComponent<Component::TransformComponent>(entity1)) continue;
			if (!globalECS.GetComponent<Component::ColliderComponent>(entity1)->collider->enabled) continue;
			quadTree.Insert(entity1);
		}

		/*DETECT COLLISION AND UPDATE COLLISION EVENTS MEMBER*/
		for (CrashoutsEngine::Entity& entity1 : colliderEntities) {

			if (!globalECS.HasComponent<Component::TransformComponent>(entity1)) continue;			
			if(!globalECS.GetComponent<Component::ColliderComponent>(entity1)->collider->enabled) continue;

			std::vector<CrashoutsEngine::Entity> collidableObjects{};
			Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity1) };
			Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(entity1) };

			AABBRect region{ GetAABB(*(collider1->collider), transform1->position) };
			quadTree.Query(region, collidableObjects);

			for (CrashoutsEngine::Entity const& entity2 : collidableObjects) {
				if (entity1 == entity2) continue;
				if (CrashoutsEngine::IsAlreadyChildOf(globalECS, entity1, entity2) ||
					CrashoutsEngine::IsAlreadyChildOf(globalECS, entity2, entity1)) continue;

				Component::ColliderComponent* collider2{ globalECS.GetComponent<Component::ColliderComponent>(entity2) };
				Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(entity2) };
				
				Collision::CollisionManifold collisionManifold;

				// Optimization: Skip two static rigidbodies
				if (globalECS.HasComponent<Component::RigidBodyComponent>(entity1) &&
					globalECS.HasComponent<Component::RigidBodyComponent>(entity2)) {
					auto* rb1 = globalECS.GetComponent<Component::RigidBodyComponent>(entity1);
					auto* rb2 = globalECS.GetComponent<Component::RigidBodyComponent>(entity2);
					if (rb1->getMass() == 0.f && rb2->getMass() == 0.f) continue;
					if (rb1 == nullptr || rb2 == nullptr) continue;

					// Check Collision
					collisionManifold = callCollFunct(collider1->collider->type, collider2->collider->type)(
						Collision::CollisionProps{ collider1->collider, rb1->GetVelocity(), transform1->position},
						Collision::CollisionProps{ collider2->collider, rb2->GetVelocity(), transform2->position}
					);

				}
				
				if (collisionManifold.collisionOccured) {
					CrashoutsEngine::Entity const* entity1Ptr{ &entity1 };
					CrashoutsEngine::Entity const* entity2Ptr{ &entity2 };

					// Ensure consistent entity ordering (ID A < ID B)
					if (collider1->collider->type > collider2->collider->type) std::swap(entity1Ptr, entity2Ptr);
					if (*entity1Ptr > *entity2Ptr) std::swap(entity1Ptr, entity2Ptr);

					// If EITHER collider is a trigger, it is a Trigger Event.
					// Otherwise, it is a Collision Event.
					bool isTriggerInteraction = collider1->collider->isTrigger || collider2->collider->isTrigger;

					using namespace Messaging;
					EventType enterType = isTriggerInteraction ? EventType::TriggerEnter : EventType::CollisionEnter;
					EventType stayType = isTriggerInteraction ? EventType::TriggerStay : EventType::CollisionStay;

					// Check if this pair was already processed this frame (e.g. duplicate query)
					bool isIn{ false };
					for (ContactInformation const& test : currentFrameCollision) {
						if (test.contacts.first == *entity1Ptr && test.contacts.second == *entity2Ptr) isIn = true;
					}

					if (!isIn) {
						ContactPair pair{ *entity1Ptr, *entity2Ptr };

						// Check if this pair existed Last Frame
						if (isPairInCollection(pair, lastFrameCollision)) {
							// STAY
							CollisionMessage stayMessage{ stayType, collisionManifold, *entity1Ptr, *entity2Ptr };
							sendMessage(stayMessage);
						}
						else {
							// ENTER
							CollisionMessage enterMessage{ enterType, collisionManifold, *entity1Ptr, *entity2Ptr };
							sendMessage(enterMessage);
						}

						// Save contact
						currentFrameCollision.push_back(std::move(ContactInformation{ pair, collisionManifold }));
					}
				}
			}
		}

		// --- EXIT EVENT LOOP ---
		for (ContactInformation const& contactInfo : lastFrameCollision) {
			// If contact existed last frame but NOT this frame -> EXIT
			if (!isPairInCollection(contactInfo.contacts, currentFrameCollision)) {

				using namespace Messaging;
				EventType exitType = EventType::CollisionExit; // Default to Collision

				// Re-check if it was a trigger to send the right Exit message.
				// Safely get components (entities might have been destroyed)
				if (globalECS.HasComponent<Component::ColliderComponent>(contactInfo.contacts.first) &&
					globalECS.HasComponent<Component::ColliderComponent>(contactInfo.contacts.second))
				{
					auto* c1 = globalECS.GetComponent<Component::ColliderComponent>(contactInfo.contacts.first);
					auto* c2 = globalECS.GetComponent<Component::ColliderComponent>(contactInfo.contacts.second);

					if (c1->collider->isTrigger || c2->collider->isTrigger) {
						exitType = EventType::TriggerExit;
					}
				}

				CollisionMessage exitMessage{ exitType, contactInfo.colManifold, contactInfo.contacts.first, contactInfo.contacts.second };
				sendMessage(exitMessage);
			}
		}

		// Swap Buffers
		std::swap(currentFrameCollision, lastFrameCollision);
		currentFrameCollision.clear();

	}

	FunctionDispatchType CollisionHandler::callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2) {

		/*Swap the type according to enum order, e.g function exists for sphere to aabb but not aabb to sphere*/
		if (type1 > type2) {
			std::swap(type1, type2);
		}

		int idx1{ static_cast<int>(type1) };
		int idx2{ static_cast<int>(type2) };
		//std::cout << " size of collidefuncts is " << collideFuncts.size() << std::endl;
		return collideFuncts[idx1 * SHAPE_COUNT + idx2];
	}

	/*----------------------------------------------------------------------------------------------------------*/
}