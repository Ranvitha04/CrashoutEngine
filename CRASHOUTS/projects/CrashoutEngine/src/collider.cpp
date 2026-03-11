/*!
@file       collider.cpp
@author     miratristan.d@digipen.edu
@date       29/11/2025`
@brief      collider class definitions and implementations

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved

//__________________________________________________________________________*/
#include "colliders.h"
#include "CEMath.h"
#include "CEMtx33.h"

namespace Collision {
	Collider::Collider(Collision::ColliderType type) : type{ type } { /*EMPTY BY DESIGN*/ }

	Circle::Circle(f32 radius, CEVec3 const& center, CEVec3 const& newOffset) : Collider{ ColliderType::CIRCLE }, radius{ radius } { position = center; offset = newOffset; ColliderType = ColliderType::CIRCLE; }

	void Circle::UpdateCollider(CEVec3 const& newPos) {
		position = newPos + offset;
		isAABBCacheDirty = true;
	}

	AABBRect::AABBRect(CEVec3 const& min, CEVec3 const& max) : Collider{ ColliderType::AABB }, min{ min }, max{ max } {
		extents = (max - min) * 0.5f;
		offset = min + extents;
		ColliderType = ColliderType::AABB;
	}

	AABBRect::AABBRect() : Collider{ ColliderType::AABB } { ColliderType = ColliderType::AABB;  }

	AABBRect GetAABB(Collider const& collider, CEVec3 const& transform) {

	if (!collider.isAABBCacheDirty) return AABBRect{ collider.cachedMin, collider.cachedMax };

	switch (collider.type) {

		case ColliderType::CIRCLE: {
			Circle const& circle{ static_cast<Circle const&>(collider) };
			CEVec3 extents{ circle.radius, circle.radius, 0.f };
			collider.cachedMin = circle.position - extents;
			collider.cachedMax = circle.position + extents;
			}
			break;
		case ColliderType::AABB: {
			AABBRect const& aabb = static_cast<AABBRect const&>(collider);
			collider.cachedMin = aabb.min;
			collider.cachedMax = aabb.max;
			}
			break;
		case ColliderType::LINE: {
			LineSegment const& line{ static_cast<LineSegment const&>(collider) };
			CEVec3 min{ line.GetWorldP1() };
			CEVec3 max{ line.GetWorldP2() };

			if (min.x > max.x) std::swap(min.x, max.x);
			if (min.y > max.y) std::swap(min.y, max.y);

			collider.cachedMin = min;
			collider.cachedMax = max;
		}				   
			break;
		case ColliderType::POLYGON: {
			Polygon const& polygon{ static_cast<Polygon const&>(collider) };
			CEVec3 minXY{ polygon.rotatedVertices[0].x, polygon.rotatedVertices[0].y, transform.z };
			CEVec3 maxXY{ polygon.rotatedVertices[0].x, polygon.rotatedVertices[0].y, transform.z };

			for (int i{}; i < polygon.vertexCount; ++i) {
				minXY.x = std::min(minXY.x, polygon.rotatedVertices[i].x);
				minXY.y = std::min(minXY.y, polygon.rotatedVertices[i].y);

				maxXY.x = std::max(maxXY.x, polygon.rotatedVertices[i].x);
				maxXY.y = std::max(maxXY.y, polygon.rotatedVertices[i].y);
			}

			collider.cachedMin = minXY;
			collider.cachedMax = maxXY;
			}
			break;
		default:
			collider.cachedMin = CEVec3{};
			collider.cachedMax = CEVec3{};
			break;
	}

	collider.isAABBCacheDirty = false;
	return AABBRect{ collider.cachedMin, collider.cachedMax };
}

	void AABBRect::UpdateCollider(CEVec3 const& newPos) {
		position = newPos + offset;
		max = position + extents;
		min = position - extents;
		isAABBCacheDirty = true;
	}

	OBBRect::OBBRect(CEVec3 const& min, CEVec3 const& max, f32 newRotation) : Collider{ ColliderType::OBB }, min{ min }, max{ max }, rotation{ 0.f } {
		
		extents = (max - min) * 0.5f;

		originalVertices[0] = min;

		originalVertices[1] = min;
		originalVertices[1].y += extents.y * 2.f;

		originalVertices[2] = max;

		originalVertices[3] = min;
		originalVertices[3].x += extents.x * 2.f;

		/*If angle of rotation is 0.f*/
		if (!rotation) {
			rotatedVertices = originalVertices;
			/*Normals will just be straight forward, pointing left, up, right, down for unrotated obb*/
			normalEdges[0] = CEVec3{ -1.f,0.f,originalVertices[0].z };
			normalEdges[1] = CEVec3{ 0.f,1.f,originalVertices[0].z };
			normalEdges[2] = CEVec3{ 1.f,0.f,originalVertices[0].z };
			normalEdges[3] = CEVec3{ 0.f,-1.f,originalVertices[0].z };

		}

		UpdateRotation(newRotation);

	}

	Polygon::Polygon() : Collider{ColliderType::POLYGON} {}

	Polygon::Polygon(std::initializer_list<CEVec3> newVertices, f32 newRotation, CEVec3 const &newPosition, CEVec3 const &newOffset) 
		: Collider{ ColliderType::POLYGON }, vertexCount{ std::min((int)newVertices.size(), MAX_VERTEX) }, rotation{ 0.f } {
		
		ColliderType = ColliderType::POLYGON;
		offset = newOffset;
		position = newPosition;

		int idx{};
		for (CEVec3 const& vert : newVertices) {
			vertices[idx] = CEVec3{ vert.x, vert.y, 0.f };
			++idx;
		}

		if (newRotation) {
			UpdateRotation(newRotation);
			CalcNormals();
		}
		else {
			rotatedVertices = vertices;
			CalcNormals();
		}

		float minX = vertices[0].x, maxX = vertices[0].x;
		float minY = vertices[0].y, maxY = vertices[0].y;
		// Calculate initial extents (bounding box half-size)
		if (vertexCount > 0) {
			for (int i = 1; i < vertexCount; ++i) {
				minX = std::min(minX, vertices[i].x);
				maxX = std::max(maxX, vertices[i].x);
				minY = std::min(minY, vertices[i].y);
				maxY = std::max(maxY, vertices[i].y);
			}
		}

		extents = CEVec3((maxX - minX) * 0.5f, (maxY - minY) * 0.5f, 0.f);
	}

	void Polygon::CalcNormals() {
		for (int i{}; i < vertexCount; ++i) {
			int nextVertexIdx{ (i + 1) % vertexCount };
			CEVec3 edge{ rotatedVertices[nextVertexIdx] - rotatedVertices[i] };
			CEVec3 rotatedEdge{ -edge.y, edge.x, edge.z };
			normalEdges[i] = CEMath::Normalize(rotatedEdge);
		}
	}

	CEVec3 Polygon::CalcNormal(CEVec3 const &edge) {
		return CEVec3 { -edge.y, edge.x, edge.z };
	}

	void Polygon::Rescale(const CEVec3& newExtents) {
		// Calculate scale factor relative to current extents
		float scaleX = newExtents.x / (extents.x > 0 ? extents.x : 1.0f);
		float scaleY = newExtents.y / (extents.y > 0 ? extents.y : 1.0f);

		for (int i = 0; i < vertexCount; ++i) {
			// Step A: Move vertex to local space (relative to position)
			float localX = vertices[i].x - position.x;
			float localY = vertices[i].y - position.y;

			// Step B: Apply scale
			localX *= scaleX;
			localY *= scaleY;

			// Step C: Move back to world space
			vertices[i].x = position.x + localX;
			vertices[i].y = position.y + localY;
		}
		extents = newExtents;
		// Update physics state
		ForceUpdateRotation(rotation);
	}

	f32 Polygon::CalculateInertia(f32 mass) {
		f32 numeratorSum{};
		f32 totalArea{};

		for (int i{}; i < vertexCount; ++i) {
			int nextIdx{ (i + 1) % vertexCount };

			// Vectors from Center (position) to vertices
			CEVec3 v1{ vertices[i] - position };
			CEVec3 v2{ vertices[nextIdx] - position };

			// 1. Calculate Area of this triangle (Cross Product Magnitude / 2)
			// For 2D on Z-plane, Cross Product is (x1*y2 - y1*x2)
			f32 crossZ = std::abs(v1.x * v2.y - v1.y * v2.x);
			f32 triArea = crossZ * 0.5f;

			totalArea += triArea;

			// 2. Calculate Geometric Inertia part for this triangle
			// Formula: Area * (v1^2 + v2^2 + v1.v2) / 6
			// We accumulate just the (v1^2... part * Area) first
			f32 term = (CEMath::LengthSq(v1) + CEMath::LengthSq(v2) + CEMath::Dot(v1, v2));

			// We multiply by density later, effectively:
			// Inertia = Sum( Density * Area * term / 6 )
			// Inertia = (Mass / TotalArea) * Sum( Area * term / 6 )
			numeratorSum += (triArea * term);
		}

		if (totalArea == 0.f) return 0.f;
		
		// I = (Mass / TotalArea) * (Sum(Area * Term) / 6)
		// Density = Mass / TotalArea
		return (mass / totalArea) * (numeratorSum / 6.f);
	}

	Polygon Polygon::CreateOBB(CEVec3 const &min, CEVec3 const &max, f32 rotation, CEVec3 offset) {
		CEVec3 extents{ 0.5f*(max - min) };
		CEVec3 center{ max - extents };
		
		Polygon poly{ {min, CEVec3{min.x, min.y + extents.y * 2.f, 0.f}, max, CEVec3{min.x + extents.x * 2.f, min.y, center.z}}, rotation, center, offset };
		//poly.ColliderType = ColliderType::OBB;
		poly.ColliderType = ColliderType::POLYGON;

		return poly;
	}

	void Polygon::UpdateOBB(CEVec3 const& min, CEVec3 const& max, Polygon& polygon) {

		CEVec3 extents{ (max - min) * 0.5f };
		polygon.vertices[0] = min;
		polygon.vertices[1] = CEVec3{min.x, min.y + extents.y * 2.f, 0.f};
		polygon.vertices[2] = max;
		polygon.vertices[3] = CEVec3{ min.x + extents.x * 2.f, min.y , 0.f };

		polygon.ForceUpdateRotation(0.f);
	}

	CEVec3 Polygon::CalculateOBBExtents(Polygon const& polygon) {
		CEVec3 position{ polygon.position.x, polygon.position.y, 0.f };
		CEVec3 extents{ position.x - polygon.vertices[0].x, position.y - polygon.vertices[0].y, 0.f };
		return extents;
	}

	Polygon Polygon::CreateAABB(CEVec3 const& min, CEVec3 const& max, CEVec3 offset) {

		Polygon poly{};
		poly.offset = offset;
		float z = 0.f;

		poly.vertexCount = 4;

		// Vertex 0 (Bottom-Right)
		poly.rotatedVertices[0] = { max.x, min.y, z };
		// Vertex 1 (Bottom-Left)
		poly.rotatedVertices[1] = { min.x, min.y, z };
		// Vertex 2 (Top-Left)
		poly.rotatedVertices[2] = { min.x, max.y, z };
		// Vertex 3 (Top-Right)
		poly.rotatedVertices[3] = { max.x, max.y, z };

		// --- CORRESPONDING NORMALS ---
		// Edge 0 (V0->V1): Bottom Edge -> Normal Points Down (0, -1)
		poly.normalEdges[0] = CEVec3{ 0.f, -1.f, 0.f };

		// Edge 1 (V1->V2): Left Edge -> Normal Points Left (-1, 0)
		poly.normalEdges[1] = CEVec3{ -1.f, 0.f, 0.f };

		// Edge 2 (V2->V3): Top Edge -> Normal Points Up (0, 1)
		poly.normalEdges[2] = CEVec3{ 0.f, 1.f, 0.f };

		// Edge 3 (V3->V0): Right Edge -> Normal Points Right (1, 0)
		poly.normalEdges[3] = CEVec3{ 1.f, 0.f, 0.f };

		return poly;
	}

	void Polygon::UpdateRotation(f32 newRotation) {

		/*Check if rotation didnt change from last time to avoid expensive calculation*/
		/*same thing differernt day*/
		if (rotation == newRotation) return;

		rotation = newRotation;
		CEMtx33 rotationMtx{};
		rotationMtx = rotationMtx.RotationDegrees(rotation);

		for (int i{}; i < vertexCount; ++i) {
			rotatedVertices[i] = vertices[i] - position;
			rotatedVertices[i] = { rotationMtx * rotatedVertices[i] };
			rotatedVertices[i] += position;
		}

		CalcNormals();
		isAABBCacheDirty = true;
	}

	void Polygon::ForceUpdateRotation(f32 newRotation) {

		/*Check if rotation didnt change from last time to avoid expensive calculation*/
		/*same thing differernt day*/

		rotation = newRotation;
		CEMtx33 rotationMtx{};
		rotationMtx = rotationMtx.RotationDegrees(rotation);

		for (int i{}; i < vertexCount; ++i) {
			rotatedVertices[i] = vertices[i] - position;
			rotatedVertices[i] = { rotationMtx * rotatedVertices[i] };
			rotatedVertices[i] += position;
		}

		CalcNormals();
		isAABBCacheDirty = true;
	}

	void Polygon::UpdatePosition(CEVec3 const& newPos) {

		CEVec3 vecToNewPos{ (newPos + offset) - position };
		position = newPos + offset;

		for (int i{}; i < vertexCount; ++i) {
			rotatedVertices[i] += vecToNewPos;
			vertices[i] += vecToNewPos;
		}
		isAABBCacheDirty = true;
	}

	void Polygon::UpdateCollider(CEVec3 const& newPos) {
		
		UpdatePosition(newPos);
		
	}

	void OBBRect::UpdateRotation(f32 newRotation) {

		/*Check if rotation didnt change from last time to avoid expensive calculation*/
		if (rotation == newRotation) return;

		rotation = newRotation;
		CEMtx33 rotationMtx{};
		rotationMtx = rotationMtx.RotationDegrees(rotation);

		CEVec3 center{ min + extents };

		Vertices origin{
			min - center,
			CEVec3{min.x, min.y + extents.y * 2.f, min.z} - center,
			max - center,
			CEVec3{min.x + extents.x * 2.f, min.y, min.z} - center
		};

		rotatedVertices = origin;

		for (int i{}; i < 4; ++i) {
			rotatedVertices[i] = { rotationMtx * rotatedVertices[i] };
			rotatedVertices[i] += center;
		}

		for (int i{}; i < 4; ++i) {
			int nextVertexIdx{ (i + 1) % 4 };
			CEVec3 edge{ rotatedVertices[nextVertexIdx] - rotatedVertices[i] };
			CEVec3 rotatedEdge{ -edge.y, edge.x, edge.z };
			normalEdges[i] = rotatedEdge;
			normalEdges[i] = CEMath::Normalize(normalEdges[i]);

		}
		isAABBCacheDirty = true;
	}

	void OBBRect::UpdateCollider(CEVec3 const& newPos) {
		CEVec3 currPos{ min + extents };
		CEVec3 vecToNewPos{ newPos - currPos };

		vecToNewPos.z = 0.f;

		max = newPos + extents;
		min = newPos - extents;

		rotatedVertices[0] += vecToNewPos;
		rotatedVertices[1] += vecToNewPos;
		rotatedVertices[2] += vecToNewPos;
		rotatedVertices[3] += vecToNewPos;
		isAABBCacheDirty = true;
	}

	LineSegment::LineSegment(const CEVec3& start, const CEVec3& end) :
		Collider{ ColliderType::LINE }, p1{ start }, p2{ end } {
		ColliderType = ColliderType::LINE;
	}

	void LineSegment::UpdateCollider(CEVec3 const& newPos) {
		position = newPos + offset;
		
		worldP1 = p1 + position;
		worldP2 = p2 + position;

		// Calculate Normal (Perpendiculate 2D vector: -y, x)
		CEVec3 dir = worldP2 - worldP1;
		normal = CEMath::Normalize(CEVec3{ -dir.y, dir.x, 0.f });
		isAABBCacheDirty = true;
	}

	void LineSegment::UpdateRotation(f32 newRotation) {
		// Apply rotation matrix to p1 and p2, then add position
		CEMtx33 rotMtx;
		rotMtx = rotMtx.RotationDegrees(newRotation);

		worldP1 = (rotMtx * p1) + position;
		worldP2 = (rotMtx * p2) + position;

		/// Calculate Normal (Perpendiculate 2D vector: -y, x)
		CEVec3 dir = worldP2 - worldP1;
		normal = CEMath::Normalize(CEVec3{ -dir.y, dir.x, 0.f });
		isAABBCacheDirty = true;
	}
}