/*!
@file   tilemapcomponent.cpp
@author lee.r@digipen.edu
@date   19/11/2025

@brief	This file defines the functions that would be used by the 
		tilemapcomponent.
__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "rendering.h"

#include "engine_pch.h"

#include "tilemapcomponent.h"
#include "assetmanager.h"
#include "tilemapasset.h"
#include "textureasset.h"

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine::TileMapUtils {

    using namespace CrashoutsEngine::Assets;

    void AddTileDefinitionToAsset(Component::TileMapComponent& map, const std::string& textureName) {
        // 1. Get the Asset
        TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(map.tmName);

        if (asset) {
            // 2. Modify the Asset (The Palette)
            unsigned int newId = asset->AddTileDefinition(textureName);

            std::cout << "TileMapComponent: Added definition '" << textureName
                << "' as ID " << newId << " to asset '" << map.tmName << "'\n";

            // 3. Mark Cache Dirty
            // This ensures that on the next frame, RebuildCache() will run,
            // finding the new texture name and fetching its GL handle.
            map.isCacheDirty = true;
        }
    }

    void RemoveTileDefinitionFromAsset(Component::TileMapComponent& map, unsigned int tileId) {
        TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(map.tmName);

        if (asset) {
            // 1. Remove from Asset
            asset->RemoveTileDefinition(tileId);

            // 2. Mark Dirty so the cache clears this ID's GL handle
            map.isCacheDirty = true;
        }
    }

    void RebuildCache(Component::TileMapComponent& map) {
        TileMapAsset* asset = AssetManager::GetInstance().Get<TileMapAsset>(map.tmName);

        if (!asset) return;

        map.runtimeTextureCache.clear();

        // Iterate over asset definitions and fetch GPU handles
        for (auto const& [id, texName] : asset->GetTiles()) {
            // Use TextureRenderer::GetTextureHandle to return a GLuint
            GLuint handle = Component::TextureRenderer::GetTextureHandle(texName);

            if (handle != 0) {
                map.runtimeTextureCache[id] = handle;
            }
        }

        std::cout << "TileMapComponent: Cache Rebuilt.\n";
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
        if (ImGui::Begin("TileMap Editor", pOpen)) {

            // --- Toolbar (Palette Selection) ---
            static unsigned int selectedBrushID = 1;
            ImGui::Text("Current Brush ID: %d", selectedBrushID);
            ImGui::SameLine();

            std::map<unsigned int, GLuint> const& texCache = tmComp->runtimeTextureCache;

            // Iterate through tmComp->runtimeTextureCache to draw selectable
            // For now, just a simple slider to pick ID
            int tmpID = (int)selectedBrushID;
            if (ImGui::SliderInt("Brush", &tmpID, 0, (int)texCache.size() - 1)) selectedBrushID = tmpID;

            // --- Setup Canvas ---
            // Calculate aspect ratio of the map
            float mapAR = 1.f;

            // Get available space in the window
            ImVec2 available = ImGui::GetContentRegionAvail();

            // Calculate Canvas Size (Fit within available space while keeping AR)
            ImVec2 canvasSize = available;
            if (available.x / available.y > mapAR) {
                // Too wide, constraint by height
                canvasSize.x = available.y * mapAR;
            }
            else {
                // Too tall, constraint by width
                canvasSize.y = available.x * mapAR;
            }

            // Center the canvas in the window 
            ImVec2 cursorStart = ImGui::GetCursorScreenPos();
            float offsetX = (available.x - canvasSize.x) * .5f;
            float offsetY = (available.y - canvasSize.y) * .5f;
            ImVec2 canvasStart{ cursorStart.x + offsetX, cursorStart.y + offsetY };
            ImVec2 canvasEnd{ cursorStart.x + canvasSize.x, cursorStart.y + canvasSize.y };

            // ImGui Draw List allows us to draw primitives (rects, lines, images)
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // --- Draw Background ---
            ImU32 bgCol = IM_COL32(40, 40, 40, 255);
            ImU32 borderCol = IM_COL32(200, 200, 200, 255);
            drawList->AddRectFilled(canvasStart, canvasEnd, bgCol);         // Draw Background
            drawList->AddRectFilled(canvasStart, canvasEnd, borderCol);     // Draw Border

            // --- Draw the tiles ---
            // Calculate the pixel size of a single tile in this view
            float drawTileW = canvasSize.x / (float)tmComp->tileSize;
            float drawTileH = canvasSize.y / (float)tmComp->tileSize;

            int height = (int)(canvasEnd.y - canvasStart.y);
            int width  = (int)(canvasEnd.x - canvasStart.x);

            for (int row = 0; row < height; ++row) {
                for (int col = 0; row < width; ++col) {
                    int index = row * width + col;
                    if (index >= tmComp->tileIds.size()) break;             // Texture index of bounds

                    unsigned int tileId = tmComp->tileIds[index];
                    if (tileId == 0) continue;                              // Skip empty

                    // Calculate Screen Position for this tile rect
                    ImVec2 pMin = {
                        canvasStart.x + (col * drawTileW),
                        canvasStart.y + (row * drawTileH)
                    };
                    ImVec2 pMax = {
                        pMin.x + drawTileW,
                        pMin.y + drawTileH
                    };

                    // Retrieve Texture
                    if (tmComp->runtimeTextureCache.count(tileId)) {
                        GLuint texHandle = tmComp->runtimeTextureCache.at(tileId);
                        drawList->AddImage((void*)(intptr_t)texHandle, pMin, pMax);
                    }
                    else {
                        // Missing Texture Fallback (Pink Square)
                        drawList->AddRectFilled(pMin, pMax, IM_COL32(255, 0, 255, 255));
                    }
                }
            }

            // --- Draw Grid Lines ---
            ImU32 gridColor = IM_COL32(255, 255, 255, 50);                  // Faint White
            for (int i = 0; i <= width; ++i) {
                float x = canvasSize.x + (i * drawTileW);
                drawList->AddLine({ x, canvasStart.y }, { x, canvasEnd.y }, gridColor);
            }
            for (int i = 0; i <= height; ++i) {
                float y = canvasSize.y + (i * drawTileH);
                drawList->AddLine({ canvasStart.x, y }, { canvasEnd.x, y }, gridColor);
            }

            // --- Handle Input (Painting) ---
            ImGui::SetCursorScreenPos(canvasStart);

            // Using InvisibleButton to detect clicks within stated area.
            if (ImGui::InvisibleButton("##MapCanvas", canvasSize)) {
                // Just clicked (single frame)
            }

            // Check if hovered and mouse is down (allows dragging / painting)
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();

                // Convert Screen Space Mouse -> Grid Coords
                int col = (int)((mousePos.x - canvasStart.x) / drawTileW);
                int row = (int)((mousePos.y - canvasStart.y) / drawTileH);

                // Boundary Check
                if (col >= 0 && col < width && row >= 0 && row < height) {
                    int index = row * width + col;

                    // Only update if different to avoid spamming flags
                    if (tmComp->tileIds[index] != selectedBrushID) {
                        tmComp->tileIds[index] = selectedBrushID;
                        tmComp->isCacheDirty = true;
                    }
                }
            }
            ImGui::End();
        }
    }

};