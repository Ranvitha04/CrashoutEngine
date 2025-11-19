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
#include "undomanager.h"
#include <filesystem>

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

        map.isCacheDirty = false;
    }

    void ResizeMap(Component::TileMapComponent& map, int newRows, int newCols) {
        // 1. Create a new vector of the correct size, filled with 0
        std::vector<unsigned int> newTileIds(newRows * newCols, 0);

        // 2. Copy old data to new vector (preserving 2D structure)
        int rowsToCopy = std::min(map.row, newRows);
        int colsToCopy = std::min(map.col, newCols);

        for (int r = 0; r < rowsToCopy; ++r) {
            for (int c = 0; c < colsToCopy; ++c) {
                int oldIndex = r * map.col + c;
                int newIndex = r * newCols + c;

                // Copy the tile ID
                if (oldIndex < map.tileIds.size()) {
                    newTileIds[newIndex] = map.tileIds[oldIndex];
                }
            }
        }

        // 3. Update Component
        map.tileIds = std::move(newTileIds);
        map.row = newRows;
        map.col = newCols;
        map.isCacheDirty = true; // Force render update
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
            static unsigned int selectedBrushID = 0;
            ImGui::Text("Current Brush ID: %d", selectedBrushID);
            //ImGui::SameLine();

            std::map<unsigned int, GLuint> const& texCache = tmComp->runtimeTextureCache;

            // Iterate through tmComp->runtimeTextureCache to draw selectable
            // For now, just a simple slider to pick ID
            int tmpID = (int)selectedBrushID;
            int limit = (int)texCache.size();
            if (ImGui::SliderInt("Brush", &tmpID, 0, (limit < 0) ? 0 : limit)) selectedBrushID = tmpID;

            // Determine if the current mode is painting in world (TRUE) or tile map editor (FALSE)
            static bool isPaintingWorld{ false };

            if (ImGui::Checkbox("Paint in World", &isPaintingWorld)) {}

            //ImGui::SameLine();
            if (ImGui::SmallButton("Add Texture...")) {
                ImGui::OpenPopup("Pick Texture");
            }

            // Modal popup with a simple file browser
            if (ImGui::BeginPopupModal("Pick Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                namespace fs = std::filesystem;
                static std::vector<std::string> s_assets;
                static bool s_scanned = false;

                // Refresh List button
                if (ImGui::Button("Refresh List")) {
                    s_scanned = false;
                }

                ImGui::SameLine();

                if (!s_scanned) {
                    s_scanned = true;
                    s_assets.clear();

                    const std::string assetRoot = CrashoutsEngine::Assets::rootPath + "images"; // base folder
                    for (auto& p : fs::recursive_directory_iterator(assetRoot)) {
                        if (!p.is_regular_file()) continue;

                        const std::string ext = p.path().extension().string();
                        std::string lower = ext;
                        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                        if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
                            lower == ".tga" || lower == ".bmp") {

                            // Store relative path for cleaner display
                            std::string rel = fs::relative(p.path(), assetRoot).string();
                            s_assets.push_back(rel);
                        }
                    }

                    std::sort(s_assets.begin(), s_assets.end());
                }

                // Left: list; Right: preview name
                ImGui::BeginChild("asset_list", ImVec2(420, 300), true);
                for (auto const& f : s_assets) {
                    if (ImGui::Selectable(f.c_str())) {
                        TileMapUtils::AddTileDefinitionToAsset(*tmComp, f);
                        /*
                        UndoManager::GetInstance().AddCommand(
                            std::make_unique<CrashoutsEngine::ChangeValueCommand<std::string>>(
                                oldTexName, sc->textureName,
                                [sc](const std::string& v) { sc->textureName = v; }
                            )
                        );
                        */
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                }
                ImGui::EndChild();

                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            // Clear tile map
            if (ImGui::Button("Clear Tile Map")) {
                if (tmComp->tileIds.size() > 0) {
                    tmComp->tileIds.assign(tmComp->tileIds.size(), 0);
                }
            }

            // --- Setup Canvas ---
            // Calculate Map Aspect Ratio (Width / Height)
            // Guard against zero division
            float rows = (float)tmComp->row;
            float cols = (float)tmComp->col;
            if (rows == 0 || cols == 0) { ImGui::End(); return; }

            float mapAR = cols / rows;

            // Get available space in the window
            ImVec2 available = ImGui::GetContentRegionAvail();

            // Calculate Canvas Size (Fit within available space while keeping AR)
            ImVec2 canvasSize = available;
            float availAR = available.x / available.y;
            if (availAR > mapAR) {
                // Window is wider than map -> Constrain by Height
                canvasSize.y = available.y;
                canvasSize.x = available.y * mapAR;
            }
            else {
                // Window is taller than map -> Constrain by Width
                canvasSize.x = available.x;
                canvasSize.y = available.x / mapAR;
            }

            // Center the canvas in the window 
            float offsetX = (available.x - canvasSize.x) * 0.5f;
            float offsetY = (available.y - canvasSize.y) * 0.5f;

            offsetX = std::max(offsetX, 0.0f);
            offsetY = std::max(offsetY, 0.0f);

            // Move cursor to centered origin
            ImVec2 canvasStart = ImGui::GetCursorScreenPos();
            canvasStart.x += offsetX;
            canvasStart.y += offsetY;

            // Reserve canvas space
            ImGui::Dummy(canvasSize);

            // Compute canvas end
            ImVec2 canvasEnd = { canvasStart.x + canvasSize.x, canvasStart.y + canvasSize.y };

            // ImGui Draw List allows us to draw primitives (rects, lines, images)
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // --- Draw Background ---
            ImU32 bgCol = IM_COL32(20, 20, 20, 255);
            ImU32 borderCol = IM_COL32(200, 200, 200, 255);
            drawList->AddRectFilled(canvasStart, canvasEnd, bgCol);         // Draw Background
            drawList->AddRect(canvasStart, canvasEnd, borderCol);     // Draw Border

            // --- Draw the tiles ---
            // Calculate the pixel size of a single tile in this view
            float tileW = canvasSize.x / cols;
            float tileH = canvasSize.y / rows;

            for (int r = 0; r < tmComp->row; ++r) {
                for (int c = 0; c < tmComp->col; ++c) {
                    int index = r * tmComp->col + c;
                    if (index >= tmComp->tileIds.size()) break;

                    unsigned int tileId = tmComp->tileIds[index];
                    if (tileId == 0) continue; 

                    ImVec2 pMin = { canvasStart.x + c * tileW, canvasStart.y + r * tileH };
                    ImVec2 pMax = { pMin.x + tileW, pMin.y + tileH };

                    if (texCache.count(tileId)) {
                        drawList->AddImage((void*)(intptr_t)texCache.at(tileId), pMin, pMax);
                    } else {
                        drawList->AddRectFilled(pMin, pMax, IM_COL32(255, 0, 255, 255));        // Error Square (Pink)
                    }
                }
            }

            // --- Draw Grid ---
            ImU32 gridCol = IM_COL32(255, 255, 255, 40);
            // Vertical lines
            for (int i = 0; i <= tmComp->col; ++i) {
                float x = canvasStart.x + (i * tileW); // FIX: Use canvasStart.x
                drawList->AddLine({ x, canvasStart.y }, { x, canvasEnd.y }, gridCol);
            }
            // Horizontal lines
            for (int i = 0; i <= tmComp->row; ++i) {
                float y = canvasStart.y + (i * tileH); // FIX: Use canvasStart.y
                drawList->AddLine({ canvasStart.x, y }, { canvasEnd.x, y }, gridCol);
            }

            // Enable Painting
            if (texCache.size() > 0) {

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
                    int col = (int)((mousePos.x - canvasStart.x) / tileW);
                    int row = (int)((mousePos.y - canvasStart.y) / tileH);

                    // Boundary Check
                    if (col >= 0 && col < tmComp->col && row >= 0 && row < tmComp->row) {
                        int index = row * tmComp->col + col;

                        // Only update if different to avoid spamming flags

                        // If not painting in world, painting on tile map editor overrides the current item
                        if (!isPaintingWorld) {
                            if (tmComp->tileIds[index] != selectedBrushID) {
                                tmComp->tileIds[index] = selectedBrushID;
                                tmComp->isCacheDirty = true;
                            }
                        }
                        // If painting in world, selecting from tile map editor changes the current brush ID
                        else {
                            selectedBrushID = tmComp->tileIds[index];
                            std::cout << "Current Brush is " << selectedBrushID << std::endl;
                        }
                    }
                }
            }

            ImGui::End();
        }
    }

};