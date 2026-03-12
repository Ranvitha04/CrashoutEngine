/*!
@file       Collision.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Function definitions for Basic Collision Functions and Colliders(Plane, ColliderType::AABB and Circle)

All content ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¯ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¿ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â½ 2025 Digipen Institute of Technology Singapore. All rights reserved.


//__________________________________________________________________________*/

#include "collisionmanager.h"
#include "rigidbodycomponent.h"
#include "activecomponent.h"
#include "glapp.h"
#include "collidercomponent.h"
#include "hierarchycomponent.h"
#include "CEMath.h"
#include "CEMtx33.h"
#include "hierarchy.h"
#include <cmath>
#include <algorithm>
#include "camera.h"
#include "colliders.h"
#include "jobsystem.h"
#include "performanceSystem.h"
#include "frameratecontroller.h"
#include <unordered_map>
#include <unordered_set>

/*UTILS RIGHT HERE*/
namespace Collision {
	namespace {
		struct ContactPairHash {
			size_t operator()(ContactPair const& pair) const noexcept {
				size_t h1 = std::hash<CrashoutsEngine::Entity>{}(pair.first);
				size_t h2 = std::hash<CrashoutsEngine::Entity>{}(pair.second);
				return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
			}
		};

		struct EntityCollisionCache {
			Component::ColliderComponent* collider{ nullptr };
			Component::TransformComponent* transform{ nullptr };
			Component::RigidBodyComponent* rigidBody{ nullptr };
			bool isDynamic{ false };
		};
	}

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

	bool CollisionHandler::RaycastAgainstList(CEVec3 start, CEVec3 end, const std::vector<CrashoutsEngine::Entity>& candidates, CEVec3& outPoint,
		CrashoutsEngine::Entity& outEntity, CrashoutsEngine::Entity ignoredEntity, CrashoutsEngine::Entity ignoredParent) {

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		thread_local std::unique_ptr<Collider> tl_rayCollider = std::make_unique<LineSegment>(CEVec3{ 0.f,0.f,0.f }, CEVec3{ 0.f,0.f,0.f });

		LineSegment* rayLine = static_cast<LineSegment*>(tl_rayCollider.get());
		rayLine->p1 = start;
		rayLine->p2 = end;
		rayLine->UpdateCollider(CEVec3{ 0.f, 0.f, 0.f });

		float closestDistSq = std::numeric_limits<float>::max();
		bool hitFound = false;

		const CEVec3 kZeroVelocity{ 0.f, 0.f, 0.f };
		CollisionProps rayProps{ tl_rayCollider, kZeroVelocity, kZeroVelocity };

		// Pre-calculate the ray's direction for the fast AABB check
		CEVec3 dir = end - start;

		for (CrashoutsEngine::Entity const& entity : candidates) {
			if (!IsEntityActive(globalECS, entity)) continue;
			if (entity == ignoredEntity) continue;
			if (ignoredParent != 0) {
				if (entity == ignoredParent) continue;
				if (globalECS.HasComponent<Component::ParentComponent>(entity)) {
					auto* parentComp = globalECS.GetComponent<Component::ParentComponent>(entity);
					if (parentComp && parentComp->parent == ignoredParent) continue;
				}
			}

			auto* colliderComp = globalECS.GetComponent<Component::ColliderComponent>(entity);
			if (!colliderComp || !colliderComp->collider || colliderComp->collider->isTrigger || !colliderComp->collider->enabled) continue;

			auto* transformComp = globalECS.GetComponent<Component::TransformComponent>(entity);
			if (!transformComp) continue;

			// ==============================================================
			// OPTIMIZATION: PER-RAY AABB BROADPHASE
			// If the ray completely misses the object's general bounding box,
			// we skip the expensive polygon math entirely!
			// ==============================================================
			AABBRect entityAABB = GetAABB(*colliderComp->collider, transformComp->position);

			float tmin = 0.0f;
			float tmax = 1.0f;

			if (std::abs(dir.x) > 1e-6f) {
				float inv = 1.0f / dir.x;
				float t1 = (entityAABB.min.x - start.x) * inv;
				float t2 = (entityAABB.max.x - start.x) * inv;
				if (t1 > t2) std::swap(t1, t2);
				tmin = std::max(tmin, t1);
				tmax = std::min(tmax, t2);
				if (tmin > tmax) continue; // MISS! Ray completely missed the box on X axis.
			}
			else if (start.x < entityAABB.min.x || start.x > entityAABB.max.x) {
				continue; // MISS!
			}

			if (std::abs(dir.y) > 1e-6f) {
				float inv = 1.0f / dir.y;
				float t1 = (entityAABB.min.y - start.y) * inv;
				float t2 = (entityAABB.max.y - start.y) * inv;
				if (t1 > t2) std::swap(t1, t2);
				tmin = std::max(tmin, t1);
				tmax = std::min(tmax, t2);
				if (tmin > tmax) continue; // MISS! Ray completely missed the box on Y axis.
			}
			else if (start.y < entityAABB.min.y || start.y > entityAABB.max.y) {
				continue; // MISS!
			}

			// If the object is an AABB, we just accidentally calculated the exact hit point!
			// No need to do any more math, just register the hit.
			if (colliderComp->collider->type == ColliderType::AABB) {
				float distSq = CEMath::LengthSq(dir) * (tmin * tmin);
				if (distSq < closestDistSq) {
					closestDistSq = distSq;
					outEntity = entity;
					outPoint = start + dir * tmin;
					hitFound = true;
				}
				continue;
			}

			// ==============================================================
			// NARROWPHASE (Only happens if the ray passed the AABB check!)
			// ==============================================================
			CEVec3 vel = kZeroVelocity;
			if (globalECS.HasComponent<Component::RigidBodyComponent>(entity)) {
				vel = globalECS.GetComponent<Component::RigidBodyComponent>(entity)->GetVelocity();
			}

			CollisionProps targetProps{ colliderComp->collider, vel, transformComp->position };

			auto colFunc = callCollFunct(ColliderType::LINE, colliderComp->collider->type);
			if (!colFunc) continue;

			CollisionManifold manifold = colFunc(rayProps, targetProps);

			if (manifold.collisionOccured) {
				float distSq = CEMath::LengthSq(manifold.contactPoints[0] - start);
				if (distSq < closestDistSq) {
					closestDistSq = distSq;
					outEntity = entity;
					outPoint = manifold.contactPoints[0];
					hitFound = true;
				}
			}
		}
		return hitFound;
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

		Circle const& sphere1Temp = static_cast<Circle const&>(*(coll1.collider));

		Circle const& sphere2Temp = static_cast<Circle const&>(*(coll2.collider));

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

		AABBRect const& aabb1Temp = static_cast<AABBRect const&>(*(coll1.collider));
		AABBRect const& aabb2Temp = static_cast<AABBRect const&>(*(coll2.collider));

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

		Polygon const& polygon1 = static_cast<Polygon const&>(*(polygonPtr1->collider));

		Polygon const& polygon2 = static_cast<Polygon const&>(*(polygonPtr2->collider));

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

		//// Calculate vector from A to B
		//CEVec3 centerVec = coll2.position - coll1.position;

		//// If the normal points in the opposite direction of the center vector, flip it.
		//if (CEMath::Dot(finalManifold.normal, centerVec) < 0.0f) {
		//	finalManifold.normal = -1.f * finalManifold.normal;
		//}

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

		Circle const& circle = static_cast<Circle const&>(*(spherePtr->collider));
		Polygon const& polygon = static_cast<Polygon const&>(*(polygonPtr->collider));

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

		// SAFE NORMAL DIRECTION
		// SAT found the axis pointing OUT of the Polygon.
		// We want the normal pointing FROM Circle (A) TO Polygon (B). 
		// Therefore, we just strictly invert the SAT axis. No center guessing needed!
		result.normal = -1.f * bestAxis;

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

		Circle const& sphere1Temp = static_cast<Circle const&>(*(spherePtr->collider));
		AABBRect const& aabb2Temp = static_cast<AABBRect const&>(*(aabbPtr->collider));

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

		Circle const& circle = static_cast<Circle const&>(*(circlePtr->collider));
		LineSegment const& line = static_cast<LineSegment const&>(*(linePtr->collider));

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

		AABBRect const& aabb = static_cast<AABBRect const&>(*(aabbPtr->collider));
		Polygon const& existingPoly = static_cast<Polygon const&>(*(polyPtr->collider));

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

		Polygon const& poly = static_cast<Polygon const&>(*(polyPtr->collider));
		LineSegment const& line = static_cast<LineSegment const&>(*(linePtr->collider));

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

		// 1. Sort Inputs (Ensure A is AABB, B is Line)
		bool swapped = (coll1.collider->type == ColliderType::LINE);
		CollisionProps const* aabbPtr = swapped ? &coll2 : &coll1;
		CollisionProps const* linePtr = swapped ? &coll1 : &coll2;

		AABBRect const& aabb = static_cast<AABBRect const&>(*(aabbPtr->collider));
		LineSegment const& line = static_cast<LineSegment const&>(*(linePtr->collider));

		// 2. Extract Data
		CEVec3 aabbCenter = aabbPtr->position + aabb.offset;
		CEVec3 p1 = line.GetWorldP1();
		CEVec3 p2 = line.GetWorldP2();

		// 3. Define the 3 SAT Axes
		CEVec3 axes[3] = {
			{1.0f, 0.0f, 0.0f}, // AABB X
			{0.0f, 1.0f, 0.0f}, // AABB Y
			line.GetNormal()    // Line Normal
		};

		float minOverlap = std::numeric_limits<float>::max();
		float minScore = std::numeric_limits<float>::max();
		CEVec3 bestAxis = { 0.0f, 0.0f, 0.0f };

		// 4. Test each axis
		for (int i = 0; i < 3; ++i) {
			float minA, maxA, minB, maxB;
			ProjectAABB(aabb, axes[i], minA, maxA);
			ProjectSegment(line, axes[i], minB, maxB);

			// If there is a gap on ANY axis, there is absolutely no collision.
			if (maxA < minB || maxB < minA) {
				return result;
			}

			float overlap = std::min(maxA - minB, maxB - minA);

			// Give the line's normal a slight priority to prevent horizontal snagging on floor edges.
			// This does not change the actual physical depth calculated.
			float score = overlap * ((i == 2) ? 0.95f : 1.0f);

			if (score < minScore) {
				minScore = score;
				minOverlap = overlap; // Keep the true physical depth
				bestAxis = axes[i];
			}
		}

		// 5. Collision Confirmed
		result.collisionOccured = true;
		result.depth = minOverlap;
		result.numOfContacts = 1;

		// 6. Ensure the normal always points FROM the AABB TO the Line
		// This is crucial for your specific position correction solver: `pos -= vec`
		CEVec3 closestLinePt = ClosestPointOnSegment(p1, p2, aabbCenter);
		CEVec3 aabbToLine = closestLinePt - aabbCenter;

		// If the axis points away from the line, flip it to point towards the line.
		if (CEMath::Dot(bestAxis, aabbToLine) < 0.0f) {
			bestAxis = -1.0f * bestAxis;
		}

		result.normal = bestAxis;
		result.contactPoints[0] = closestLinePt;

		// 7. Handle component swapping
		if (swapped) {
			result.normal = -1.0f * result.normal;
		}

		return result;
	}

	CollisionManifold CollisionHandler::LineToLineCollision(CollisionProps const& coll1, CollisionProps const& coll2) {
		CollisionManifold result{ .collisionOccured = false };

		LineSegment const& line1 = static_cast<LineSegment const&>(*(coll1.collider));
		LineSegment const& line2 = static_cast<LineSegment const&>(*(coll2.collider));

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
	struct ThreadResult {
		std::vector<CollisionEvent> events;
		std::vector<ContactInformation> contacts;
		struct Msg {
			Messaging::EventType type;
			CollisionManifold manifold;
			CrashoutsEngine::Entity e1, e2;
		};
		std::vector<Msg> messages;
	};

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

/*----------------COLLISIONHANDLER CLASS DEFINITIONS--------------------------------------------------------*/

	CollisionHandler::CollisionHandler() : Messaging::Observable{}, collideFuncts{}
	{
		int circle = static_cast<int>(ColliderType::CIRCLE);
		int aabb = static_cast<int>(ColliderType::AABB);
		int obb = static_cast<int>(ColliderType::OBB); // Map OBB
		int line = static_cast<int>(ColliderType::LINE);
		int poly = static_cast<int>(ColliderType::POLYGON);

		// Initialize BOTH trees
		staticQuadTree.Resize(AABBRect{ CEVec3{-2000.f, -2000.f, 0.f}, CEVec3{2000.f, 2000.f, 0.f} });
		dynamicQuadTree.Resize(AABBRect{ CEVec3{-2000.f, -2000.f, 0.f}, CEVec3{2000.f, 2000.f, 0.f} });
		isStaticDirty = true;
		lastStaticCount = 0;

		/*Fill in the table of collisions*/
		collideFuncts[circle * SHAPE_COUNT + circle] = CircleToCircleCollision;

		collideFuncts[circle * SHAPE_COUNT + line] = CircleToLineCollision;
		collideFuncts[line * SHAPE_COUNT + circle] = CircleToLineCollision;

		collideFuncts[aabb * SHAPE_COUNT + aabb] = AABBToAABBCollision;

		collideFuncts[aabb * SHAPE_COUNT + line] = AABBToLineCollision;
		collideFuncts[line * SHAPE_COUNT + aabb] = AABBToLineCollision;

		collideFuncts[circle * SHAPE_COUNT + aabb] = CircleToAABBCollision;

		collideFuncts[aabb * SHAPE_COUNT + poly] = AABBToPolygonCollision;
		collideFuncts[aabb * SHAPE_COUNT + obb] = AABBToPolygonCollision;

		collideFuncts[circle * SHAPE_COUNT + poly] = CircleToPolygonCollision;
		collideFuncts[circle * SHAPE_COUNT + obb] = CircleToPolygonCollision;

		collideFuncts[line * SHAPE_COUNT + poly] = PolygonToLineCollision;
		collideFuncts[line * SHAPE_COUNT + obb] = PolygonToLineCollision;

		collideFuncts[line * SHAPE_COUNT + line] = LineToLineCollision;

		collideFuncts[poly * SHAPE_COUNT + poly] = PolygonToPolygonCollision;
		collideFuncts[obb * SHAPE_COUNT + obb] = PolygonToPolygonCollision;
		collideFuncts[poly * SHAPE_COUNT + obb] = PolygonToPolygonCollision;
		collideFuncts[obb * SHAPE_COUNT + poly] = PolygonToPolygonCollision;
	}

	// SIGNATURE UPDATED
	void CollisionHandler::Update([[maybe_unused]] double deltaTime, EngineState state, int subStepIndex) {

		FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "CollisionDetection");
		m_FrameCollisions.clear();

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		std::vector<CrashoutsEngine::Entity> colliderEntities =
			globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity;

		static std::vector<CrashoutsEngine::Entity> dynamicEntities;
		static std::vector<CrashoutsEngine::Entity> staticEntities;

		static std::unordered_set<ContactPair, ContactPairHash> lastFramePairs;
		static std::unordered_set<ContactPair, ContactPairHash> currentFramePairs;

		if (subStepIndex == 0) {
			if (state == EngineState::STOP) {
				lastFrameCollision.clear();
			}
			currentFrameCollision.clear();
		}

		lastFramePairs.clear();
		for (ContactInformation const& info : lastFrameCollision) {
			lastFramePairs.insert(info.contacts);
		}

		currentFramePairs.clear();
		for (ContactInformation const& info : currentFrameCollision) {
			currentFramePairs.insert(info.contacts);
		}

		// --- SPLIT STATIC AND DYNAMIC LISTS ---
		// We only evaluate static objects on Substep 0. Dynamic objects must be grabbed every substep.
		if (subStepIndex == 0 || state == EngineState::STOP) {
			dynamicEntities.clear();
			staticEntities.clear();

			for (CrashoutsEngine::Entity& entity : colliderEntities) {
				if (!IsEntityActive(globalECS, entity)) continue;

				auto* collider = globalECS.GetComponent<Component::ColliderComponent>(entity);
				if (!collider || !collider->collider || !collider->collider->enabled) continue;

				auto* transform = globalECS.GetComponent<Component::TransformComponent>(entity);
				if (!transform) continue;

				auto* rb = globalECS.GetComponent<Component::RigidBodyComponent>(entity);
				bool isDynamic = rb && rb->getMass() != 0.0f;

				if (isDynamic) {
					dynamicEntities.push_back(entity);
				}
				else {
					staticEntities.push_back(entity);
					// Update static transforms only once per frame
					collider->collider->UpdateCollider(transform->position);
					collider->collider->UpdateRotation(transform->rotation);
				}
			}
		}
		else {
			// Fast-path for sub-steps: just grab the dynamic ones!
			dynamicEntities.clear();
			for (CrashoutsEngine::Entity& entity : colliderEntities) {
				if (!IsEntityActive(globalECS, entity)) continue;
				auto* rb = globalECS.GetComponent<Component::RigidBodyComponent>(entity);
				if (rb && rb->getMass() != 0.0f) {
					auto* collider = globalECS.GetComponent<Component::ColliderComponent>(entity);
					if (collider && collider->collider && collider->collider->enabled) {
						dynamicEntities.push_back(entity);
					}
				}
			}
		}

		/*BROADPHASE DUAL QUADTREE*/
		// 1. Rebuild STATIC tree safely ONCE per frame (Substep 0)
		if (subStepIndex == 0 || state == EngineState::STOP) {
			staticQuadTree.Clear();
			for (CrashoutsEngine::Entity& staticEnt : staticEntities) {
				staticQuadTree.Insert(staticEnt);
			}
		}

		// 2. Rebuild DYNAMIC tree EVERY sub-step
		dynamicQuadTree.Clear();
		for (CrashoutsEngine::Entity& dynamicEnt : dynamicEntities) {
			dynamicQuadTree.Insert(dynamicEnt);
		}

		/*DETECT COLLISION*/
		unsigned int numThreads = CrashoutsEngine::JobSystem::Get().GetThreadCount();
		if (numThreads == 0) numThreads = 1;
		std::vector<ThreadResult> threadResults(numThreads);
		std::mutex threadResultsMutex;

		static std::vector<ThreadResult> threadResults;
		if (threadResults.size() < numThreads) threadResults.resize(numThreads);

		for (auto& res : threadResults) {
			res.events.clear();
			res.contacts.clear();
			res.messages.clear();
		}

		CrashoutsEngine::JobSystem::Get().ParallelFor((unsigned int)dynamicEntities.size(), [&](unsigned int start, unsigned int end) {
			ThreadResult localResult;

			std::vector<CrashoutsEngine::Entity> localCollidableObjects;
			localCollidableObjects.reserve(128);

			for (unsigned int i = start; i < end; ++i) {
				CrashoutsEngine::Entity entity1 = dynamicEntities[i];

				Component::ColliderComponent* collider1 = globalECS.GetComponent<Component::ColliderComponent>(entity1);
				Component::TransformComponent* transform1 = globalECS.GetComponent<Component::TransformComponent>(entity1);
				Component::RigidBodyComponent* rb1 = globalECS.GetComponent<Component::RigidBodyComponent>(entity1);

				if (subStepIndex > 0 && !collider1->collider->isContinuous) {
					continue;
				}

				// --- QUERY BOTH TREES WITH VELOCITY EXPANSION ---
				localCollidableObjects.clear();
				AABBRect region = GetAABB(*(collider1->collider), transform1->position);

				// Stretch the query box so fast objects don't skip over thin walls!
				if (rb1) {
					int totalSubSteps = FrameRateController::GetInstance().GetMaxPhysicsSteps();
					float dtToUse = collider1->collider->isContinuous ? (float)(deltaTime / totalSubSteps) : (float)deltaTime;
					CEVec3 velDelta = rb1->GetVelocity() * dtToUse;

					if (velDelta.x > 0.0f) region.max.x += velDelta.x;
					else region.min.x += velDelta.x;

					if (velDelta.y > 0.0f) region.max.y += velDelta.y;
					else region.min.y += velDelta.y;
				}

				staticQuadTree.Query(region, localCollidableObjects);
				dynamicQuadTree.Query(region, localCollidableObjects);

				// NARROW PHASE COLLISION CHECK
				for (CrashoutsEngine::Entity const& entity2 : localCollidableObjects) {
					if (entity1 == entity2) continue;

					Component::ColliderComponent* collider2 = globalECS.GetComponent<Component::ColliderComponent>(entity2);
					if (!collider2 || !collider2->collider || !collider2->collider->enabled) continue;

					Component::TransformComponent* transform2 = globalECS.GetComponent<Component::TransformComponent>(entity2);
					if (!transform2) continue;

					Component::RigidBodyComponent* rb2 = globalECS.GetComponent<Component::RigidBodyComponent>(entity2);
					bool isDynamic2 = rb2 && rb2->getMass() != 0.0f;

					if (CrashoutsEngine::IsAlreadyChildOf(globalECS, entity1, entity2) ||
						CrashoutsEngine::IsAlreadyChildOf(globalECS, entity2, entity1)) continue;

					if (isDynamic2) {
						bool entity2Initiates = (subStepIndex == 0 || collider2->collider->isContinuous);
						if (entity2Initiates && entity1 > entity2) {
							continue;
						}
					}

					FunctionDispatchType dispatch = callCollFunct(collider1->collider->type, collider2->collider->type);
					if (!dispatch) continue;

					Collision::CollisionManifold collisionManifold;

					if (rb1 && rb2 && rb1->getMass() == 0.f && rb2->getMass() == 0.f) continue;

					const CEVec3 kZeroVelocity{ 0.f, 0.f, 0.f };
					CEVec3 const& vel1 = rb1 ? rb1->GetVelocity() : kZeroVelocity;
					CEVec3 const& vel2 = rb2 ? rb2->GetVelocity() : kZeroVelocity;

					collisionManifold = dispatch(
						Collision::CollisionProps{ collider1->collider, vel1, transform1->position },
						Collision::CollisionProps{ collider2->collider, vel2, transform2->position }
					);

					if (collisionManifold.collisionOccured) {
						// Populate master list
						localResult.events.push_back(CollisionEvent{ collisionManifold, entity1, entity2 });

						if (state == EngineState::PLAY) {
							bool isTriggerInteraction = collider1->collider->isTrigger || collider2->collider->isTrigger;

							using namespace Messaging;
							EventType enterType = isTriggerInteraction ? EventType::TriggerEnter : EventType::CollisionEnter;
							EventType stayType = isTriggerInteraction ? EventType::TriggerStay : EventType::CollisionStay;

							ContactPair pair{ entity1, entity2 };

							if (lastFramePairs.find(pair) != lastFramePairs.end()) {
								localResult.messages.push_back({ stayType, collisionManifold, entity1, entity2 });
							}
							else {
								localResult.messages.push_back({ enterType, collisionManifold, entity1, entity2 });
							}

							localResult.contacts.push_back({ pair, collisionManifold });
						}
					}
				}
			}

			if (!localResult.events.empty() || !localResult.messages.empty() || !localResult.contacts.empty()) {
				std::lock_guard<std::mutex> lock(threadResultsMutex);

				auto bestIt = std::min_element(threadResults.begin(), threadResults.end(),
					[](const ThreadResult& lhs, const ThreadResult& rhs) {
						return lhs.events.size() < rhs.events.size();
					});

				if (bestIt != threadResults.end()) {
					bestIt->events.insert(bestIt->events.end(), localResult.events.begin(), localResult.events.end());
					bestIt->messages.insert(bestIt->messages.end(), localResult.messages.begin(), localResult.messages.end());
					bestIt->contacts.insert(bestIt->contacts.end(), localResult.contacts.begin(), localResult.contacts.end());
				}
			}
			});

		for (auto& res : threadResults) {
			m_FrameCollisions.insert(m_FrameCollisions.end(), res.events.begin(), res.events.end());

			if (state == EngineState::PLAY) {
				for (size_t i = 0; i < res.contacts.size(); ++i) {
					ContactPair pair = res.contacts[i].contacts;

					if (currentFramePairs.insert(pair).second) {
						currentFrameCollision.push_back(res.contacts[i]);

						auto& msg = res.messages[i];
						CollisionMessage cmsg{ msg.type, msg.manifold, msg.e1, msg.e2 };
						sendMessage(cmsg);
					}
				}
			}
		}

		int totalSubSteps = FrameRateController::GetInstance().GetMaxPhysicsSteps();

		if (state == EngineState::PLAY) {
			// DEFER EXITS
			if (subStepIndex == totalSubSteps - 1) {
				for (ContactInformation const& contactInfo : lastFrameCollision) {
					if (currentFramePairs.find(contactInfo.contacts) == currentFramePairs.end()) {

						using namespace Messaging;
						EventType exitType = EventType::CollisionExit;

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
				std::swap(currentFrameCollision, lastFrameCollision);
			}
		}

		if (state == EngineState::STOP && subStepIndex == 0) {
			currentFrameCollision.clear();
			lastFrameCollision.clear();
		}

		if (subStepIndex == totalSubSteps - 1) {
			m_FrameCollisions.clear();
			for (auto& info : currentFrameCollision) {
				m_FrameCollisions.push_back(CollisionEvent{ info.colManifold, info.contacts.first, info.contacts.second });
			}
		}
	}

	FunctionDispatchType CollisionHandler::callCollFunct(Collision::ColliderType type1, Collision::ColliderType type2) {
		if (type1 > type2) {
			std::swap(type1, type2);
		}
		int idx1{ static_cast<int>(type1) };
		int idx2{ static_cast<int>(type2) };
		return collideFuncts[idx1 * SHAPE_COUNT + idx2];
	}

	bool CollisionHandler::Raycast(CEVec3 start, CEVec3 end, CrashoutsEngine::Entity& outEntity, CrashoutsEngine::Entity ignoredEntity) {
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		LineSegment rayLine(start, end);
		rayLine.UpdateCollider(CEVec3{ 0.f, 0.f, 0.f });

		CEVec3 min, max;
		CEVec3 pts[2] = { rayLine.GetWorldP1(), rayLine.GetWorldP2() };
		min.x = std::min(pts[0].x, pts[1].x);
		min.y = std::min(pts[0].y, pts[1].y);
		min.z = 0.0f;
		max.x = std::max(pts[0].x, pts[1].x);
		max.y = std::max(pts[0].y, pts[1].y);
		max.z = 0.0f;

		CEVec3 padding{ 5.0f, 5.0f, 0.0f };
		min = min - padding;
		max = max + padding;

		AABBRect region(min, max);
		std::vector<CrashoutsEngine::Entity> candidates;
		candidates.reserve(128);

		// --- QUERY BOTH TREES ---
		QueryTrees(region, candidates);

		float closestDistSq = std::numeric_limits<float>::max();
		bool hitFound = false;

		std::unique_ptr<Collider> rayCollider = std::make_unique<LineSegment>(rayLine);

		CollisionProps rayProps{ rayCollider, CEVec3{0,0,0}, CEVec3{0,0,0} };

		for (CrashoutsEngine::Entity const& entity : candidates) {
			if (!IsEntityActive(globalECS, entity)) continue;
			if (entity == ignoredEntity) continue;

			auto* colliderComp = globalECS.GetComponent<Component::ColliderComponent>(entity);
			if (!colliderComp || !colliderComp->collider) continue;
			if (colliderComp->collider->isTrigger) continue;
			if (!colliderComp->collider->enabled) continue;

			auto* transformComp = globalECS.GetComponent<Component::TransformComponent>(entity);
			if (!transformComp) continue;

			switch (colliderComp->collider->type) {
			case ColliderType::LINE:
			case ColliderType::AABB:
			case ColliderType::CIRCLE:
				colliderComp->collider->UpdateCollider(transformComp->position);
				break;
			case ColliderType::OBB:
			case ColliderType::POLYGON:
			case ColliderType::TRIANGLE:
				colliderComp->collider->UpdateCollider(transformComp->position);
				colliderComp->collider->UpdateRotation(transformComp->rotation);
				break;
			default:
				break;
			}

			CEVec3 vel{ 0,0,0 };
			if (globalECS.HasComponent<Component::RigidBodyComponent>(entity)) {
				vel = globalECS.GetComponent<Component::RigidBodyComponent>(entity)->GetVelocity();
			}

			CollisionProps targetProps{ colliderComp->collider, vel, transformComp->position };

			auto colFunc = callCollFunct(ColliderType::LINE, colliderComp->collider->type);
			if (!colFunc) continue;

			CollisionManifold manifold = colFunc(rayProps, targetProps);

			if (manifold.collisionOccured) {
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

	bool CollisionHandler::RaycastHit(CEVec3 start, CEVec3 end, CEVec3& outPoint, CrashoutsEngine::Entity& outEntity,
		CrashoutsEngine::Entity ignoredEntity, CrashoutsEngine::Entity ignoredParent) {
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		LineSegment rayLine(start, end);
		rayLine.UpdateCollider(CEVec3{ 0.f, 0.f, 0.f });

		CEVec3 min, max;
		CEVec3 pts[2] = { rayLine.GetWorldP1(), rayLine.GetWorldP2() };
		min.x = std::min(pts[0].x, pts[1].x);
		min.y = std::min(pts[0].y, pts[1].y);
		min.z = 0.0f;
		max.x = std::max(pts[0].x, pts[1].x);
		max.y = std::max(pts[0].y, pts[1].y);
		max.z = 0.0f;

		CEVec3 padding{ 5.0f, 5.0f, 0.0f };
		min = min - padding;
		max = max + padding;

		AABBRect region(min, max);
		std::vector<CrashoutsEngine::Entity> candidates;
		candidates.reserve(128);

		// --- QUERY BOTH TREES ---
		QueryTrees(region, candidates);

		float closestDistSq = std::numeric_limits<float>::max();
		bool hitFound = false;

		std::unique_ptr<Collider> rayCollider = std::make_unique<LineSegment>(rayLine);
		CollisionProps rayProps{ rayCollider, CEVec3{0,0,0}, CEVec3{0,0,0} };

		for (CrashoutsEngine::Entity const& entity : candidates) {
			if (!IsEntityActive(globalECS, entity)) continue;
			if (entity == ignoredEntity) continue;
			if (ignoredParent != 0) {
				if (entity == ignoredParent) continue;
				if (globalECS.HasComponent<Component::ParentComponent>(entity)) {
					auto* parentComp = globalECS.GetComponent<Component::ParentComponent>(entity);
					if (parentComp && parentComp->parent == ignoredParent) continue;
				}
			}

			auto* colliderComp = globalECS.GetComponent<Component::ColliderComponent>(entity);
			if (!colliderComp || !colliderComp->collider) continue;
			if (colliderComp->collider->isTrigger) continue;
			if (!colliderComp->collider->enabled) continue;

			auto* transformComp = globalECS.GetComponent<Component::TransformComponent>(entity);
			if (!transformComp) continue;

			switch (colliderComp->collider->type) {
			case ColliderType::LINE:
			case ColliderType::AABB:
			case ColliderType::CIRCLE:
				colliderComp->collider->UpdateCollider(transformComp->position);
				break;
			case ColliderType::OBB:
			case ColliderType::POLYGON:
			case ColliderType::TRIANGLE:
				colliderComp->collider->UpdateCollider(transformComp->position);
				colliderComp->collider->UpdateRotation(transformComp->rotation);
				break;
			default:
				break;
			}

			CEVec3 vel{ 0,0,0 };
			if (globalECS.HasComponent<Component::RigidBodyComponent>(entity)) {
				vel = globalECS.GetComponent<Component::RigidBodyComponent>(entity)->GetVelocity();
			}

			CollisionProps targetProps{ colliderComp->collider, vel, transformComp->position };

			if (colliderComp->collider->type == ColliderType::AABB) {
				auto& aabb = static_cast<AABBRect&>(*colliderComp->collider);
				CEVec3 dir = end - start;

				float tmin = 0.0f;
				float tmax = 1.0f;

				if (std::abs(dir.x) > 1e-6f) {
					float inv = 1.0f / dir.x;
					float t1 = (aabb.min.x - start.x) * inv;
					float t2 = (aabb.max.x - start.x) * inv;
					if (t1 > t2) std::swap(t1, t2);
					tmin = std::max(tmin, t1);
					tmax = std::min(tmax, t2);
					if (tmin > tmax) continue;
				}
				else if (start.x < aabb.min.x || start.x > aabb.max.x) {
					continue;
				}

				if (std::abs(dir.y) > 1e-6f) {
					float inv = 1.0f / dir.y;
					float t1 = (aabb.min.y - start.y) * inv;
					float t2 = (aabb.max.y - start.y) * inv;
					if (t1 > t2) std::swap(t1, t2);
					tmin = std::max(tmin, t1);
					tmax = std::min(tmax, t2);
					if (tmin > tmax) continue;
				}
				else if (start.y < aabb.min.y || start.y > aabb.max.y) {
					continue;
				}

				float distSq = CEMath::LengthSq(dir) * (tmin * tmin);
				if (distSq < closestDistSq) {
					closestDistSq = distSq;
					outEntity = entity;
					outPoint = start + dir * tmin;
					hitFound = true;
				}
				continue;
			}

			auto colFunc = callCollFunct(ColliderType::LINE, colliderComp->collider->type);
			if (!colFunc) continue;

			CollisionManifold manifold = colFunc(rayProps, targetProps);

			if (manifold.collisionOccured) {
				float distSq = CEMath::LengthSq(manifold.contactPoints[0] - start);
				if (distSq < closestDistSq) {
					closestDistSq = distSq;
					outEntity = entity;
					outPoint = manifold.contactPoints[0];
					hitFound = true;
				}
			}
		}

		return hitFound;
	}
}