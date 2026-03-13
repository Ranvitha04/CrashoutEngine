/*!
@file       debugcollision.cpp
@author     Renfred Lee
@author     Tristan Mira
@author     tristan
@date       28/09/2025
@brief      Implements collision debug visualization for Crashout Engine.
@details    Renders wireframe overlays for all active colliders (AABBs & spheres),
			highlights entities participating in collisions, and draws contact
			points and normal vectors derived from the collision manifold.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "engine.h"
#include "debugcollision.h"
#include <algorithm>
#include <cmath>
#include "camera.h"
#include "collidercomponent.h"
#include "rigidbodycomponent.h"
#include "activecomponent.h"
#include "CEMath.h"
#include "shapes.h"
#include <limits>
#include "rendering.h"

using namespace CrashoutsEngine;
using namespace Component;

namespace CollisionGizmos {
	std::vector<DebugLineCmd> g_DebugLineQueue{};
}

static RenderCamera::Camera& GetActiveDebugCamera() {
	return (engineState == EngineState::STOP) ? RenderCamera::engineCamera : RenderCamera::gameCamera;
}

/* internal helpers
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
*/
static void DrawAABBOutline(const CEVec3& minP, const CEVec3& maxP, float thickness, const CEColor& color) {
	float z = GetActiveDebugCamera().GetFar(); // Restored World Space Depth!
	CEVec3 p0(minP.x, minP.y, z);
	CEVec3 p1(maxP.x, minP.y, z);
	CEVec3 p2(maxP.x, maxP.y, z);
	CEVec3 p3(minP.x, maxP.y, z);

	CollisionGizmos::g_DebugLineQueue.push_back({ p0, p1, thickness, color });
	CollisionGizmos::g_DebugLineQueue.push_back({ p1, p2, thickness, color });
	CollisionGizmos::g_DebugLineQueue.push_back({ p2, p3, thickness, color });
	CollisionGizmos::g_DebugLineQueue.push_back({ p3, p0, thickness, color });
}

/*!
 @brief Draws a 2D oriented bounding box as a wireframe rectangle.
*/
static void DrawOBBOutline(const Collision::OBBRect* vertices, float thickness, const CEColor& color) {
	float z = GetActiveDebugCamera().GetFar(); // Restored World Space Depth!
	CEVec3 v0 = { vertices->rotatedVertices[0].x, vertices->rotatedVertices[0].y, z };
	CEVec3 v1 = { vertices->rotatedVertices[1].x, vertices->rotatedVertices[1].y, z };
	CEVec3 v2 = { vertices->rotatedVertices[2].x, vertices->rotatedVertices[2].y, z };
	CEVec3 v3 = { vertices->rotatedVertices[3].x, vertices->rotatedVertices[3].y, z };

	CollisionGizmos::g_DebugLineQueue.push_back({ v0, v1, thickness, color });
	CollisionGizmos::g_DebugLineQueue.push_back({ v1, v2, thickness, color });
	CollisionGizmos::g_DebugLineQueue.push_back({ v2, v3, thickness, color });
	CollisionGizmos::g_DebugLineQueue.push_back({ v3, v0, thickness, color });

}

/*!
 @brief Draws a Polygon.
*/
static void DrawPolygon(const Collision::Polygon* vertices, float thickness, const CEColor& color) {
	float z = GetActiveDebugCamera().GetFar(); // Restored World Space Depth!

	for (int i = 0; i < vertices->vertexCount; ++i) {
		int nextIdx = (i + 1) % vertices->vertexCount;
		CollisionGizmos::g_DebugLineQueue.push_back({
			CEVec3{ vertices->rotatedVertices[i].x, vertices->rotatedVertices[i].y, z },
			CEVec3{ vertices->rotatedVertices[nextIdx].x, vertices->rotatedVertices[nextIdx].y, z },
			thickness,
			color
			});
	}
}

/*!
 @brief Draws a 2D circle wireframe.
*/
static void DrawCircleOutline(const CEVec3& c, float r, int segments, float thickness, const CEColor& color) {
	if (segments > 24) segments = 24;
	if (segments < 8) segments = 8;

	const float twoPi = 6.28318530718f;
	const float step = twoPi / segments;

	float z = GetActiveDebugCamera().GetFar(); // Restored World Space Depth!

	// THE BUG WAS HERE: This used to be 0.0f! Now it correctly uses 'z'
	CEVec3 prev(c.x + r, c.y, z);

	for (int i = 1; i <= segments; ++i) {
		float t = i * step;
		CEVec3 next(c.x + r * std::cos(t), c.y + r * std::sin(t), z);
		CollisionGizmos::g_DebugLineQueue.push_back({ prev, next, thickness, color });
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
	void DrawAllColliders(ECS& ecs, const std::vector<Collision::CollisionEvent>& events, float wireThickness) {
		RenderCamera::Camera& cam = GetActiveDebugCamera();
		Component::Rendering::Get().shdr_pgm->Use();
		Component::Rendering::Get().shdr_pgm->SetUniform("uView", cam.GetViewMatrix());
		Component::Rendering::Get().shdr_pgm->SetUniform("uProjection", cam.GetProjectionMatrix());

		auto& collArr = ecs.GetComponentArray<Component::ColliderComponent>();
		const CEColor ok(0.15f, 0.8f, 0.55f);    // non-colliding tint
		const CEColor hit(1.0f, 0.3f, 0.3f);     // colliding tint

		// Culling preperations

		CEVec3 camPos = cam.GetPosition();
		float camHeight = cam.GetVisibleWorldHeight();
		float camWidth = cam.GetVisibleWorldWidth();

		// Create a bounding box for the camera viewport
		float camMinX = camPos.x - (camWidth * 0.5f);
		float camMaxX = camPos.x + (camWidth * 0.5f);
		float camMinY = camPos.y - (camHeight * 0.5f);
		float camMaxY = camPos.y + (camHeight * 0.5f);

		// We use a HashSet for O(1) lookups later.
		std::unordered_set<Entity> collidingEntities;

		// Reserve space to avoid reallocations
		collidingEntities.reserve(events.size() * 2);

		for (const auto& ev : events) {
			collidingEntities.insert(ev.entity1);
			collidingEntities.insert(ev.entity2);
		}

		for (auto e : collArr.indexToEntity) {
			if (!IsEntityActive(ecs, e)) continue;
			Component::ColliderComponent const* pColliderComp = ecs.GetComponent<Component::ColliderComponent>(e);

			if (!pColliderComp || !pColliderComp->collider) continue;

			// OPTIMIZATION: O(1) Lookup
			bool isHit = collidingEntities.find(e) != collidingEntities.end();
			const CEColor color = isHit ? hit : ok;

			// Skip drawing inactive/disabled colliders if desired
			if (!pColliderComp->collider->enabled) continue;

			// Frustum Culling Check

			switch (pColliderComp->collider->type) {
			case Collision::ColliderType::CIRCLE: {
				auto& circle = static_cast<Collision::Circle const&>(*pColliderComp->collider);
				// Simple Circle vs AABB (Camera) check
				float closestX = std::fmaxf(camMinX, std::fminf(circle.position.x, camMaxX));
				float closestY = std::fmaxf(camMinY, std::fminf(circle.position.y, camMaxY));
				float distanceSq = std::powf(circle.position.x - closestX, 2) + std::powf(circle.position.y - closestY, 2);

				if (distanceSq < (circle.radius * circle.radius)) {
					// Reduced segments from 48 to 24 for performance
					DrawCircleOutline(circle.position, circle.radius, 24, wireThickness, color);
				}
				break;
			}
			case Collision::ColliderType::AABB: {
				auto& aabb = static_cast<Collision::AABBRect const&>(*pColliderComp->collider);
				// Standard AABB vs AABB check
				if (aabb.max.x > camMinX && aabb.min.x < camMaxX &&
					aabb.max.y > camMinY && aabb.min.y < camMaxY) {
					DrawAABBOutline(aabb.min, aabb.max, wireThickness, color);
				}
				break;
			}
			case Collision::ColliderType::OBB: {
				// For OBB and Polygon, use their pre-calculated AABB 
				auto& poly = static_cast<Collision::Polygon const&>(*pColliderComp->collider);

				// Manual mini-AABB check for the polygon vertices
				float pMinX = 1e10f, pMaxX = -1e10f, pMinY = 1e10f, pMaxY = -1e10f;
				for (int i = 0; i < poly.vertexCount; ++i) {
					pMinX = std::min(pMinX, poly.rotatedVertices[i].x);
					pMaxX = std::max(pMaxX, poly.rotatedVertices[i].x);
					pMinY = std::min(pMinY, poly.rotatedVertices[i].y);
					pMaxY = std::max(pMaxY, poly.rotatedVertices[i].y);
				}

				if (pMaxX > camMinX && pMinX < camMaxX && pMaxY > camMinY && pMinY < camMaxY) {
					if (pColliderComp->collider->type == Collision::ColliderType::OBB) {
						auto& obbTemp = static_cast<Collision::OBBRect const&>(*pColliderComp->collider);
						DrawOBBOutline(&obbTemp, wireThickness, color);
					}
					else
						DrawPolygon(&poly, wireThickness, color);
				}
				break;
			} case Collision::ColliderType::LINE: {
				auto& line = static_cast<Collision::LineSegment const&>(*pColliderComp->collider);
				g_DebugLineQueue.push_back({ line.GetWorldP1(), line.GetWorldP2(), 1.f, color });
				g_DebugLineQueue.push_back({ line.GetWorldP1(), line.GetWorldP1() + line.GetNormal(), 1.f, color });
				break;
			}
			case Collision::ColliderType::POLYGON: {
				// For OBB and Polygon, use their pre-calculated AABB 
				auto& poly = static_cast<Collision::Polygon const&>(*pColliderComp->collider);

				// Manual mini-AABB check for the polygon vertices
				float pMinX = 1e10f, pMaxX = -1e10f, pMinY = 1e10f, pMaxY = -1e10f;
				for (int i = 0; i < poly.vertexCount; ++i) {
					pMinX = std::min(pMinX, poly.rotatedVertices[i].x);
					pMaxX = std::max(pMaxX, poly.rotatedVertices[i].x);
					pMinY = std::min(pMinY, poly.rotatedVertices[i].y);
					pMaxY = std::max(pMaxY, poly.rotatedVertices[i].y);
				}

				if (pMaxX > camMinX && pMinX < camMaxX && pMaxY > camMinY && pMinY < camMaxY) {
					if (pColliderComp->collider->type == Collision::ColliderType::OBB) {
						auto& obbTemp = static_cast<Collision::OBBRect const&>(*pColliderComp->collider);
						DrawOBBOutline(&obbTemp, wireThickness, color);
					}
					else
						DrawPolygon(&poly, wireThickness, color);
				}
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
	void DrawContactsAndNormals(ECS& ecs, const std::vector<Collision::CollisionEvent>& events, float baseLen, float wireThickness) {

		RenderCamera::Camera& cam = GetActiveDebugCamera();
		Component::Rendering::Get().shdr_pgm->Use();
		Component::Rendering::Get().shdr_pgm->SetUniform("uView", cam.GetViewMatrix());
		Component::Rendering::Get().shdr_pgm->SetUniform("uProjection", cam.GetProjectionMatrix());

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

			// 3. Draw
			// Draw contact point
			DrawCircleOutline(contactPt, wireThickness * 3.f, 8, wireThickness, CEColor{ 1.f, 0.f, 0.f }); // slightly larger circle

			// Draw Normal Line scaled by velocity
			g_DebugLineQueue.push_back({ contactPt, contactPt + (normal * baseLen), wireThickness });
		}
	}

} // namespace CollisionGizmos