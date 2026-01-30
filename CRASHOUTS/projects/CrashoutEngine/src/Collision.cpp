/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, ColliderType::AABB and Circle)

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.


//__________________________________________________________________________*/

#include "collisionmanager.h"
#include "rigidbodycomponent.h"
#include "activecomponent.h"
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
	std::pair<f32, f32> projOBBOnAxis(Polygon::Vertices const& vertices, CEVec3 const& axis) {
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

	std::pair<f32, f32> projectCircleOntoAxis(Circle const& sphere2Temp, CEVec3 const& center, CEVec3 const& axis) {

		CEVec3 outerPoint1{ center + axis * sphere2Temp.radius };
		CEVec3 outerPoint2{ center - axis * sphere2Temp.radius };

		f32 min{ CEMath::Dot(outerPoint1, axis) };
		f32 max{ CEMath::Dot(outerPoint2, axis) };

		if (min > max) std::swap(min, max);

		return std::pair<f32, f32>{min, max};

	}

	Polygon::SupportPoint findSupportPoint(CEVec3 const& edgeNormal, CEVec3 const& pointOnEdge, const Polygon& otherPolygon) {
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
		f32 minimumPenetration{ std::numeric_limits<f32>::max() };

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

	CollisionHandler::Contact sphereVsPolyEdges(const Circle& a, const Polygon& b) {

		CEVec3 sphereCenter{ a.position };

		std::optional<CEVec3> nearestEdgeNormal{ std::nullopt };
		std::optional<CEVec3> nearestEdgeVertex{ std::nullopt };

		float minProj = std::numeric_limits<float>::max();

		for (int i{}; i < b.vertexCount; ++i) {
			int idx{ (i + 1) % b.vertexCount };
			CEVec3 vertex{ b.rotatedVertices[i] };
			CEVec3 nextVertex{ b.rotatedVertices[idx] };

			CEVec3 vertToCirc{ sphereCenter - vertex };
			CEVec3 dirToNext{ nextVertex - vertex };
			f32 dirToNextLength{ CEMath::Length(dirToNext) };
			dirToNext = CEMath::Normalize(dirToNext);

			f32 dirToNextProjection{ CEMath::Dot(vertToCirc, dirToNext) };

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

		CEVec3 vertexToCircle{ sphereCenter - *nearestEdgeVertex };
		f32 projToEdgeNormal{ CEMath::Dot(*nearestEdgeNormal, vertexToCircle) };
		if (projToEdgeNormal - a.radius < 0.f) {
			f32 depth{ a.radius - projToEdgeNormal };
			CEVec3 penetrationPt{ sphereCenter + (*nearestEdgeNormal * -a.radius) };
			return std::make_optional(CollisionManifold{ *nearestEdgeNormal, {penetrationPt}, depth, true, 1 });
		}

		return std::nullopt;

	}

	CollisionHandler::Contact sphereVsPolyCorner(const Circle& a, const Polygon& b) {

		CEVec3 sphereCenter{ a.position };

		for (int i{}; i < b.vertexCount; ++i) {
			CEVec3 vertex{ b.rotatedVertices[i] };
			CEVec3 vertToCenter = sphereCenter - vertex;
			float distSq = CEMath::LengthSq(vertToCenter);

			if (distSq < a.radius * a.radius) {
				float distance = std::sqrt(distSq);

				if (distance == 0.0f) return std::nullopt;

				f32 depth{ a.radius - distance };

				CEVec3 normal = vertToCenter * (1.0f / distance);

				return std::make_optional(CollisionManifold{ normal, {vertex}, depth, true, 1 });
			}
		}

		return std::nullopt;
	}

	// --- Helper: Get Support Point ---
	// Returns the corner of the AABB furthest in the direction of 'dir'
	CEVec3 GetAABBSupport(const AABBRect& box, const CEVec3& dir) {
		CEVec3 center{ box.position };
		return {
			dir.x >= 0 ? center.x + box.extents.x : center.x - box.extents.x,
			dir.y >= 0 ? center.y + box.extents.y : center.y - box.extents.y,
			0.f
		};
	}

	// --- Helper: Project Segment onto Axis ---
	void ProjectSegment(const LineSegment& seg, const CEVec3& axis, float& min, float& max) {
		float dot1 = CEMath::Dot(seg.GetWorldP1(), axis);
		float dot2 = CEMath::Dot(seg.GetWorldP2(), axis);
		min = std::min(dot1, dot2);
		max = std::max(dot1, dot2);
	}

	// --- Helper: Project AABB onto Axis ---
	void ProjectAABB(const AABBRect& box, const CEVec3& axis, float& min, float& max) {
		CEVec3 center{ box.position };
		float centerProj = CEMath::Dot(center, axis);
		// Project half-extents (radius calculation)
		// We take absolute value of dot products to map half-extents to the axis
		float r = box.extents.x * std::abs(axis.x) + box.extents.y * std::abs(axis.y);
		min = centerProj - r;
		max = centerProj + r;
	}

	/*
	* @brief Find the point on segment AB that is closed to point P
	* @param A - Vertex A
	* @param B - Vertex B
	* @param P - Point P
	* @return The point on AB closed to P
	*/
	CEVec3 ClosestPointOnSegment(CEVec3 const& A, CEVec3 const& B, CEVec3 const& P) {
		CEVec3 AP = P - A;
		CEVec3 AB = B - A;

		f32 ab2 = CEMath::LengthSq(AB);
		f32 apDotAb = CEMath::Dot(AP, AB);

		// Projection Ratio
		f32 t = apDotAb / ab2;

		// Clamp t to segment [0, 1]
		if (t < 0.f) t = 0.f;
		if (t > 1.f) t = 1.f;

		return A + (AB * t);
	}
}

/*ALL THE COLLISION DETECTIONS RIGHT HERE*/
namespace Collision {

	// CIRCLE to CIRCLE
	CollisionManifold CollisionHandler::CircleToCircleCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionManifold result{ .collisionOccured = false };

		Circle const& sphere1Temp = dynamic_cast<Circle const&>(*(coll1.collider));

		Circle const& sphere2Temp = dynamic_cast<Circle const&>(*(coll2.collider));

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

		CEVec3 posDelta{ (coll2.position + aabb2Temp.offset) - (coll1.position + aabb1Temp.offset) }; /*Vector showing distance + the direction*/

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
		}
		else {
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
	CollisionManifold CollisionHandler::CircleToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2) {

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* polygonPtr{ &coll2 };
		bool swapped = false;

		if (spherePtr->collider->type > polygonPtr->collider->type) {
			std::swap(spherePtr, polygonPtr);
			swapped = true;
		}

		Circle const& circle = dynamic_cast<Circle const&>(*(spherePtr->collider));
		Polygon const& polygon = dynamic_cast<Polygon const&>(*(polygonPtr->collider));

		// SAT IMPLEMENTATION (Replaces Edge/Corner helper logic for robustness)
		float minOverlap = std::numeric_limits<float>::max();
		CEVec3 bestAxis{ 0.f, 0.f, 0.f };

		// 1. Test Polygon Axes (Edge Normals)
		for (int i = 0; i < polygon.vertexCount; ++i) {
			CEVec3 axis = polygon.normalEdges[i];

			// Project Polygon
			auto [minP, maxP] = projOBBOnAxis(polygon.rotatedVertices, axis);

			// Project Circle
			float cProj = CEMath::Dot(circle.position, axis);
			float minC = cProj - circle.radius;
			float maxC = cProj + circle.radius;

			// Calc Overlap
			float overlap = std::min(maxP, maxC) - std::max(minP, minC);

			if (overlap < 0) return { .collisionOccured = false };

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
			}
		}

		// 2. Test Circle Axis (Center to Closest Vertex)
		CEVec3 closestVert = polygon.rotatedVertices[0];
		float minDistSq = std::numeric_limits<float>::max();

		for (int i = 0; i < polygon.vertexCount; ++i) {
			float d = CEMath::LengthSq(circle.position - polygon.rotatedVertices[i]);
			if (d < minDistSq) {
				minDistSq = d;
				closestVert = polygon.rotatedVertices[i];
			}
		}

		CEVec3 axis = circle.position - closestVert;
		float len = CEMath::Length(axis);

		// Only test this axis if we are outside (or barely inside) the vertex region.
		// If center is same as vertex, ignore (fallback to edge normals).
		if (len > 1e-5f) {
			axis = axis * (1.0f / len); // Normalize

			// Project Poly
			auto [minP, maxP] = projOBBOnAxis(polygon.rotatedVertices, axis);

			// Project Circle
			float cProj = CEMath::Dot(circle.position, axis);
			float minC = cProj - circle.radius;
			float maxC = cProj + circle.radius;

			float overlap = std::min(maxP, maxC) - std::max(minP, minC);

			if (overlap < 0) return { .collisionOccured = false };

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
			}
		}

		// Collision Found
		CollisionManifold result;
		result.collisionOccured = true;
		result.depth = minOverlap;
		result.normal = bestAxis;
		result.numOfContacts = 1;

		// Ensure normal points Circle -> Polygon (Standard: Normal points FROM A TO B)
		// Here A=Circle, B=Polygon.
		// If normal points towards Circle, flip it.
		// Vector from Circle to Poly Center:
		CEVec3 dir = polygon.position - circle.position;
		if (CEMath::Dot(result.normal, dir) < 0.f) {
			result.normal = -1.f * result.normal;
		}

		// Calculate Contact Point
		result.contactPoints[0] = circle.position + result.normal * circle.radius;

		if (!swapped) {
			// If not swapped, we want Normal to point A -> B (Circle -> Poly).
			// The logic above ensures it points Circle -> Poly.
			// But wait, the standard usually wants Normal pointing OUT of B towards A for resolution?
			// Let's check CircleToCircle: normal is A -> B (normalized).
			// Let's check AABBToAABB: normal points towards B?
			// "result.normal = posDelta.x < 0 ? -1 : 1". PosDelta is B - A. So normal is roughly A->B.

			// So result.normal IS correct (Circle -> Poly).
		}
		else {
			// Swapped: Input was Poly vs Circle. We computed Circle vs Poly.
			// We return normal relative to Poly (Poly -> Circle).
			result.normal = -1.f * result.normal;
		}

		return result;
	}

	CollisionManifold CollisionHandler::CircleToAABBCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* spherePtr{ &coll1 };
		CollisionProps const* aabbPtr{ &coll2 };

		bool swapped = false;

		if (spherePtr->collider->type > aabbPtr->collider->type) {
			std::swap(spherePtr, aabbPtr);
			swapped = true;
		}

		Circle const& sphere1Temp = dynamic_cast<Circle const&>(*(spherePtr->collider));
		AABBRect const& aabb2Temp = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));

		// Apply Offsets (Use the collider's effective center, not just entity position)
		CEVec3 spherePos = spherePtr->position + sphere1Temp.offset;
		CEVec3 aabbPos = aabbPtr->position + aabb2Temp.offset;

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

			// Circle Center is INSIDE the AABB
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
			// Circle Center is OUTSIDE (touching edge/corner)
			else {
				f32 distance{ CEMath::Length(ClosestPtToCirc) };

				result.depth = sphere1Temp.radius - distance;
				result.normal = -1.f * CEMath::Normalize(ClosestPtToCirc);
				result.contactPoints[0] = closestPt;
			}

			// Handle swapped normal
			if (!swapped) {
				result.normal = -1.f * result.normal;
			}
		}

		return result;
	}

	CollisionManifold CollisionHandler::CircleToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* circlePtr{ &coll1 };
		CollisionProps const* linePtr{ &coll2 };
		bool swapped = false;

		// Ensure strict ordering
		if (circlePtr->collider->type == ColliderType::LINE) {
			std::swap(circlePtr, linePtr);
			swapped = true;
		}

		Circle const& circle = dynamic_cast<Circle const&>(*(circlePtr->collider));
		LineSegment const& line = dynamic_cast<LineSegment const&>(*(linePtr->collider));

		// Find closest point on line segment to circle
		CEVec3 closest = ClosestPointOnSegment(line.GetWorldP1(), line.GetWorldP2(), circle.position);

		// Check distance
		CEVec3 vecToCirlce = circle.position - closest;
		f32 distSq = CEMath::LengthSq(vecToCirlce);

		if (distSq < circle.radius * circle.radius) {
			f32 dist = CEMath::Length(vecToCirlce);

			result.collisionOccured = true;
			result.depth = circle.radius - dist;
			result.contactPoints[0] = closest;
			result.numOfContacts = 1;

			// Handle normal
			if (dist > 0.0001f) {
				result.normal = -1.f * (vecToCirlce * (1.f / dist)); // Normalized
			}
			else {
				// If circle center is on line, use line normal
				result.normal = line.GetNormal();
			}

			if (swapped) result.normal = -1.f * result.normal;
		}

		return result;
	}

	CollisionManifold CollisionHandler::AABBToPolygonCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* aabbPtr{ &coll1 };
		CollisionProps const* polyPtr{ &coll2 };

		// Sort inputs so we know which is which (Enum Order: AABB < POLYGON)
		bool swapped = false;
		if (aabbPtr->collider->type > polyPtr->collider->type) {
			std::swap(aabbPtr, polyPtr);
			swapped = true;
		}

		AABBRect const& aabb = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));
		Polygon const& existingPoly = dynamic_cast<Polygon const&>(*(polyPtr->collider));

		// Cannot use aabb.min/max directly if they are local. We use Position +/- Extents
		CEVec3 aabbPos = aabbPtr->position + aabb.offset;
		CEVec3 minWorld = aabbPos - aabb.extents;
		CEVec3 maxWorld = aabbPos + aabb.extents;

		// Create the temporary polygon in World Space
		Polygon tempPoly = Polygon::CreateAABB(minWorld, maxWorld, CEVec3{ 0,0,0 });

		// Ensure the temp polygon has valid normals!
		if (tempPoly.normalEdges[0] == CEVec3{ 0,0,0 }) {
			tempPoly.normalEdges[0] = CEVec3{ 0.f, -1.f, 0.f }; // Bottom Edge Normal
			tempPoly.normalEdges[1] = CEVec3{ 1.f, 0.f, 0.f };  // Right Edge Normal
			tempPoly.normalEdges[2] = CEVec3{ 0.f, 1.f, 0.f };  // Top Edge Normal
			tempPoly.normalEdges[3] = CEVec3{ -1.f, 0.f, 0.f }; // Left Edge Normal
		}

		// SAT Checks
		Contact contactA{ getContactPoint(tempPoly, existingPoly) };
		if (!contactA) return result;

		Contact contactB{ getContactPoint(existingPoly, tempPoly) };
		if (!contactB) return result;

		CollisionManifold finalManifold;

		if (contactA->depth < contactB->depth) {
			finalManifold = *contactA;

			// SAT found smallest overlap on AABB's axis.
			// Normal currently pushes Polygon OUT of AABB.
			// Flip normal to push AABB away from Polygon (AABB -> Polygon collision)
			finalManifold.normal = -1.f * finalManifold.normal;
		}
		else {
			finalManifold = *contactB;
		}

		if (!swapped) {
			finalManifold.normal = -1.f * finalManifold.normal;
		}

		return finalManifold;
	}

	CollisionManifold CollisionHandler::PolygonToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		CollisionProps const* polyPtr{ &coll1 };
		CollisionProps const* linePtr{ &coll2 };

		// Sort inputs so we know which is which 
		bool swapped = false;
		if (polyPtr->collider->type == ColliderType::LINE) {
			std::swap(polyPtr, linePtr);
			swapped = true;
		}

		Polygon const& poly = dynamic_cast<Polygon const&>(*(polyPtr->collider));
		LineSegment const& line = dynamic_cast<LineSegment const&>(*(linePtr->collider));

		f32 minOverlap = std::numeric_limits<f32>::max();
		CEVec3 bestAxis{ 0.f, 0.f, 0.f };
		bool isLineAxis = false;

		// Test 1: Polygon axes
		for (int i = 0; i < poly.vertexCount; ++i) {
			CEVec3 axis = poly.normalEdges[i];

			// Project poly
			std::pair<f32, f32> proj = projOBBOnAxis(poly.rotatedVertices, axis);
			float minPoly = proj.first;
			float maxPoly = proj.second;

			// Project line
			f32 dotP1 = CEMath::Dot(line.GetWorldP1(), axis);
			f32 dotP2 = CEMath::Dot(line.GetWorldP2(), axis);
			f32 minLine = std::min(dotP1, dotP2);
			f32 maxLine = std::max(dotP1, dotP2);

			// Calculate MTV (Minimum Translation Vector) using Extents
			float rPoly = (maxPoly - minPoly) * 0.5f;
			float rLine = (maxLine - minLine) * 0.5f;
			float cPoly = minPoly + rPoly;
			float cLine = minLine + rLine;

			f32 overlap = (rPoly + rLine) - std::abs(cPoly - cLine);

			if (overlap < 0) return result;	// SAT found

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
				isLineAxis = false;
			}
		}

		// Test 2: Line Axis
		{
			CEVec3 axis = line.GetNormal();

			// Project Poly
			f32 minPoly = std::numeric_limits<f32>::max();
			f32 maxPoly = std::numeric_limits<f32>::min();
			for (int i = 0; i < poly.vertexCount; ++i) {
				f32 dot = CEMath::Dot(poly.rotatedVertices[i], axis);
				minPoly = std::min(minPoly, dot);
				maxPoly = std::max(maxPoly, dot);
			}

			// Project Line
			f32 lineProj = CEMath::Dot(line.GetWorldP1(), axis);
			// Line projection on its own normal is a single point (0 thickness)
			//f32 minLine = lineProj;
			//f32 maxLine = lineProj;

			// Calculate MTV
			float rPoly = (maxPoly - minPoly) * 0.5f;
			float rLine = 0.0f; // Line has 0 thickness on its normal
			float cPoly = minPoly + rPoly;
			float cLine = lineProj;

			f32 overlap = (rPoly + rLine) - std::abs(cPoly - cLine);

			if (overlap < 0) return result;

			// Bias towards Line Axis to prevent jitter (prefer sliding along line)
			float bias = 0.95f;
			if (overlap * bias < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
				isLineAxis = true;
			}
		}

		// Collision Confirmed
		result.collisionOccured = true;
		result.depth = minOverlap;
		result.normal = bestAxis;

		// 1. SAFE NORMAL FLIPPING
		// Use closest point on line for stability (fixes flip issues on long lines)
		CEVec3 closestOnLine = ClosestPointOnSegment(line.GetWorldP1(), line.GetWorldP2(), poly.position);
		CEVec3 dir = closestOnLine - poly.position;

		if (CEMath::Dot(result.normal, dir) < 0.f) {
			result.normal = -1.f * result.normal;
		}

		// 2. GENERATE CONTACT POINT (Critical for Torque stability)
		if (isLineAxis) {
			// Case A: Polygon Corner hitting Line Face
			// Find the polygon vertex furthest in the direction of the normal
			float maxProj = -std::numeric_limits<float>::max();
			CEVec3 bestVert = poly.rotatedVertices[0];

			for (int i = 0; i < poly.vertexCount; ++i) {
				float proj = CEMath::Dot(poly.rotatedVertices[i], result.normal);
				if (proj > maxProj) {
					maxProj = proj;
					bestVert = poly.rotatedVertices[i];
				}
			}
			result.contactPoints[0] = bestVert;
		}
		else {
			// Case B: Line Tip hitting Polygon Face
			// Pick the line endpoint that is deepest into the polygon (against the normal)
			float d1 = CEMath::Dot(line.GetWorldP1(), result.normal);
			float d2 = CEMath::Dot(line.GetWorldP2(), result.normal);

			if (d1 < d2) {
				result.contactPoints[0] = line.GetWorldP1();
			}
			else {
				result.contactPoints[0] = line.GetWorldP2();
			}
		}
		result.numOfContacts = 1;

		if (swapped) result.normal = -1.f * result.normal;

		return result;
	}

	CollisionManifold CollisionHandler::AABBToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };
		result.depth = std::numeric_limits<float>::max();

		// 1. Sort Inputs (AABB vs Line)
		CollisionProps const* aabbPtr = (coll1.collider->type == ColliderType::AABB) ? &coll1 : &coll2;
		CollisionProps const* linePtr = (coll1.collider->type == ColliderType::LINE) ? &coll1 : &coll2;

		// Detect if we swapped (so we can flip the normal later)
		bool swapped = (coll1.collider->type == ColliderType::LINE);

		// 2. Cast to specific types
		AABBRect const& aabb = dynamic_cast<AABBRect const&>(*(aabbPtr->collider));
		LineSegment const& line = dynamic_cast<LineSegment const&>(*(linePtr->collider));

		// 3. Get World Space Data
		// AABB Center & Extents
		CEVec3 aabbCenter = aabbPtr->position + aabb.offset;
		CEVec3 extents = aabb.extents;

		// Line Points (World Space)
		CEVec3 p1 = line.GetWorldP1();
		CEVec3 p2 = line.GetWorldP2();

		// 1. Define the 3 axes to test
		// Axis 1 & 2: AABB local axes (Global X and Y)
		// Axis 3: Segment normal
		CEVec3 segDir = p2 - p1;
		CEVec3 segNormal = line.GetNormal();

		CEVec3 axes[3] = {
			{1.0f, 0.0f, 0.f}, // AABB X
			{0.0f, 1.0f, 0.f}, // AABB Y
			segNormal     // Segment Normal
		};

		// 2. Loop through axes and find overlap
		for (int i = 0; i < 3; i++) {
			CEVec3 axis = axes[i];

			// Project both shapes
			float minA, maxA, minB, maxB;
			ProjectAABB(aabb, axis, minA, maxA);
			ProjectSegment(line, axis, minB, maxB);

			// Check for separation
			if (maxA < minB || maxB < minA) {
				return result; // No collision found, return empty
			}

			// Calculate overlap
			float axisDepth = std::min(maxA - minB, maxB - minA);

			// Keep the smallest overlap (this is the penetration depth)
			if (axisDepth < result.depth) {
				result.depth = axisDepth;
				result.normal = axis;
			}
		}

		// 3. Ensure Normal Direction
		// We want the normal to point from Box -> Segment.
		// We check the vector from Box Center to Segment Center (approx midpoint)
		CEVec3 segMid = (p1 + p2) * 0.5f;
		CEVec3 direction = segMid - aabbCenter;

		if (CEMath::Dot(direction, result.normal) < 0) {
			result.normal = result.normal * -1.0f;
		}

		result.collisionOccured = true;

		// 4. Calculate Contact Point
		// Case A: The collision axis is the Segment Normal. 
		// This means a corner of the AABB is hitting the flat side of the line.
		// We find the AABB corner closest to the segment.
		if (std::abs(CEMath::Dot(result.normal, segNormal)) > 0.99f) {
			// Find the support point on the box in the direction of the normal (towards segment)
			result.contactPoints[0] = GetAABBSupport(aabb, result.normal);
		}
		// Case B: The collision axis is one of the AABB axes (X or Y).
		// This means the tip of the line segment is hitting a flat face of the box.
		// We find which segment endpoint is inside the box.
		else {

			// Since normal points Box -> Segment, the point closer to the box has a *smaller* dot product along that normal?
			// Actually, simpler logic: The point overlapping the box is the contact.
			// We pick the point closest to the box center.
			if (CEMath::LengthSq(p1 - aabbCenter) < CEMath::LengthSq(p2 - aabbCenter)) {
				result.contactPoints[0] = p1;
			}
			else {
				result.contactPoints[0] = p2;
			}
		}

		// 7. Handle Swapping (If input was Line vs AABB, flip normal)
		if (swapped) {
			result.normal = -1.f * result.normal;
		}

		return result;
	}

	CollisionManifold CollisionHandler::LineToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		LineSegment const& line1 = dynamic_cast<LineSegment const&>(*(coll1.collider));
		LineSegment const& line2 = dynamic_cast<LineSegment const&>(*(coll2.collider));

		CEVec3 p1 = line1.GetWorldP1();
		CEVec3 p2 = line1.GetWorldP2();
		CEVec3 q1 = line2.GetWorldP1();
		CEVec3 q2 = line2.GetWorldP2();

		CEVec3 r = p2 - p1;
		CEVec3 s = q2 - q1;
		CEVec3 v = q1 - p1;

		// 2D Cross Product (z component)
		auto Cross2D = [](CEVec3 const& a, CEVec3 const& b) {
			return a.x * b.y - a.y * b.x;
			};

		float rXs = Cross2D(r, s);
		float qmp1Xr = Cross2D(v, r);

		const float EPSILON = 1e-6f;

		// If r x s = 0 and (q1-p1) x r = 0, then the two lines are collinear.
		if (std::abs(rXs) < EPSILON && std::abs(qmp1Xr) < EPSILON) {
			// Collinear case - check overlap
			// Project onto r axis
			float t0 = CEMath::Dot(v, r) / CEMath::Dot(r, r);
			float t1 = t0 + CEMath::Dot(s, r) / CEMath::Dot(r, r);

			// If interval [t0, t1] intersects [0, 1], they overlap
			if ((t0 >= 0 && t0 <= 1) || (t1 >= 0 && t1 <= 1) || (t0 < 0 && t1 > 1) || (t1 < 0 && t0 > 1)) {
				result.collisionOccured = true;
				result.normal = line2.GetNormal(); // Just pick one normal
				result.contactPoints[0] = p1; // Approximate
				return result;
			}
			return result;
		}

		if (std::abs(rXs) < EPSILON) {
			// Parallel but not collinear
			return result;
		}

		float t = Cross2D(v, s) / rXs;
		float u = Cross2D(v, r) / rXs;

		if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
			result.collisionOccured = true;
			result.contactPoints[0] = p1 + r * t;

			// Normal handling: For raycasts, we usually want the normal of the surface we hit (line2).
			result.normal = line2.GetNormal();

			// Ensure normal faces against ray direction (r)
			if (CEMath::Dot(result.normal, r) > 0) {
				result.normal = result.normal * -1.0f;
			}

			// Calculate depth (distance) - for rays this is usually distance from start
			result.depth = 0.0f;
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
		int circle = static_cast<int>(ColliderType::CIRCLE);
		int aabb = static_cast<int>(ColliderType::AABB);
		int obb = static_cast<int>(ColliderType::OBB); // Map OBB
		int line = static_cast<int>(ColliderType::LINE);
		int poly = static_cast<int>(ColliderType::POLYGON);

		quadTree.Resize(AABBRect{ CEVec3{-2000.f, -2000.f, 0.f}, CEVec3{2000.f, 2000.f, 0.f} });

		/*Fill in the table of collisions, e.g collideFuncts[0] will be ColliderType::CIRCLE TO ColliderType::CIRCLE collision (2 total collider types will have 2^2 permutations(different types of collisions possible) ) */

		// Circle vs Circle
		collideFuncts[circle * SHAPE_COUNT + circle] = CircleToCircleCollision;

		collideFuncts[circle * SHAPE_COUNT + line] = CircleToLineCollision;
		collideFuncts[line * SHAPE_COUNT + circle] = CircleToLineCollision;

		// AABB vs AABB
		collideFuncts[aabb * SHAPE_COUNT + aabb] = AABBToAABBCollision;

		collideFuncts[aabb * SHAPE_COUNT + line] = AABBToLineCollision;
		collideFuncts[line * SHAPE_COUNT + aabb] = AABBToLineCollision;

		// Circle vs AABB (Circle is 0, AABB is 1. 0 < 1, so index is 0 * N + 1)
		collideFuncts[circle * SHAPE_COUNT + aabb] = CircleToAABBCollision;

		// AABB vs Polygon (AABB is 1, Poly is 4. 1 < 4)
		collideFuncts[aabb * SHAPE_COUNT + poly] = AABBToPolygonCollision;
		collideFuncts[aabb * SHAPE_COUNT + obb] = AABBToPolygonCollision; // AABB vs OBB uses Poly logic

		// Circle vs Polygon (Circle is 0, Poly is 4. 0 < 4)
		collideFuncts[circle * SHAPE_COUNT + poly] = CircleToPolygonCollision;
		collideFuncts[circle * SHAPE_COUNT + obb] = CircleToPolygonCollision; // Circle vs OBB uses Poly logic

		collideFuncts[line * SHAPE_COUNT + poly] = PolygonToLineCollision;
		collideFuncts[line * SHAPE_COUNT + obb] = PolygonToLineCollision; // Line vs OBB uses Poly logic

		// Line vs Line
		collideFuncts[line * SHAPE_COUNT + line] = LineToLineCollision;

		// Polygon vs Polygon (and OBB vs OBB, and Mixed)
		collideFuncts[poly * SHAPE_COUNT + poly] = PolygonToPolygonCollision;
		collideFuncts[obb * SHAPE_COUNT + obb] = PolygonToPolygonCollision;
		collideFuncts[poly * SHAPE_COUNT + obb] = PolygonToPolygonCollision;
		collideFuncts[obb * SHAPE_COUNT + poly] = PolygonToPolygonCollision;

	}

	void CollisionHandler::Update([[maybe_unused]] double deltaTime, EngineState state) {
		m_FrameCollisions.clear();

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		// 1. Get ALL entities with Colliders (not just RigidBodies)
		std::vector<CrashoutsEngine::Entity> colliderEntities =
			globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity;

		if (state == EngineState::STOP) {
			lastFrameCollision.clear();
			currentFrameCollision.clear();
		}

		// Force all colliders to match their Transform position BEFORE we calculate collisions.
		// ONLY for entities without RigidBodyComponent (Physics system handles the rest)
		for (CrashoutsEngine::Entity& entity : colliderEntities) {
			if (!IsEntityActive(globalECS, entity)) continue;

			if (!globalECS.HasComponent<Component::RigidBodyComponent>(entity)) {
				if (globalECS.HasComponent<Component::TransformComponent>(entity)) {
					auto* transform = globalECS.GetComponent<Component::TransformComponent>(entity);
					auto* collider = globalECS.GetComponent<Component::ColliderComponent>(entity);

					// Sync Position & Rotation
					if (collider && collider->collider) {
						collider->collider->UpdateCollider(transform->position);
						collider->collider->UpdateRotation(transform->rotation);
					}
				}
			}
		}

		/*BROADPHASE*/
		// 2. Insert into QuadTree
		quadTree.Clear();
		for (CrashoutsEngine::Entity& entity1 : colliderEntities) { // Changed from globalEntities
			if (!IsEntityActive(globalECS, entity1)) continue;

			if (!globalECS.HasComponent<Component::TransformComponent>(entity1)) continue;
			auto* collider = globalECS.GetComponent<Component::ColliderComponent>(entity1);
			if (!collider || !collider->collider) continue;
			if (!collider->collider->enabled) continue;
			quadTree.Insert(entity1);
		}

		static std::vector<CrashoutsEngine::Entity> collidableObjects{};

		/*DETECT COLLISION AND UPDATE COLLISION EVENTS MEMBER*/
		for (CrashoutsEngine::Entity& entity1 : colliderEntities) {
			if (!IsEntityActive(globalECS, entity1)) continue;

			if (!globalECS.HasComponent<Component::TransformComponent>(entity1)) continue;
			auto* collider = globalECS.GetComponent<Component::ColliderComponent>(entity1);
			if (!collider || !collider->collider) continue;
			if (!collider->collider->enabled) continue;

			collidableObjects.clear();

			Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity1) };
			Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(entity1) };

			AABBRect region{ GetAABB(*(collider1->collider), transform1->position) };
			quadTree.Query(region, collidableObjects);

			for (CrashoutsEngine::Entity const& entity2 : collidableObjects) {
				if (!IsEntityActive(globalECS, entity2)) continue;

				if (entity1 == entity2) continue;

				// Prevents double checks, only process if ID1 < ID2
				// Ensures pair (A,B) is processed, and skip (B,A)
				if (entity1 > entity2) continue;

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
						Collision::CollisionProps{ collider1->collider, rb1->GetVelocity(), transform1->position },
						Collision::CollisionProps{ collider2->collider, rb2->GetVelocity(), transform2->position }
						);

				}

				if (collisionManifold.collisionOccured) {
					// Populate master list
					m_FrameCollisions.push_back(
						CollisionEvent{ collisionManifold, entity1, entity2 }
					);

					// Gameplay messaging only when in play mode

					if (state == EngineState::PLAY) {
						// If EITHER collider is a trigger, it is a Trigger Event.
						// Otherwise, it is a Collision Event.
						bool isTriggerInteraction = collider1->collider->isTrigger || collider2->collider->isTrigger;

						using namespace Messaging;
						EventType enterType = isTriggerInteraction ? EventType::TriggerEnter : EventType::CollisionEnter;
						EventType stayType = isTriggerInteraction ? EventType::TriggerStay : EventType::CollisionStay;

						// Check if this pair was already processed this frame (e.g. duplicate query)
						bool isIn{ false };
						for (ContactInformation const& test : currentFrameCollision) {
							if (test.contacts.first == entity1 && test.contacts.second == entity2) isIn = true;
						}

						if (!isIn) {
							ContactPair pair{ entity1, entity2 };

							// Check if this pair existed Last Frame
							if (isPairInCollection(pair, lastFrameCollision)) {
								// STAY
								CollisionMessage stayMessage{ stayType, collisionManifold, entity1, entity2 };
								sendMessage(stayMessage);
							}
							else {
								// ENTER
								CollisionMessage enterMessage{ enterType, collisionManifold, entity1, entity2 };
								sendMessage(enterMessage);
							}

							// Save contact
							currentFrameCollision.push_back(std::move(ContactInformation{ pair, collisionManifold }));
						}
					}
				}
			}
		}
		if (state == EngineState::PLAY) {
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

		if (state == EngineState::STOP) {
			// Refresh collision data for the next play
			currentFrameCollision.clear();
			lastFrameCollision.clear();
		}

	}

	FunctionDispatchType CollisionHandler::callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2) {

		/*Swap the type according to enum order, e.g function exists for circle to aabb but not aabb to circle*/
		if (type1 > type2) {
			std::swap(type1, type2);
		}

		int idx1{ static_cast<int>(type1) };
		int idx2{ static_cast<int>(type2) };
		//std::cout << " size of collidefuncts is " << collideFuncts.size() << std::endl;
		return collideFuncts[idx1 * SHAPE_COUNT + idx2];
	}

	bool CollisionHandler::Raycast(CEVec3 start, CEVec3 end, CrashoutsEngine::Entity& outEntity, CrashoutsEngine::Entity ignoredEntity) {
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		// Create a Line Segment Collider use local points as start/end, and position at 0,0,0
		LineSegment rayLine(start, end);
		rayLine.UpdateCollider(CEVec3{ 0.f, 0.f, 0.f }); // Initialize worldP1/P2

		// Query QuadTree to calculate AABB of the line for QuadTree query rahhhhhhhh
		CEVec3 min, max;
		// Use World Points
		CEVec3 pts[2] = { rayLine.GetWorldP1(), rayLine.GetWorldP2() };
		min.x = std::min(pts[0].x, pts[1].x);
		min.y = std::min(pts[0].y, pts[1].y);
		min.z = 0.0f;
		max.x = std::max(pts[0].x, pts[1].x);
		max.y = std::max(pts[0].y, pts[1].y);
		max.z = 0.0f;

		// Expand AABB slightly to ensure we catch border cases
		CEVec3 padding{ 5.0f, 5.0f, 0.0f }; // Increase padding slightly
		min = min - padding;
		max = max + padding;

		AABBRect region(min, max);
		static std::vector<CrashoutsEngine::Entity> candidates;
		candidates.clear();
		quadTree.Query(region, candidates);

		// Check Collisions
		float closestDistSq = std::numeric_limits<float>::max();
		bool hitFound = false;

		std::unique_ptr<Collider> rayCollider = std::make_unique<LineSegment>(rayLine);

		CollisionProps rayProps{ rayCollider, CEVec3{0,0,0}, CEVec3{0,0,0} };

		for (CrashoutsEngine::Entity const& entity : candidates) {
			if (!IsEntityActive(globalECS, entity)) continue;
			if (entity == ignoredEntity) continue;

			// Do not raycast against Triggers
			auto* colliderComp = globalECS.GetComponent<Component::ColliderComponent>(entity);
			if (!colliderComp || !colliderComp->collider) continue;
			if (colliderComp->collider->isTrigger) continue;
			if (!colliderComp->collider->enabled) continue;

			auto* transformComp = globalECS.GetComponent<Component::TransformComponent>(entity);
			if (!transformComp) continue;

			CEVec3 vel{ 0,0,0 };
			if (globalECS.HasComponent<Component::RigidBodyComponent>(entity)) {
				vel = globalECS.GetComponent<Component::RigidBodyComponent>(entity)->GetVelocity();
			}

			CollisionProps targetProps{ colliderComp->collider, vel, transformComp->position };

			auto colFunc = callCollFunct(ColliderType::LINE, colliderComp->collider->type);
			if (!colFunc) continue;

			CollisionManifold manifold = colFunc(rayProps, targetProps);

			if (manifold.collisionOccured) {
				// Find distance to start point
				float distSq = CEMath::LengthSq(manifold.contactPoints[0] - start);

				if (distSq < closestDistSq) {
					closestDistSq = distSq;
					outEntity = entity;
					hitFound = true;
				}
			}
		}

		return hitFound;
	}
}