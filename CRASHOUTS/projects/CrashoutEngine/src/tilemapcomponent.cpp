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
#include "shapescomponent.h"
#include "namecomponent.h"
#include "assetmanager.h"
#include "tilemapasset.h"
#include "textureasset.h"
#include "hierarchy_components.h"
#include "hierarchy.h"
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
    static unsigned int CalculateBitMask(Component::TileMapComponent& map, int x, int y, const std::string& asset) {
        unsigned int mask = 0;

        // Define bitmask values for cardinal directions only
        enum {
            Up =    0b0001,     // 1
            Right = 0b0010,     // 2
            Down =  0b0100,     // 4
            Left =  0b1000      // 8
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

    void RebuildCache(Component::TileMapComponent& map, const Entity& parentEnt) {

        if (!map.isCacheDirty) return;

        // 1. Clear Visual Cache
        map.renderCache.clear();

        // 2. Clear/Destroy Old Physics Entities
        // We destroy all old tile colliders and rebuild them. 
        auto* ecs = ECS::getInstance();

        // Check if entity with tilemapcomponent has hierachy component
        if (!ecs->HasComponent<Component::ChildrenComponent>(parentEnt)) ecs->AddComponent<Component::ChildrenComponent>(parentEnt);

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

            // --- D. HANDLE PHYSICS (The Entity part) ---
            // Parent as needed

            Entity ent = ecs->CreateEntity();

            // 1. Name it for debugging
            std::string name = "Tile_" + std::to_string(pos.first) + "_" + std::to_string(pos.second);
            ecs->AddComponent<Component::NameComponent>(ent, name);
            
            // Add Sprite Component?
            Component::ShapeComponent s;
            s.enableTexture = true;
            s.textureName = matchedRule->texAssetName;
            s.type = Component::ShapeType::Rectangle;
            ecs->AddComponent<Component::ShapeComponent>(ent, std::move(s));
            
            // Add child component
            CrashoutsEngine::AttachChild(*ecs, parentEnt, ent);
            //AttachChild(*ecs, parentEnt, ent);

            // 2. Transform (Place it in world space)
            Component::TransformComponent& t = ecs->AddComponent<Component::TransformComponent>(ent);
            t.position.x = (float)pos.first * map.tileSize + map.tileSize * .5f;
            t.position.y = (float)pos.second * map.tileSize + map.tileSize * .5f;
            t.position.z = 0.1f; // Slight Z offset
            t.size = CEVec2((float)map.tileSize, (float)map.tileSize);

            // 3. Collider
            Component::ColliderComponent& c = ecs->AddComponent<Component::ColliderComponent>(ent);
            Collision::ColliderFactory factory;
            Collision::ColliderFactory::ArgMap args;

            //auto* a = static_cast<Collision::AABBRect*>(c.collider.get());
            //
            //CEVec3 extents;
            //extents.x = t.size.x * 0.5f;
            //extents.y = t.size.y * 0.5f;
            //extents.z = 0.f;
            //
            //a->min = t.position - extents;
            //a->max = t.position + extents;
            //a->extents = (a->max - a->min) * 0.5f;

            // 4. Store in Cache so we can delete it later
            map.colliderCache[pos] = ent;

            // 5. Parent it (Optional, if your hierarchy system supports it)
            // CrashoutsEngine::AttachChild(ecs, parentEntity, ent);
        }

        std::cout << "TileMapComponent: Cache Rebuilt. Colliders: " << map.colliderCache.size() << "\n";
        map.isCacheDirty = false;
    }
}

#include "inputmanager.h"

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

//namespace CrashoutsEngine::Rendering {
//
//    void RenderTileMaps(ECS& ecs) {
//        // Get all TileMapComponents
//        auto& tmArray = ecs.GetComponentArray<Component::TileMapComponent>();
//
//        for (size_t i = 0; i < tmArray.dense.size(); ++i) {
//
//            // 1. Get the Component
//            Component::TileMapComponent& tm = tmArray.dense[i];
//
//            // 2. Get the Entity ID associated with this index
//            Entity entityID = tmArray.indexToEntity[i];
//
//            // 1. Update Cache if needed (Handles Logic/Physics entities)
//            if (tm.isCacheDirty) {
//                TileMapUtils::RebuildCache(tm, entityID);
//            }
//
//            // 2. Setup shared drawing variables
//            CEVec2 tileSize = { (float)tm.tileSize, (float)tm.tileSize };
//            CEVec3 tint = { 1.0f, 1.0f, 1.0f }; // White (no tint)
//            CEVec2 uvSize = { 1.0f, 1.0f };     // Full texture
//            float rotation = 0.0f;
//
//            // 3. Iterate the Visual Cache and Draw
//            for (auto const& [pos, textureID] : tm.renderCache) {
//
//                // Calculate World Position based on Grid Coordinate
//                // Assuming Grid (0,0) is at World (0,0)
//                float x = (float)pos.first * tm.tileSize +(tm.tileSize * 0.5f);
//                float y = (float)pos.second * tm.tileSize +(tm.tileSize * 0.5f);
//
//                // Z-Index: You might want a variable for this in the component
//                float z = 0.0f;
//
//                CEVec3 worldPos = { x, y, z };
//
//                // 4. Call the OVERLOADED draw function
//                Component::Shapes::CreateTexturedRectangle(
//                    tileSize,       // Size
//                    textureID,      // GLuint Handle (Fast!)
//                    worldPos,       // Position
//                    rotation,       // Rotation
//                    tint,           // Color
//                    uvSize,         // UVs
//                    true            // isWorldSpace (Yes, tiles exist in the world)
//                );
//            }
//        }
//    }
//
//}
