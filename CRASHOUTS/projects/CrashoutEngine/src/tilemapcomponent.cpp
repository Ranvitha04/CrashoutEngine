/*!
@file   tilemapcomponent.cpp
@author lee.r@digipen.edu
@date   19/11/2025

@brief	This file defines the functions that would be used by the 
		tilemapcomponent.
__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "engine_pch.h"
#include "rendering.h"
#include "shapes.h"
#include "tilemapcomponent.h"
#include "collidercomponent.h"
#include "transformcomponent.h"
#include "namecomponent.h"
#include "assetmanager.h"
#include "tilemapasset.h"
#include "textureasset.h"
#include <filesystem>
#include <cmath> // For std::floor

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine::TileMapUtils {

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
    unsigned int CalculateBitMask(Component::TileMapComponent& map, int x, int y, const std::string& asset) {
        unsigned int mask = 0;

        /*  Rule Bit Map:

           Up:         0000'0001
           Up-Right:   0000'0010
           Right:      0000'0100
           Down-Right: 0000'1000

           Down:       0001'0000
           Down-Left:  0010'0000
           Left:       0100'0000
           Up-Left:    1000'0000
        */
        enum {
            Up = 0b0000'0001,
            UpRight = 0b0000'0010,
            Right = 0b0000'0100,
            DownRight = 0b0000'1000,
            Down = 0b0001'0000,
            DownLeft = 0b0010'0000,
            Left = 0b0100'0000,
            UpLeft = 0b1000'0000
        };

        // Check UP
        if (map.gridData.count({ x, y + 1 }) && map.gridData.at({ x, y + 1 }) == asset) {
            mask |= Up;
        }
        // Check UPRIGHT
        if (map.gridData.count({ x + 1, y + 1 }) && map.gridData.at({ x + 1, y + 1 }) == asset) {
            mask |= UpRight;
        }
        // Check RIGHT
        if (map.gridData.count({ x + 1, y }) && map.gridData.at({ x + 1, y }) == asset) {
            mask |= Right;
        }
        // Check DOWNRIGHT
        if (map.gridData.count({ x + 1, y - 1 }) && map.gridData.at({ x + 1, y - 1 }) == asset) {
            mask |= DownRight;
        }
        // Check DOWN
        if (map.gridData.count({ x, y - 1 }) && map.gridData.at({ x, y - 1 }) == asset) {
            mask |= Down;
        }
        // Check DOWNLEFT
        if (map.gridData.count({ x - 1, y - 1 }) && map.gridData.at({ x - 1, y - 1 }) == asset) {
            mask |= DownLeft;
        }
        // Check LEFT
        if (map.gridData.count({ x - 1, y }) && map.gridData.at({ x - 1, y }) == asset) {
            mask |= Left;
        }
        // Check UPLEFT
        if (map.gridData.count({ x - 1, y + 1 }) && map.gridData.at({ x - 1, y + 1 }) == asset) {
            mask |= UpLeft;
        }

        return mask;
    }

    void RebuildCache(Component::TileMapComponent& map) {
        if (!map.isCacheDirty) return;

        // 1. Clear Visual Cache
        map.renderCache.clear();

        // 2. Clear/Destroy Old Physics Entities
        // We destroy all old tile colliders and rebuild them. 
        auto* ecs = ECS::getInstance();
        for (auto const& [pos, entityID] : map.colliderCache) {
            if (ecs->HasComponent<Component::ColliderComponent>(entityID)) {
                ecs->DestroyEntity(entityID);
            }
        }
        map.colliderCache.clear();

        // Iterate over every painted tile
        for (auto const& [pos, tileAssetName] : map.gridData) {

            // A. Get the Asset
            TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(tileAssetName);
            if (!asset) continue;

            // B. Calculate Bitmask & Find Rule
            unsigned int currentMask = CalculateBitMask(map, pos.first, pos.second, tileAssetName);

            const RuleTile* matchedRule = nullptr;

            // Find match
            for (const RuleTile& rule : asset->GetTiles()) {
                if (rule.ruleBitMap == currentMask) {
                    matchedRule = &rule;
                    break;
                }
            }

            // Fallback
            if (!matchedRule && !asset->GetTiles().empty()) {
                matchedRule = &asset->GetTiles()[0];
            }

            if (!matchedRule) continue;

            // --- C. HANDLE VISUALS ---
            GLuint handle = Component::TextureRenderer::GetTextureHandle(matchedRule->texAssetName);
            if (handle != 0) map.renderCache[pos] = handle;

            // --- D. HANDLE PHYSICS (The Entity part) ---

            Entity colEnt = ecs->CreateEntity();

            // 1. Name it for debugging
            std::string name = "TileCol_" + std::to_string(pos.first) + "_" + std::to_string(pos.second);
            ecs->AddComponent<Component::NameComponent>(colEnt, name);

            // 2. Transform (Place it in world space)
            Component::TransformComponent& t = ecs->AddComponent<Component::TransformComponent>(colEnt);
            t.position.x = (float)pos.first * map.tileSize;
            t.position.y = (float)pos.second * map.tileSize;
            t.position.z = 0.1f; // Slight Z offset
            t.size = CEVec2((float)map.tileSize, (float)map.tileSize);

            // 3. Collider
            Component::ColliderComponent& c = ecs->AddComponent<Component::ColliderComponent>(colEnt);
            Collision::ColliderFactory factory;
            Collision::ColliderFactory::ArgMap args;

            // AABB Logic
            if (matchedRule->colliderType == Collision::ColliderType::AABB) {
                // Box needs min/max relative to center or corner. 
                // If your physics engine uses Min/Max relative to position:
                args["minX"] = std::to_string(0);
                args["minY"] = std::to_string(0);
                args["maxX"] = std::to_string(map.tileSize);
                args["maxY"] = std::to_string(map.tileSize);

                // Or if your physics engine assumes Center Position + HalfExtents, adjust accordingly.
            }
            // Add OBB/Sphere logic if needed...

            c.collider = factory.CreateCollider(matchedRule->colliderType);

            // 4. Store in Cache so we can delete it later
            map.colliderCache[pos] = colEnt;

            // 5. Parent it (Optional, if your hierarchy system supports it)
            // CrashoutsEngine::AttachChild(ecs, parentEntity, colEnt);
        }

        std::cout << "TileMapComponent: Cache Rebuilt. Colliders: " << map.colliderCache.size() << "\n";
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
                std::string path = CrashoutsEngine::Assets::rootPath + "tilemaps/";

                if (fs::exists(path)) {
                    for (const auto& entry : fs::directory_iterator(path)) {
                        std::string filename = entry.path().filename().string();
                        if (ImGui::Selectable(filename.c_str())) {
                            tmComp->activeAsset = filename;
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

            ImGui::End();
        }
    }

};

namespace CrashoutsEngine::Rendering {

    void RenderTileMaps(ECS& ecs) {
        // Get all TileMapComponents
        auto& tmArray = ecs.GetComponentArray<Component::TileMapComponent>();

        for (Component::TileMapComponent& tm : tmArray.dense) {

            // 1. Update Cache if needed (Handles Logic/Physics entities)
            if (tm.isCacheDirty) {
                TileMapUtils::RebuildCache(tm);
            }

            // 2. Setup shared drawing variables
            CEVec2 tileSize = { (float)tm.tileSize, (float)tm.tileSize };
            CEVec3 tint = { 1.0f, 1.0f, 1.0f }; // White (no tint)
            CEVec2 uvSize = { 1.0f, 1.0f };     // Full texture
            float rotation = 0.0f;

            // 3. Iterate the Visual Cache and Draw
            for (auto const& [pos, textureID] : tm.renderCache) {

                // Calculate World Position based on Grid Coordinate
                // Assuming Grid (0,0) is at World (0,0)
                float x = (float)pos.first * tm.tileSize + (tm.tileSize * 0.5f);
                float y = (float)pos.second * tm.tileSize + (tm.tileSize * 0.5f);

                // Z-Index: You might want a variable for this in the component
                float z = 0.0f;

                CEVec3 worldPos = { x, y, z };

                // 4. Call the OVERLOADED draw function
                Component::Shapes::CreateTexturedRectangle(
                    tileSize,       // Size
                    textureID,      // GLuint Handle (Fast!)
                    worldPos,       // Position
                    rotation,       // Rotation
                    tint,           // Color
                    uvSize,         // UVs
                    true            // isWorldSpace (Yes, tiles exist in the world)
                );
            }
        }
    }

}
