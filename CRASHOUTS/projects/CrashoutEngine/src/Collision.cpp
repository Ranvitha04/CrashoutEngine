/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, ColliderType::AABB and Circle)


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

namespace {

	std::pair<f32, f32> ComputeWorldDimensions() {
		f32 worldWidth{}, worldHeight{};
		const float BASE_SIZE = 100.0f;
		float aspect = static_cast<float>(GLApp::width) / static_cast<float>(GLApp::height);
		if (aspect >= 1.0f) {
			worldHeight = BASE_SIZE;
			worldWidth = worldHeight * aspect;
		}
		else {
			worldWidth = BASE_SIZE;
			worldHeight = worldWidth / aspect;
		}

		return std::pair{ worldWidth, worldHeight };
	}

}

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
		f32 minDist{ std::numeric_limits<f32>::min() };

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
		f32 currentDeepestPenetration{};
		Polygon::SupportPoint supportPoint{ std::nullopt };

		for (int i{}; i < otherPolygon.vertexCount; ++i) {
			CEVec3 vertice{ otherPolygon.rotatedVertices[i] };
			CEVec3 verticeToPointEdge{ vertice - pointOnEdge};
			f32 penetrationDepth{CEMath::Dot(verticeToPointEdge, -1.f * edgeNormal)};

			if (penetrationDepth > currentDeepestPenetration) {
				currentDeepestPenetration = penetrationDepth;
				supportPoint = std::make_optional<std::pair<CEVec3, f32>>({ vertice, currentDeepestPenetration});
			}
		}

		return supportPoint;
	}

	CollisionHandler::Contact getContactPoint(const Polygon& a, const Polygon& b) {
		CollisionHandler::Contact contact{ std::nullopt };
		f32 minimumPenetration{std::numeric_limits<f32>::max()};

		for (int i{}; i < a.vertexCount; ++i) {
			CEVec3 pointOnEdge{ a.rotatedVertices[i]  };
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
			f32 dirToNormalProjection{ CEMath::Dot(vertToCirc, b.normalEdges[i]) };
			if (dirToNormalProjection>=0.f && dirToNextProjection > 0.f && dirToNextProjection < dirToNextLength) {
				nearestEdgeNormal = std::make_optional<CEVec3>(b.normalEdges[i]);
				nearestEdgeVertex = std::make_optional<CEVec3>(vertex);
			}
		}

		if (!nearestEdgeNormal || !nearestEdgeVertex) return std::nullopt;

		CEVec3 vertexToCircle{sphereCenter - *nearestEdgeVertex};
		f32 projToEdgeNormal{ CEMath::Dot(*nearestEdgeNormal, vertexToCircle) };
		if (projToEdgeNormal - a.radius < 0.f) {
			f32 depth{projToEdgeNormal - a.radius};
			CEVec3 penetrationPt{sphereCenter + (*nearestEdgeNormal * (a.radius*-1.f))};
			return std::make_optional(CollisionManifold{ *nearestEdgeNormal * -1.f, {penetrationPt}, depth * -1.f, true, 1 }); /*COME BACK TO THIS PLS*/
		}

		return std::nullopt;

	}

	CollisionHandler::Contact sphereVsPolyCorner(const Sphere& a, const Polygon& b) {

		CEVec3 sphereCenter{ a.position };

		for (int i{}; i < b.vertexCount; ++i) {
			CEVec3 vertex{ b.rotatedVertices[i] };
			CEVec3 dirToSphereCenter{ vertex - sphereCenter };
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
	CollisionManifold CollisionHandler::OBBToOBBCollision([[maybe_unused]] CollisionProps const& coll1, [[maybe_unused]] CollisionProps const& coll2) {
		CollisionManifold result{ .depth = std::numeric_limits<f32>::max(), .collisionOccured = true };

		CollisionProps const* obbPtr1{ &coll1 };
		CollisionProps const* obbPtr2{ &coll2 };

		OBBRect const& obbTemp1 = dynamic_cast<OBBRect const&>(*(obbPtr1->collider));
		OBBRect const& obbTemp2 = dynamic_cast<OBBRect const&>(*(obbPtr2->collider));

		CEVec3 smallestAxis{};
		f32 depthAmount{ std::numeric_limits<float>::max() };

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp1.rotatedVertices, obbTemp1.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(obbTemp2.rotatedVertices, obbTemp1.normalEdges[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = obbTemp1.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = -1.f * obbTemp1.normalEdges[i];
				}
			}
		}

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp1.rotatedVertices, obbTemp2.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(obbTemp2.rotatedVertices, obbTemp2.normalEdges[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = obbTemp2.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = -1.f * obbTemp2.normalEdges[i];
				}
			}
		}

		result.depth = depthAmount;
		result.normal = smallestAxis;


		return result;
	}

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

			}
		}
		return result;
	}
	
	/*------------------------ColliderType::POLYGON COLLISIONS-----------------------------------*/

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

		if (contactA->depth < contactB->depth) {
			return *contactA;
		} else {
			return CollisionManifold{ -1.f * contactB->normal, contactB->contactPoints ,contactB->depth, true, contactB->numOfContacts };
		}
	}

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

		CollisionProps const* polygonPtr1{ &coll1 };
		CollisionProps const* polygonPtr2{ &coll2 };

		if (polygonPtr1->collider->type > polygonPtr2->collider->type) {
			std::swap(polygonPtr1, polygonPtr2);
		}

		AABBRect const& aabb = dynamic_cast<AABBRect const&>(*(polygonPtr1->collider));

		Polygon const& polygon1{ Polygon::createAABB(aabb.min, aabb.max, aabb.offset) };

		Polygon const& polygon2 = dynamic_cast<Polygon const&>(*(polygonPtr2->collider));

		Contact contactA{ getContactPoint(polygon1, polygon2) };
		if (!contactA) return result;

		Contact contactB{ getContactPoint(polygon2, polygon1) };
		if (!contactB) return result;

		if (contactA->depth < contactB->depth) {
			return *contactA;
		}
		else {
			return CollisionManifold{ -1.f * contactB->normal, contactB->contactPoints ,contactB->depth, true, contactB->numOfContacts };
		}

	}

	/*----------------------------ColliderType::AABB COLLISIONS--------------------------------------------------------------*/

	CollisionManifold CollisionHandler::SphereToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* aabbPtr{ &coll2 };
		if (spherePtr->collider->type > aabbPtr->collider->type) {
			std::swap(spherePtr, aabbPtr);
		}

		Sphere const& sphere1Temp = dynamic_cast<Sphere const&>(*(spherePtr->collider));

		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));

		CEVec3 minAABB{ (aabbPtr->position.x ) - aabb2Temp.extents.x, (aabbPtr->position.y) - aabb2Temp.extents.y, 0.f };
		CEVec3 maxAABB{ (aabbPtr->position.x) + aabb2Temp.extents.x, (aabbPtr->position.y) + aabb2Temp.extents.y, 0.f };


		CEVec3 pos{ spherePtr->position  };

		CEVec3 closestPt{ pos }; /*CLOSEST POINT OF THE CENTER OF THE CIRCLE TO THE EDGES OF THE ColliderType::AABB*/

		/*CLAMP THE POINT OF THE CENTER OF THE CIRCLE TO THE POINT ON THE SIDE OF ColliderType::AABB*/
		if (pos.x < minAABB.x) { closestPt.x = minAABB.x; }
		if (pos.x > maxAABB.x) { closestPt.x = maxAABB.x; }

		if (pos.y < minAABB.y) { closestPt.y = minAABB.y; }
		if (pos.y > maxAABB.y) { closestPt.y = maxAABB.y; }

		CEVec3 ClosestPtToCirc{ closestPt - pos };
		float closestPtSquared{ CEMath::LengthSq(ClosestPtToCirc) };


		if (closestPtSquared < sphere1Temp.radius * sphere1Temp.radius) {


			result.collisionOccured = true;

			/*Center of Circle is already inside ColliderType::AABB*/
			if (closestPtSquared == 0.f) {
				CEVec3 vecFromAABBToCirc{ spherePtr->position - aabbPtr->position };

				float depthX{ sphere1Temp.radius + aabb2Temp.extents.x - fabsf(vecFromAABBToCirc.x) };

				float depthY{ sphere1Temp.radius + aabb2Temp.extents.y - fabsf(vecFromAABBToCirc.y) };

				if (depthX < depthY) {
					result.depth = depthX;
					result.normal = vecFromAABBToCirc.x < 0.f ? CEVec3{ 1.f, 0.f, 0.f } : CEVec3{ -1.f, 0.f, 0.f };

				}
				else {
					result.depth = depthY;
					result.normal = vecFromAABBToCirc.y < 0.f ? CEVec3{ 0.f, 1.f, 0.f } : CEVec3{ 0.f, -1.f, 0.f };
				}


			}
			else {
				f32 distance{ CEMath::Length(ClosestPtToCirc) };

				result.depth = sphere1Temp.radius - distance;
				result.normal = CEMath::Normalize(ClosestPtToCirc);
			}

		}

		return result;
	}

	CollisionManifold CollisionHandler::SphereToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .depth = std::numeric_limits<f32>::max(), .collisionOccured = true };

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* obbPtr{ &coll2 };
		if (spherePtr->collider->type > obbPtr->collider->type) {
			std::swap(spherePtr, obbPtr);
		}

		Sphere const& sphereTemp = dynamic_cast<Sphere const&>(*(spherePtr->collider));
		OBBRect const& obbTemp = dynamic_cast<OBBRect const&>(*(obbPtr->collider));

		CEVec3 smallestAxis{};
		f32 depthAmount{ std::numeric_limits<float>::max() };

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp.rotatedVertices, obbTemp.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projectCircleOntoAxis(sphereTemp, spherePtr->position, obbTemp.normalEdges[i]) };

			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };

			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = -1.f * obbTemp.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = obbTemp.normalEdges[i];
				}
			}
		}

		CEVec3 const* closestPt{ findClosestVertex(obbTemp.rotatedVertices, spherePtr->position) };

		CEVec3 circleCenterToPtAxis{ *closestPt - spherePtr->position };
		circleCenterToPtAxis = CEMath::Normalize(circleCenterToPtAxis);

		std::pair<f32, f32> projection1{ projOBBOnAxis(obbTemp.rotatedVertices, circleCenterToPtAxis) };
		std::pair<f32, f32> projection2{ projectCircleOntoAxis(sphereTemp, spherePtr->position, circleCenterToPtAxis) };

		f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };

		if (overlap < 0.f) {
			result.collisionOccured = false;
		}

		if (overlap < depthAmount) {
			depthAmount = overlap;
			smallestAxis = -1.f * circleCenterToPtAxis;
			if (projection1.second > projection2.second) {
				smallestAxis = circleCenterToPtAxis;
			}
		}


		result.depth = depthAmount;
		result.normal = smallestAxis;


		return result;
	}

	CollisionManifold CollisionHandler::AABBToOBBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .depth = std::numeric_limits<f32>::max(), .collisionOccured = true };

		CollisionProps const* aabbPtr{ &coll1 };
		CollisionProps const* obbPtr{ &coll2 };
		if (aabbPtr->collider->type > obbPtr->collider->type) {
			std::swap(obbPtr, aabbPtr);
		}

		AABBRect const& aabb1Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));
		OBBRect const& obb2Temp = dynamic_cast<OBBRect const&>(*(obbPtr->collider));

		std::array<CEVec3, 4> aabbVertex{
			CEVec3{aabb1Temp.min.x, aabb1Temp.min.y, 0.0f },
			{aabb1Temp.min.x, aabb1Temp.min.y + aabb1Temp.extents.y * 2.f, 0.0f },
			CEVec3{aabb1Temp.max.x, aabb1Temp.max.y, 0.0f },
			{aabb1Temp.min.x + aabb1Temp.extents.x * 2.f, aabb1Temp.min.y, 0.0f}
		};

		std::array<CEVec3, 2> aabbNormals{
			CEVec3{-1.f, 0.f, 0.0f},
			CEVec3{0.f, 1.f, 0.0f}
		};


		CEVec3 smallestAxis{};
		f32 depthAmount{ std::numeric_limits<float>::max() };

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obb2Temp.rotatedVertices, obb2Temp.normalEdges[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(aabbVertex, obb2Temp.normalEdges[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = -1.f * obb2Temp.normalEdges[i];
				if (projection1.second > projection2.second) {
					smallestAxis = 1.f * obb2Temp.normalEdges[i];
				}
			}
		}

		for (int i{}; i < 2; ++i) {
			std::pair<f32, f32> projection1{ projOBBOnAxis(obb2Temp.rotatedVertices, aabbNormals[i]) };
			std::pair<f32, f32> projection2{ projOBBOnAxis(aabbVertex, aabbNormals[i]) };
			f32 overlap{ std::min(projection1.second, projection2.second) - std::max(projection1.first, projection2.first) };
			if (overlap < 0.f) {
				result.collisionOccured = false;
				break;
			}

			if (overlap < depthAmount) {
				depthAmount = overlap;
				smallestAxis = -1.f * aabbNormals[i];
				if (projection1.second > projection2.second) {
					smallestAxis = 1.f * aabbNormals[i];
				}
			}
		}

		result.depth = depthAmount;
		result.normal = smallestAxis;

		return result;
	}
}

/*MAIN UPDATE STUFF!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
namespace Collision {

	/*--------------------------------------COLLISION MESSAGE DEFINITION---------------------------------------*/

	CollisionMessage::CollisionMessage(const std::string& id, CollisionManifold const& colManifold, CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2)
		: Messages::Messages{ id }, colManifold{ colManifold }, entity1{ entity1 }, entity2{ entity2 }
	{ /*EMPTY BY DESIGN*/ }

	std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> CollisionMessage::getCollisionPair() {
		return std::pair{ entity1, entity2 };
	}

	CollisionManifold CollisionMessage::getCollisionManifold() {
		return colManifold;
	}
	/*----------------------------------------------------------------------------------------------------------*/


	/*----------------COLLISIONHANDLER CLASS DEFINITIONS--------------------------------------------------------*/

	CollisionHandler::CollisionHandler() : Messaging::Observable{ "Collision" }, collideFuncts{}
	{
		int sphere = static_cast<int>(ColliderType::SPHERE);   // 0
		int aabb = static_cast<int>(ColliderType::AABB);     // 1
		//int obb = static_cast<int>(ColliderType::OBB);      // 2
		int poly = static_cast<int>(ColliderType::POLYGON);  // 4

		quadTree.Resize(AABBRect{ CEVec3{-2000.f, -2000.f, 0.f}, CEVec3{2000.f, 2000.f, 0.f} });

		/*currentFrameCollision.reserve(100);
		lastFrameCollision.reserve(100);*/

		/*SOON TO BE DEPRECATED*/
		//int OBBIdx{ static_cast<int>(ColliderType::OBB) };

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
		/*collideFuncts[AABBIdx * TYPES_OF_COLLISION + OBBIdx] = AABBToOBBCollision;
		collideFuncts[sphereIdx * TYPES_OF_COLLISION + OBBIdx] = SphereToOBBCollision;
		collideFuncts[OBBIdx * TYPES_OF_COLLISION + OBBIdx] = OBBToOBBCollision;*/
	}

	void CollisionHandler::update([[maybe_unused]] double deltaTime, EngineState state) {

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		std::vector<CrashoutsEngine::Entity> globalEntities = globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity;

		switch (state) {
			case EngineState::PAUSE:
				return;
			case EngineState::STOP:
				lastFrameCollision.clear();
				return;
			default:
				break;
		}

		

		std::pair<f32, f32> newDimen{ ComputeWorldDimensions() };

		newDimen.first *= 1.2f;
		newDimen.second *= 1.2f;

		CEVec3 newExtents{ newDimen.first * 0.5f, newDimen.second * 0.5f, 0.f };

		/*CEVec3 pos{ CrashoutsEngine::RenderCamera::gameCamera.GetPosition() };

		AABBRect bounds{pos - newExtents, pos + newExtents};*/

		quadTree.Clear();

		/*BROADPHASE*/
		for (CrashoutsEngine::Entity& entity1 : globalEntities) {
			quadTree.Insert(entity1);
		}

		/*DETECT COLLISION AND UPDATE COLLISION EVENTS MEMBER*/
		for (CrashoutsEngine::Entity& entity1 : globalEntities) {

			if (!globalECS.HasComponent<Component::TransformComponent>(entity1)) continue;

			

			std::vector<CrashoutsEngine::Entity> collidableObjects{};

			Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity1) };
			Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(entity1) };
			AABBRect region{ GetAABB(*(collider1->collider), transform1->position) };

			//if (!bounds.contains(region)) continue;

			

			

			quadTree.Query(region, collidableObjects);

			for (CrashoutsEngine::Entity const& entity2 : collidableObjects) {
				if (entity1 == entity2 || CrashoutsEngine::IsAlreadyChildOf(globalECS, entity1, entity2) || CrashoutsEngine::IsAlreadyChildOf(globalECS, entity2, entity1)) continue;
				
				
				/**/
				if (globalECS.HasComponent<Component::RigidBodyComponent>(entity1) && globalECS.HasComponent<Component::RigidBodyComponent>(entity2)) {
					Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(entity1) };
					Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(entity2) };

					if (rigidBody1->getMass() == 0.f && rigidBody2->getMass() == 0.f) continue;

				}
				
				
				CrashoutsEngine::Entity const* entity1Ptr{ &entity1 };
				CrashoutsEngine::Entity const* entity2Ptr{ &entity2 };

				Component::ColliderComponent* collider2{ globalECS.GetComponent<Component::ColliderComponent>(entity2) };
				//Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(entity2) };

				/*CHECK FOR COLLISION and call collision function*/
				Collision::CollisionManifold collisionManifold{};

				Component::TransformComponent newTransform1{ *transform1 };


				collisionManifold = callCollFunct(collider1->collider->type, collider2->collider->type)(
					Collision::CollisionProps{
						collider1->collider,
						CEVec3{},
						collider1->collider->position
					},
					Collision::CollisionProps{
						collider2->collider,
						CEVec3{},
						collider2->collider->position
					}
					);

				if (collisionManifold.collisionOccured) {

					if (collider1->collider->type > collider2->collider->type) {
						std::swap(entity1Ptr, entity2Ptr);
					}
					CollisionMessage colMessage{ Messaging::getCollisionMsgID(), collisionManifold, *entity1Ptr, *entity2Ptr };

					bool isIn{ false };

					if (*entity1Ptr > *entity2Ptr) std::swap(entity1Ptr, entity2Ptr);

					for (ContactInformation const& test : currentFrameCollision) {
						if (test.contacts.first == *entity1Ptr && test.contacts.second == *entity2Ptr) isIn = true;
					}

					/*UNIQUE PAIR*/
					if (!isIn) {
						ContactPair pair{ *entity1Ptr, *entity2Ptr };

						CollisionMessage stayMessage{ Messaging::getStayMsgID(), collisionManifold, *entity1Ptr, *entity2Ptr };
						CollisionMessage enterMessage{ Messaging::getEnterMsgID(), collisionManifold, *entity1Ptr, *entity2Ptr };

						if (isPairInCollection(pair, lastFrameCollision)) {
							sendMessage(stayMessage);
						}
						else {
							sendMessage(enterMessage);
						}

						currentFrameCollision.push_back(std::move(ContactInformation{ pair, collisionManifold }));
						sendMessage(colMessage);
					}

				}
			}
		}

		/*EVENT MANAGER STUFF*/

		for (ContactInformation const& contactInfo : lastFrameCollision) {

			CollisionMessage exitMessage{ Messaging::getExitMsgID(), contactInfo.colManifold, contactInfo.contacts.first, contactInfo.contacts.second };
			if (!isPairInCollection(contactInfo.contacts, currentFrameCollision)) {
				sendMessage(exitMessage);
			}
		}

		/*-------------------*/
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