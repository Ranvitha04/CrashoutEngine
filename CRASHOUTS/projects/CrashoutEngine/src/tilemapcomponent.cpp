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
#include "tilemapcomponent.h"
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
            Up          = 0b0000'0001,
            UpRight     = 0b0000'0010,
            Right       = 0b0000'0100,
            DownRight   = 0b0000'1000,
            Down        = 0b0001'0000,
            DownLeft    = 0b0010'0000,
            Left        = 0b0100'0000,
            UpLeft      = 0b1000'0000
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

        map.renderCache.clear();

        // Iterate over every painted tile in the sparse grid
        for (auto const& [pos, tileAssetName] : map.gridData) {

            // 1. Get the Asset fopr this file
            TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(tileAssetName);
            if (!asset) continue;   // Asset not found?

            // 2. Calculate the environment bitmask (Look at neighbours)
            unsigned int currentMask = CalculateBitMask(map, pos.first, pos.second, tileAssetName);

            // 3. Find the mathcign texture in the Asset's Rule List
            std::string texToUse{ "" };
        
            // Find first tile that matches the mask
            for (const RuleTile& rule : asset->GetTiles()) {
                if (rule.ruleBitMap == currentMask) {
                    texToUse = rule.texAssetName;
                    break;  // Found! Break early.
                }
            }

            // FALLBACK: No rules found
            if (texToUse.empty() && !asset->GetTiles().empty()) {
                texToUse = asset->GetTiles()[0].texAssetName;
            }

            // 4. Get the GPU Handle and store in Render Cache
            if (!texToUse.empty()) {
                GLuint handle = Component::TextureRenderer::GetTextureHandle(texToUse);
                if (handle != 0) map.renderCache[pos] = handle;
            }

            std::cout << "TileMapComponent: Sparse Grid Cache Rebuilt.\n";
            map.isCacheDirty = false;
        }

        std::cout << "TileMapComponent: Cache Rebuilt.\n";

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