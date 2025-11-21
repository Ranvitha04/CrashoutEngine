#include "quadtree.h"
#include "colliders.h"
#include "collidercomponent.h"
#include "transformcomponent.h"
#include "shapes.h"
#include "camera.h"

namespace {
	static void DrawAABBOutline(const CEVec3& minP, const CEVec3& maxP, float thickness, const CEVec3& color) {
		using namespace CrashoutsEngine;

		float z = RenderCamera::engineCamera.GetFar();
		CEVec3 p0(minP.x, minP.y, z);
		CEVec3 p1(maxP.x, minP.y, z);
		CEVec3 p2(maxP.x, maxP.y, z);
		CEVec3 p3(minP.x, maxP.y, z);

		CEVec2 size = { thickness, thickness };
		
		using namespace Component;

		Shapes::CreateLine(p0, p1, thickness, color);
		Shapes::CreateLine(p1, p2, thickness, color);
		Shapes::CreateLine(p2, p3, thickness, color);
		Shapes::CreateLine(p3, p0, thickness, color);
	}
}

namespace DataStructure {
	QuadTree::QuadTree(Collision::AABBRect const& boundary, int const& depth) : boundary{ boundary }, depth{ depth } {
		Resize(boundary);
		divided = false;
	}

	QuadTree::~QuadTree() {
		Clear();
	}

	void QuadTree::Insert(CrashoutsEngine::Entity const& entity)
	{
		CrashoutsEngine::ECS& globalECS{ *(CrashoutsEngine::ECS::getInstance()) };
		if (globalECS.HasComponent<Component::ColliderComponent>(entity) && globalECS.HasComponent<Component::TransformComponent>(entity)) {
			Component::ColliderComponent const& collider{ *globalECS.GetComponent<Component::ColliderComponent>(entity) };
			Component::TransformComponent const& transform{ *globalECS.GetComponent<Component::TransformComponent>(entity) };

			Collision::AABBRect entityAABB{ Collision::GetAABB(*collider.collider, transform.position) };

			for (int i{}; i < 4; ++i) {
				if (childSizes[i].contains(entityAABB)) {
					if (depth + 1 < maxDepth) {

						divided = true;
						if (!childRegions[i]) {
							childRegions[i] = std::make_unique<QuadTree>(QuadTree{ childSizes[i], depth + 1 });
						}
						childRegions[i]->Insert(entity); /*Recursively add data items to children if it is contained within its boundary*/
						return;
					}
					else {
						break;
					}
				}
			}

			/*Insert into up most node if it dosent fit into any other children*/
			objects.push_back({ entity, entityAABB });
		}
	}

	/*For collision*/
	void QuadTree::Query(Collision::AABBRect& region, std::vector<CrashoutsEngine::Entity>& collection)
	{
		for (int i{}; i < 4; ++i)
		{
			if (childRegions[i]) {
				if (childRegions[i]->boundary.intersect(region)) {
					childRegions[i]->Query(region, collection);
				}
			}
		}

		for (int i{}; i < objects.size(); ++i) {
			if(objects[i].second.intersect(region)) collection.push_back(objects[i].first);
		}

	}

	void QuadTree::Clear() {
		objects = std::vector<std::pair<CrashoutsEngine::Entity, Collision::AABBRect>>{};

		for (int i{}; i < 4; ++i) {
			if (childRegions[i])
				childRegions[i]->Clear();
			childRegions[i].reset();
		}

	}

	void QuadTree::Resize(Collision::AABBRect const& newBoundary) {
		Clear();
		boundary = newBoundary;
		CEVec3 extents{ (newBoundary.max - newBoundary.min) * 0.5f };
		childSizes = {
			Collision::AABBRect{CEVec3{newBoundary.min + extents}, CEVec3{newBoundary.max}},
			Collision::AABBRect{CEVec3{newBoundary.min.x, newBoundary.min.y + extents.y, 0.f}, CEVec3{newBoundary.max.x - extents.x, newBoundary.max.y, 0.f}},
			Collision::AABBRect{CEVec3{newBoundary.min}, CEVec3{newBoundary.min + extents}},
			Collision::AABBRect{CEVec3{newBoundary.min.x + extents.x, newBoundary.min.y, 0.f}, CEVec3{newBoundary.max.x, newBoundary.max.y - extents.y, 0.f}}
		};

	}

	void QuadTree::Draw() {
		DrawAABBOutline(boundary.min, boundary.max, 0.8f, CEVec3{ 1.f, 0.f, 0.f });

		for (int i{}; i < 4; ++i) {
			if (!divided) return;
			if (!childRegions[i]) { continue; }
			childRegions[i]->Draw();
		}
	}
}