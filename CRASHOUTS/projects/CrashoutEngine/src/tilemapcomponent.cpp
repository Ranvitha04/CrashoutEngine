/*!
@file   tilemapcomponent.cpp
@author lee.r@digipen.edu
@date   19/11/2025

@brief	This file defines the functions that would be used by the
		tilemapcomponent.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "engine_pch.h"
#include "rendering.h"
#include "shapes.h"
#include "tilemapcomponent.h"
#include "collidercomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "rigidbodycomponent.h"
#include "transientcomponent.h"
#include "layercomponent.h"
#include "namecomponent.h"
#include "assetmanager.h"
#include "tilemapasset.h"
#include "tagcomponent.h"
#include "textureasset.h"
#include "hierarchycomponent.h"
#include "hierarchy.h"
#include <filesystem>
#include <cmath> // For std::floor
#include "inputmanager.h"
#include "engine.h"

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine::TileMapUtils {

	// Directions for scanning
	enum class EdgeType { Top, Bottom, Left, Right };

	// Simple struct to hold a generated line segment in Grid Space
	struct SegmentDef {
		int x1, y1; // Start Grid Coords
		int x2, y2; // End Grid Coords
		EdgeType edge;
	};

	using namespace CrashoutsEngine::Assets;

	std::pair<int, int> WorldToGrid(const Component::TileMapComponent& map, float worldX, float worldY) {
		// Use std::floor to handle negative coordinates.
		int gridX = static_cast<int>(std::floor(worldX / map.tileSize));
		int gridY = static_cast<int>(std::floor(worldY / map.tileSize));
		return { gridX, gridY };
	}

	void PaintTile(Component::TileMapComponent& map, int gridX, int gridY, const std::string& assetName) {
		std::pair<int, int> key = { gridX, gridY };

		// Check if this tile is already the right value
		if (map.gridData.count(key) && map.gridData[key] == assetName) return;

		// Update / Insert the tile
		map.gridData[key] = assetName;

		// Mark dirty to tell program to recalculate rules etc
		map.isCacheDirty = true;
	}

	void EraseTile(Component::TileMapComponent& map, int gridX, int gridY) {
		std::pair<int, int> key{ gridX, gridY };

		if (!map.gridData[key].empty()) {
			map.gridData.erase(key);
			map.isCacheDirty = true;
		}
	}

	/*!
	@brief  Get the bitmask for a specific tile based on its neighbours
	@param  map TileMapComponent to reference.
	@param  x	    The grid key x value.
	@param  y	    The grid key y value.
	@param  asset   The current tile map asset that is used.
	*/
	static unsigned int CalculateBitMask(Component::TileMapComponent& map, int x, int y, const std::string& asset) {
		unsigned int mask = 0;

		// Define bitmask values for cardinal directions only
		enum {
			Up = 0b0001,     // 1
			Right = 0b0010,     // 2
			Down = 0b0100,     // 4
			Left = 0b1000      // 8
		};

		// Helper lambda to check a specific cell
		auto Check = [&](int gx, int gy) -> bool {
			// 1. Check bounds/existence
			if (map.gridData.count({ gx, gy })) {
				// 2. Check if it matches the current asset (Connects to same type)
				return map.gridData.at({ gx, gy }) == asset;
			}
			return false;
			};

		// Check Cardinal Directions
		if (Check(x, y + 1)) mask |= Up;
		if (Check(x + 1, y)) mask |= Right;
		if (Check(x, y - 1)) mask |= Down;
		if (Check(x - 1, y)) mask |= Left;

		return mask;
	}

	// Helper to check if a specific grid cell is solid
	bool IsSolid(const Component::TileMapComponent& map, int x, int y) {
		// 1. Check if anything is painted there
		auto it = map.gridData.find({ x, y });
		if (it == map.gridData.end()) return false;

		std::string assetName = it->second;

		// 2. Get the Asset data
		using namespace CrashoutsEngine::Assets;
		TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(assetName);
		if (!asset) return false;

		// 3. Calculate the Rule Mask for this specific tile
		// (We need to verify if this tile connects to its neighbors to find the right rule)
		unsigned int mask = CalculateBitMask(const_cast<Component::TileMapComponent&>(map), x, y, assetName);

		// 4. Find the matching RuleTile
		const RuleTile* matchedRule = nullptr;
		for (const RuleTile& rule : asset->GetTiles()) {
			if (rule.ruleBitMap == mask) {
				matchedRule = &rule;
				break;
			}
		}

		// 5. Fallback rule (Index 0) if no specific mask matched
		if (!matchedRule && !asset->GetTiles().empty()) {
			matchedRule = &asset->GetTiles()[0];
		}

		// 6. The Final Check: Does this specific rule have a collider?
		if (matchedRule && matchedRule->colliderType != Collision::ColliderType::NONE) {
			return true;
		}

		return false;
	}

	// The Greedy Algorithm
	std::vector<SegmentDef> GenerateGreedySegments(const Component::TileMapComponent& map) {
		std::vector<SegmentDef> segments;

		if (map.gridData.empty()) return segments;

		// 1. Determine Bounds of the current map
		int minX = 99999, maxX = -99999, minY = 99999, maxY = -99999;
		for (const auto& [pos, val] : map.gridData) {
			if (pos.first < minX) minX = pos.first;
			if (pos.first > maxX) maxX = pos.first;
			if (pos.second < minY) minY = pos.second;
			if (pos.second > maxY) maxY = pos.second;
		}

		// Add 1 to max to ensure we check the empty space right after the last tile
		maxX++; maxY++;

		// 2. Horizontal Pass (Find Top and Bottom edges)
		for (int y = minY; y <= maxY; ++y) {
			// Trackers for current line. -1 means no line currently active.
			int startTop = -1;
			int startBottom = -1;

			for (int x = minX; x <= maxX; ++x) {
				bool currSolid = IsSolid(map, x, y);
				bool aboveSolid = IsSolid(map, x, y + 1);
				bool belowSolid = IsSolid(map, x, y - 1);

				// --- TOP EDGE LOGIC (Current is Solid, Above is Empty) ---
				if (currSolid && !aboveSolid) {
					if (startTop == -1) startTop = x; // Start new line
				}
				else {
					if (startTop != -1) {
						// Line ended, push it
						segments.push_back({ startTop, y, x, y, EdgeType::Top });
						startTop = -1;
					}
				}

				// --- BOTTOM EDGE LOGIC (Current is Solid, Below is Empty) ---
				if (currSolid && !belowSolid) {
					if (startBottom == -1) startBottom = x; // Start new line
				}
				else {
					if (startBottom != -1) {
						segments.push_back({ startBottom, y, x, y, EdgeType::Bottom });
						startBottom = -1;
					}
				}
			}
		}

		// 3. Vertical Pass (Find Left and Right edges)
		for (int x = minX; x <= maxX; ++x) {
			int startLeft = -1;
			int startRight = -1;

			for (int y = minY; y <= maxY; ++y) {
				bool currSolid = IsSolid(map, x, y);
				bool leftSolid = IsSolid(map, x - 1, y);
				bool rightSolid = IsSolid(map, x + 1, y);

				// --- LEFT EDGE LOGIC (Current Solid, Left Empty) ---
				if (currSolid && !leftSolid) {
					if (startLeft == -1) startLeft = y;
				}
				else {
					if (startLeft != -1) {
						// Note: Storing Y in x1/x2 slots for convenience, handled in generation
						segments.push_back({ x, startLeft, x, y, EdgeType::Left });
						startLeft = -1;
					}
				}

				// --- RIGHT EDGE LOGIC (Current Solid, Right Empty) ---
				if (currSolid && !rightSolid) {
					if (startRight == -1) startRight = y;
				}
				else {
					if (startRight != -1) {
						segments.push_back({ x, startRight, x, y, EdgeType::Right });
						startRight = -1;
					}
				}
			}
		}

		return segments;
	}

	void RebuildCache(Component::TileMapComponent& map, const Entity& parentEnt) {
		if (!map.isCacheDirty) return;

		// 1. Clear Visual Cache
		map.renderCache.clear();

		// 2. Clear/Destroy Old Physics Entities
		// We destroy all old tile colliders and rebuild them. 
		ECS* ecs = ECS::getInstance();

		for (auto const& [key, entityID] : map.tileCache) {
			CrashoutsEngine::DetachChild(*ecs, entityID);
			ecs->DestroyEntity(entityID);
		}
		map.tileCache.clear();

		// B. Clear Segment Entities (Greedy Lines)
		for (Entity entityID : map.segmentCache) {
			CrashoutsEngine::DetachChild(*ecs, entityID);
			ecs->DestroyEntity(entityID);
		}
		map.segmentCache.clear();

		Component::LayerComponent* parentLayer = nullptr;
		if (ecs->HasComponent<Component::LayerComponent>(parentEnt)) {
			parentLayer = ecs->GetComponent<Component::LayerComponent>(parentEnt);
		}

		// Iterate over every painted tile
		for (auto const& [pos, tileAssetName] : map.gridData) {
			// A. Get the Asset
			TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(tileAssetName);

			if (!asset) {
				std::string fixedPath = "tilemaps/" + tileAssetName;
				asset = AssetManager::GetInstance().Get<TileMapAsset>(fixedPath);
			}

			if (!asset) continue;

			// B. Calculate Bitmask & Find Rule
			unsigned int currentMask = CalculateBitMask(map, pos.first, pos.second, tileAssetName);
			const RuleTile* matchedRule = nullptr;

			// Helper Lambda: Searches the asset for a specific mask
			auto FindRuleForMask = [&](unsigned int searchMask) -> const RuleTile* {
				for (const RuleTile& rule : asset->GetTiles()) {
					if (rule.ruleBitMap == searchMask) {
						return &rule;
					}
				}
				return nullptr;
				};

			// ---------------------------------------------------------
			// EXACT MATCH (The Perfect Fit)
			// ---------------------------------------------------------
			matchedRule = FindRuleForMask(currentMask);

			// ---------------------------------------------------------
			//      FALLBACK
			// ---------------------------------------------------------
			if (!matchedRule && !asset->GetTiles().empty()) {
				matchedRule = &asset->GetTiles()[0]; // Default tile
			}

			if (!matchedRule) continue;

			// --- C. HANDLE VISUALS ---
			GLuint handle = Component::TextureRenderer::GetTextureHandle(matchedRule->texAssetName);
			if (handle != 0) map.renderCache[pos] = handle;

			// --- D. HANDLE PHYSICS (Per-Tile Entities) ---			
			Entity ent = ecs->CreateEntity();

			// Mark entity as transient (Runtime only)
			ecs->AddComponent<Component::TransientComponent>(ent);

			// 1. Name it for debugging
			std::string name = "Tile (x:" + std::to_string(pos.first) + ",y:" + std::to_string(pos.second) + ")";
			ecs->AddComponent<Component::NameComponent>(ent, name);

			ecs->AddComponent<Component::TagComponent>(ent, asset->GetLayerMask());

			// Add Sprite Component
			Component::ShapeComponent s;
			s.enableTexture = true;
			s.textureName = matchedRule->texAssetName;
			s.type = Component::ShapeType::Rectangle;
			ecs->AddComponent<Component::ShapeComponent>(ent, std::move(s));

			// 2. Transform (Place it in world space)
			Component::TransformComponent& t = ecs->AddComponent<Component::TransformComponent>(ent);
			t.position.x = (float)pos.first * map.tileSize + map.tileSize * .5f;
			t.position.y = (float)pos.second * map.tileSize + map.tileSize * .5f;
			t.position.z = ecs->GetComponent<Component::TransformComponent>(parentEnt)->position.z;
			t.size = CEVec2((float)map.tileSize, (float)map.tileSize);

			// Apply Parent Layer to Child
			if (parentLayer) {
				// Copy the parent's layer settings exactly
				ecs->AddComponent<Component::LayerComponent>(ent, *parentLayer);
			}
			else {
				// Fallback if parent has no layer (rare, but safe)
				ecs->AddComponent<Component::LayerComponent>(ent, Component::LayerComponent{ "Default", 0 });
			}

			// 3. Collider
			if (matchedRule->colliderType != Collision::ColliderType::NONE &&
				matchedRule->colliderType != Collision::ColliderType::AABB) {

				// Prepare common variables
				float halfW = t.size.x * 0.5f;
				float halfH = t.size.y * 0.5f;
				CEVec3 tPos = t.position;
				float rot = t.rotation;

				std::unique_ptr<Collision::Collider> newCollider = nullptr;
				Collision::ColliderType helperT = Collision::ColliderType::CIRCLE; // Default

				switch (matchedRule->colliderType) {
					// --------------------------------------------------------
					// ColliderType::CIRCLE
					// --------------------------------------------------------
				case Collision::ColliderType::CIRCLE: {
					// Radius is usually half the width (or average of width/height)
					float radius = std::min(halfW, halfH);
					newCollider = std::make_unique<Collision::Circle>(radius, tPos);
					helperT = Collision::ColliderType::CIRCLE;
					break;
				}
													// --------------------------------------------------------
													// ColliderType::OBB (Oriented Bounding Box)
													// --------------------------------------------------------
				case Collision::ColliderType::OBB: {
					// ColliderType::OBB needs min/max (usually unrotated or local) + rotation
					CEVec3 min{ tPos.x - halfW, tPos.y - halfH, 0.0f };
					CEVec3 max{ tPos.x + halfW, tPos.y + halfH, 0.0f };
					newCollider = std::make_unique<Collision::OBBRect>(min, max, rot);
					helperT = Collision::ColliderType::OBB;
					break;
				}
												 // --------------------------------------------------------
												 // ColliderType::POLYGON (Box Shape)
												 // --------------------------------------------------------
				case Collision::ColliderType::POLYGON: {
					// Create a box polygon relative to the center
					// Vertices are defined relative to the origin (0,0), 
					// the Polygon class handles adding the position.
					CEVec3 v1{ -halfW, -halfH, 0.0f };
					CEVec3 v2{ halfW, -halfH, 0.0f };
					CEVec3 v3{ halfW,  halfH, 0.0f };
					CEVec3 v4{ -halfW,  halfH, 0.0f };

					newCollider = std::make_unique<Collision::Polygon>(
						std::initializer_list<CEVec3>{v1, v2, v3, v4},
						rot,
						tPos
					);
					helperT = Collision::ColliderType::POLYGON;
					break;
				}
				default:
					break;
				}
				// Assign to Entity if created successfully
				if (newCollider) {
					// IMPORTANT: Set the type so your Inspector can read it correctly
					newCollider->ColliderType = helperT;

					Component::ColliderComponent& c = ecs->AddComponent<Component::ColliderComponent>(ent);
					c.collider = std::move(newCollider);

					// 5. RigidBody (CRITICAL FOR COLLISION)
					// We add a Static Rigidbody (Mass = 0) so the physics engine knows this is a wall
					if (!ecs->HasComponent<Component::RigidBodyComponent>(ent)) {
						ecs->AddComponent<Component::RigidBodyComponent>(ent, CEVec3{ 0.f,0.f,0.f }, 0.0f, 0.0f);
					}
				}
			}

			// 6. Cache
			map.tileCache[pos] = ent;

			// 7. Parent it (Optional, if your hierarchy system supports it)
			CrashoutsEngine::AttachChild(*ecs, parentEnt, ent);
		}

		// FOR LINE SEGMENTS
		{
			// Generate Optimized Physics Entities
			std::vector<SegmentDef> segments = GenerateGreedySegments(map);
			float tSize = (float)map.tileSize;

			// Get Parent Z depth to keep things aligned
			float parentZ = ecs->GetComponent<Component::TransformComponent>(parentEnt)->position.z;

			// Create entity for collision data
			for (const auto& seg : segments) {
				Entity ent = ecs->CreateEntity();
				ecs->AddComponent<Component::TransientComponent>(ent);
				ecs->AddComponent<Component::NameComponent>(ent, "MapSegment");

				// Static Rigidbody
				ecs->AddComponent<Component::RigidBodyComponent>(ent, CEVec3{ 0,0,0 }, 0.0f, 0.0f);

				// Add layer data
				// 1. Find the asset name used at the start of this segment
				if (map.gridData.count({ seg.x1, seg.y1 })) {
					std::string aName = map.gridData.at({ seg.x1, seg.y1 });

					// 2. Get Asset Data
					TileMapAsset* segAsset = AssetManager::GetInstance().Get<TileMapAsset>(aName);
					if (!segAsset) segAsset = AssetManager::GetInstance().Get<TileMapAsset>("tilemaps/" + aName);

					// 3. Apply Layer
					if (segAsset) {
						ecs->AddComponent<Component::TagComponent>(ent, segAsset->GetLayerMask());
					}
					else {
						// Default layer if asset missing (Safety)
						ecs->AddComponent<Component::TagComponent>(ent, 0);
					}
				}

				// --- Calculate World Points ---
				CEVec3 startPos{ 0,0,0 };
				CEVec3 endPos{ 0,0,0 };

				// Coordinates are Grid coordinates (Integers).
				// 0,0 Grid -> 0.0f World. 
				// 1,0 Grid -> 32.0f World.

				// Adjust based on Edge Type to align with the edge of the tile, not the center
				switch (seg.edge) {
				case EdgeType::Top:
					// Top of row y. Spans x1 to x2.
					startPos = { seg.x1 * tSize, (seg.y1 + 1) * tSize, parentZ };
					endPos = { seg.x2 * tSize, (seg.y2 + 1) * tSize, parentZ };
					break;
				case EdgeType::Bottom:
					// Bottom of row y. Spans x1 to x2.
					startPos = { seg.x1 * tSize, seg.y1 * tSize, parentZ };
					endPos = { seg.x2 * tSize, seg.y2 * tSize, parentZ };
					break;
				case EdgeType::Left:
					// Left of col x. Spans y1 to y2.
					startPos = { seg.x1 * tSize, seg.y1 * tSize, parentZ };
					endPos = { seg.x1 * tSize, seg.y2 * tSize, parentZ };
					break;
				case EdgeType::Right:
					// Right of col x. Spans y1 to y2.
					startPos = { (seg.x1 + 1) * tSize, seg.y1 * tSize, parentZ };
					endPos = { (seg.x1 + 1) * tSize, seg.y2 * tSize, parentZ };
					break;
				}

				// --- Create Transform ---
				// We position the entity at the MIDPOINT of the line
				Component::TransformComponent& t = ecs->AddComponent<Component::TransformComponent>(ent);
				t.position = (startPos + endPos) * 0.5f;
				t.size = { 1.0f, 1.0f }; // Scale doesn't affect LineSegment logic usually, but good to keep 1
				t.rotation = 0.0f;

				// --- Create Line Collider ---
				// Calculate Local P1 and P2 relative to the center (Transform)
				CEVec3 localP1 = startPos - t.position;
				CEVec3 localP2 = endPos - t.position;

				auto lineCol = std::make_unique<Collision::LineSegment>(localP1, localP2);
				lineCol->ColliderType = Collision::ColliderType::LINE;

				// Set Normal based on edge type (Important for physics resolution)
				CEVec3 normal{ 0,0,0 };
				if (seg.edge == EdgeType::Top) normal = { 0, 1, 0 };
				else if (seg.edge == EdgeType::Bottom) normal = { 0, -1, 0 };
				else if (seg.edge == EdgeType::Left) normal = { -1, 0, 0 };
				else if (seg.edge == EdgeType::Right) normal = { 1, 0, 0 };

				lineCol->SetNormal(normal);

				ecs->AddComponent<Component::ColliderComponent>(ent, Component::ColliderComponent{ std::move(lineCol) });

				// Parent it
				CrashoutsEngine::AttachChild(*ecs, parentEnt, ent);

				// Cache it using a unique key 
				// For simplicity: We just add it to tileCache using the Start Grid Pos as key.
				map.segmentCache.push_back(ent);
			}
		}

		map.isCacheDirty = false;

		std::cout << "TileMapComponent: Cache Rebuilt. Tiles: " << map.tileCache.size() << "\n";
		map.isCacheDirty = false;
	}
}

namespace CrashoutsEngine::Editor {

	void DrawTileMapEditor(Component::TileMapComponent* tmComp, bool* pOpen) {
		if (!*pOpen) return;

		if (!tmComp) {
			std::cout << "Tile Map Component is missing!!!" << std::endl;
			return;
		}

		// Create the IMGUI window
		if (ImGui::Begin("TileMap Palette", pOpen)) {

			ImGui::Text("Active Tile Map: %s", tmComp->activeAsset.empty() ? "None" : tmComp->activeAsset.c_str());
			ImGui::Separator();

			// --- 1. Asset Selector ---
			// List all avaialble .json files in the tilemaps folder
			if (ImGui::Button("Load/ Select Asset...")) {
				ImGui::OpenPopup("SelectRuleAsset");
			}

			if (ImGui::BeginPopup("SelectRuleAsset")) {
				namespace fs = std::filesystem;
				std::string path = GetGameRoot() + "/assets/tilemaps/";

				if (fs::exists(path)) {
					for (const auto& entry : fs::directory_iterator(path)) {
						std::string filename = entry.path().filename().string();
						if (ImGui::Selectable(filename.c_str())) {
							tmComp->activeAsset = "tilemaps/" + filename;
						}
					}
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			// --- 2. Painting Mode Info ---
			ImGui::TextColored(ImVec4(.5f, 1.f, .5f, 1.f), "Painting Mode Active");
			ImGui::Text("LMB: Paint Tile");
			ImGui::Text("RMB: Erase Tile");

			ImGui::SliderInt("Tile Size", &tmComp->tileSize, 8, 128);

			// --- 3. Force Refresh Button (Debug) ---
			if (ImGui::Button("Force Rebuild Cache")) {
				tmComp->isCacheDirty = true;
			}
		}
		ImGui::End();

		// ---------------------------------------------------------
		// SEPARATE DEBUG WINDOW
		// ---------------------------------------------------------
		if (tmComp->pOpen) {
			// ImGuiWindowFlags_AlwaysAutoResize: fits the window to the text content
			// ImGuiWindowFlags_NoFocusOnAppearing: prevents it from stealing focus from the viewport
			if (ImGui::Begin("TileMap Debugger", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing)) {

				// 1. Get Mouse Position in Grid
				CEVec2 worldM = InputManager::g_mouseWorldPos;
				auto gridPos = TileMapUtils::WorldToGrid(*tmComp, worldM.x, worldM.y);

				int x = gridPos.first;
				int y = gridPos.second;

				ImGui::Text("Grid Coordinates: [%d, %d]", x, y);
				ImGui::Separator();

				// 2. Check if tile exists here
				if (tmComp->gridData.count(gridPos)) {
					std::string asset = tmComp->gridData[gridPos];

					// 3. Calculate the mask
					// Note: Ensure 'CalculateBitMask' is accessible (remove 'static' in .cpp if needed)
					unsigned int mask = TileMapUtils::CalculateBitMask(*tmComp, x, y, asset);

					ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Active Tile Found");
					ImGui::Text("Asset: %s", asset.c_str());
					ImGui::Text("Calculated Mask: 0x%02X", mask);

					ImGui::Separator();
					ImGui::Text("Neighbour Grid:");

					// Helper lambda for drawing the state
					auto DrawBitState = [&](int bitIndex) {
						// -1 represents an empty corner
						if (bitIndex == -1) {
							ImGui::TextDisabled("   ");
							return;
						}

						bool isSet = (mask & (1 << bitIndex)) != 0;
						if (isSet) ImGui::TextColored(ImVec4(0, 1, 0, 1), "[1]"); // Green for ON
						else       ImGui::TextDisabled("[0]");                    // Grey for OFF
						};

					// Visual Representation (The "+" Shape)
					ImGui::BeginGroup();

					// Top Row:   [ ] [U] [ ]
					DrawBitState(-1); ImGui::SameLine(); DrawBitState(0); ImGui::SameLine(); DrawBitState(-1);

					// Mid Row:   [L]  @  [R]
					DrawBitState(3); ImGui::SameLine(); ImGui::Text(" @ ");  ImGui::SameLine(); DrawBitState(1);

					// Bot Row:   [ ] [D] [ ]
					DrawBitState(-1); ImGui::SameLine(); DrawBitState(2); ImGui::SameLine(); DrawBitState(-1);

					ImGui::EndGroup();
				}
				else {
					ImGui::TextDisabled("Mouse is over empty space.");
				}
			}
			ImGui::End(); // Close the Debugger Window
		}
	}

};