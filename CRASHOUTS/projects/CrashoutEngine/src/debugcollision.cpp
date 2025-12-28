/*!
@file       debugcollision.cpp
@author     mohamadamirakmal.b@digipen.edu
@date       28/09/2025
@brief      Implements collision debug visualization for Crashout Engine.
@details    Renders wireframe overlays for all active colliders (AABBs & spheres),
			highlights entities participating in collisions, and draws contact
			points and normal vectors derived from the collision manifold.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "debugcollision.h"
#include <algorithm>
#include <cmath>
#include "camera.h"
#include "collidercomponent.h"
#include "rigidbodycomponent.h"
#include "CEMath.h"
#include "shapes.h"

using namespace CrashoutsEngine;
using namespace Component;

/*                                                       internal helpers
----------------------------------------------------------------------------- */
/*!
 @brief Checks if an entity appears in any of the provided collision events.
 @param e            Entity to query.
 @param events       Optional pointer to a collision event list. If null, returns false.
 @return             True if the entity is part of at least one event; false otherwise.
*/
[[maybe_unused]] static bool IsColliding(Entity e, const Collision::ContactCollection& events) {

	if (!events.size()) return false;
	for (Collision::ContactInformation const& ev : events) {
		if (ev.contacts.first == e || ev.contacts.second == e) return true;
	}
	return false;
}

/*!
 @brief Draws a 2D axis-aligned bounding box as a wireframe rectangle.
 @param minP         Minimum point (bottom-left).
 @param maxP         Maximum point (top-right).
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawAABBOutline(const CEVec3& minP, const CEVec3& maxP, float thickness, const CEColor& color) {
	float z = RenderCamera::engineCamera.GetFar();
	CEVec3 p0(minP.x, minP.y, z);
	CEVec3 p1(maxP.x, minP.y, z);
	CEVec3 p2(maxP.x, maxP.y, z);
	CEVec3 p3(minP.x, maxP.y, z);

	CEVec2 size = { thickness, thickness };

	Shapes::CreateLine(p0, p1, thickness, color);
	Shapes::CreateLine(p1, p2, thickness, color);
	Shapes::CreateLine(p2, p3, thickness, color);
	Shapes::CreateLine(p3, p0, thickness, color);
}

/*!
 @brief Draws a 2D oriented bounding box as a wireframe rectangle.
 @param vertices     A vertex array
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawOBBOutline(const Collision::OBBRect* vertices, float thickness, const CEColor& color) {
	float z = RenderCamera::engineCamera.GetFar();
	// Create new vertices with the modified Z value
	CEVec3 v0 = { vertices->rotatedVertices[0].x, vertices->rotatedVertices[0].y, z };
	CEVec3 v1 = { vertices->rotatedVertices[1].x, vertices->rotatedVertices[1].y, z };
	CEVec3 v2 = { vertices->rotatedVertices[2].x, vertices->rotatedVertices[2].y, z };
	CEVec3 v3 = { vertices->rotatedVertices[3].x, vertices->rotatedVertices[3].y, z };

	// Draw the lines using the new, z-modified vertices
	Shapes::CreateLine(v0, v1, thickness, color);
	Shapes::CreateLine(v1, v2, thickness, color);
	Shapes::CreateLine(v2, v3, thickness, color);
	Shapes::CreateLine(v3, v0, thickness, color);
}

/*!
 @brief Draws a Polygon.
 @param vertices     A vertex array
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawPolygon(const Collision::Polygon* vertices, float thickness, const CEColor& color) {
	float z = RenderCamera::engineCamera.GetFar();

	for (int i{}; i < vertices->vertexCount; ++i) {
		int nextIdx{ (i + 1) % vertices->vertexCount };
		// Draw the lines using the new, z-modified vertices
		//Shapes::CreateLine({ vertices->rotatedVertices[i].x, vertices->rotatedVertices[i].y, z }, CEVec3{ vertices->rotatedVertices[i].x + vertices->normalEdges[i].x * 10.f, vertices->rotatedVertices[i].y + vertices->normalEdges[i].y * 10.f, z}, thickness, color);
		Shapes::CreateLine(CEVec3{ vertices->rotatedVertices[i].x, vertices->rotatedVertices[i].y, z }, CEVec3{ vertices->rotatedVertices[nextIdx].x, vertices->rotatedVertices[nextIdx].y, z }, thickness, color);
	}
}

/*!
 @brief Draws a 2D circle wireframe (used for sphere colliders projected to XY).
 @param c            Center position.
 @param r            Radius.
 @param segments     Number of line segments (clamped to minimum of 8).
 @param thickness    Line thickness.
 @param color        RGB-like color vector used by the line drawer.
*/
static void DrawCircleOutline(const CEVec3& c, float r, int segments, float thickness, const CEColor& color) {
	if (segments > 24) segments = 24;
	if (segments < 8) segments = 8;

	const float twoPi = 6.28318530718f;
	const float step = twoPi / segments;

	CEVec3 prev(c.x + r, c.y, 0.0f);
	float z = RenderCamera::engineCamera.GetFar();

	for (int i = 1; i <= segments; ++i) {
		float t = i * step;
		CEVec3 next(c.x + r * std::cos(t), c.y + r * std::sin(t), z);
		Shapes::CreateLine(prev, next, thickness, color);
		prev = next;
	}
}

/*                                                public visualization apis
----------------------------------------------------------------------------- */
namespace CollisionGizmos {

	/*!
	 @brief Draws wireframes for all colliders in the ECS; tints colliding ones.
	 @param ecs            Reference to ECS.
	 @param events         Optional collision events to determine tinting.
	 @param wireThickness  Line thickness for the wire overlay.
	 @note When @p events is provided, colliders for entities present in any event
		   are tinted with a hit color to visually distinguish collisions.
	*/
	void DrawAllColliders(ECS& ecs, [[maybe_unused]] const std::vector<Collision::CollisionEvent>& events, float wireThickness) {
		auto& collArr = ecs.GetComponentArray<Component::ColliderComponent>();
		const CEColor ok(0.15f, 0.8f, 0.55f);    // non-colliding tint
		const CEColor hit(1.0f, 0.3f, 0.3f);     // colliding tint

		Collision::CollisionHandler& colHandler{ Collision::CollisionHandler::GetInstance() };

		// We use a HashSet for O(1) lookups later.
		std::unordered_set<Entity> collidingEntities;

		// Reserve space to avoid reallocations
		collidingEntities.reserve(colHandler.lastFrameCollision.size() * 2);

		for (const auto& ev : colHandler.lastFrameCollision) {
			collidingEntities.insert(ev.contacts.first);
			collidingEntities.insert(ev.contacts.second);
		}

		for (auto e : collArr.indexToEntity) {
			Component::ColliderComponent const* pColliderComp = ecs.GetComponent<Component::ColliderComponent>(e);

			if (!pColliderComp || !pColliderComp->collider) continue;

			// OPTIMIZATION: O(1) Lookup
			bool isHit = collidingEntities.find(e) != collidingEntities.end();
			const CEColor color = isHit ? hit : ok;

			// Skip drawing inactive/disabled colliders if desired
			if (!pColliderComp->collider->enabled) continue;

			switch (pColliderComp->collider->type) {
			case Collision::ColliderType::SPHERE: {
				auto& sphereTemp = static_cast<Collision::Sphere const&>(*pColliderComp->collider);
				// Reduced segments from 48 to 24 for performance
				DrawCircleOutline(sphereTemp.position, sphereTemp.radius, 24, wireThickness, color);
				break;
			}
			case Collision::ColliderType::AABB: {
				auto& aabbTemp = static_cast<Collision::AABBRect const&>(*pColliderComp->collider);
				DrawAABBOutline(aabbTemp.min, aabbTemp.max, wireThickness, color);
				break;
			}
			case Collision::ColliderType::OBB: {
				auto& obbTemp = static_cast<Collision::OBBRect const&>(*pColliderComp->collider);
				DrawOBBOutline(&obbTemp, wireThickness, color);
				break;
			}
			case Collision::ColliderType::POLYGON: {
				auto& polygonTemp = static_cast<Collision::Polygon const&>(*pColliderComp->collider);
				DrawPolygon(&polygonTemp, wireThickness, color);
				break;
			}
			}
		}
	}

	/*!
	 @brief Visualizes contact points as crosses and draws the manifold normal.
	 @param events         List of collision events containing manifolds.
	 @param normalLen      Length of the normal vector drawn from the first contact.
	 @param wireThickness  Line thickness for points and normal.
	 @note For each event, all contact points are marked with a small cross.
		   If any contact exists, the manifold�s normal is drawn starting at
		   the first contact point with length @p normalLen.
	*/
	void DrawContactsAndNormals(ECS& ecs, [[maybe_unused]] const std::vector<Collision::CollisionEvent>& events, float baseLen, float wireThickness) {
		for (const auto& info : events) {
			if (info.colManifold.contactPoints.empty()) continue;

			const CEVec3& contactPt = info.colManifold.contactPoints[0];
			CEVec3 normal = info.colManifold.normal;

			// 1. Calculate Relative Velocity
			CEVec3 velA{ 0,0,0 };
			CEVec3 velB{ 0,0,0 };

			// Check and retrieve velocity for Entity A
			if (ecs.HasComponent<Component::RigidBodyComponent>(info.entity1)) {
				velA = ecs.GetComponent<Component::RigidBodyComponent>(info.entity1)->GetVelocity();
			}

			// Check and retrieve velocity for Entity B
			if (ecs.HasComponent<Component::RigidBodyComponent>(info.entity2)) {
				velB = ecs.GetComponent<Component::RigidBodyComponent>(info.entity2)->GetVelocity();
			}

			CEVec3 relativeVel = velA - velB;

			// Project relative velocity onto the normal to see how fast they are "approaching"
			float impactSpeed = std::abs(CEMath::Dot(relativeVel, normal));

			// 2. Determine Length
			// Base length + extra length based on speed. 
			// e.g., 5 units fixed + 0.5 units per 1 m/s of speed.
			float lineLength = baseLen + (impactSpeed * 0.5f);

			// Clamp it so it doesn't go to infinity or zero
			lineLength = std::clamp(lineLength, baseLen, baseLen * 5.0f);

			// 3. Draw
			// Draw contact point
			DrawCircleOutline(contactPt, wireThickness * 3.f, 8, wireThickness, CEColor{ 1.f, 0.f, 0.f }); // slightly larger circle

			// Draw Normal Line scaled by velocity
			Shapes::CreateLine(contactPt, contactPt + (normal * lineLength), wireThickness);
		}
	}

} // namespace CollisionGizmos