/*!
@file   tilemapasset.h
@author Renfred Lee
@date   13/11/2025

@brief  This file contains the declaration of the Tile Map Asset which stores 
        the asset name, as well as the key data which is a map of index to 
        texture name.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

#ifndef TILEMAPASSET_H
#define TILEMAPASSET_H

// LIBRARIES INCLUDES
#include <rapidjson/document.h>
#include <string>

#include "assetmanager.h"
#include "colliders.h"

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

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
        /** @brief RuleTile */
        struct RuleTile {
            std::string texAssetName;
            Collision::ColliderType colliderType{ Collision::ColliderType::AABB };
            unsigned int ruleBitMap{ 0b0000'0000 };
        };

        /** @brief TileMapAsset */
        class TileMapAsset : public IAsset {
        public:
            // Default Constructor
            TileMapAsset() = default;

            // Custom Constructor 
            TileMapAsset(const std::string& tileMapName) :
                tileMapName(tileMapName){ }

            // Deleted copy operations
            TileMapAsset(const TileMapAsset&) = delete;
            TileMapAsset& operator=(const TileMapAsset&) = delete;

            // Move constructor and assignment
            TileMapAsset(TileMapAsset&& other) noexcept :
                tileMapName{std::move(other.tileMapName)},
                tiles {std::move(other.tiles)}
            {};

            TileMapAsset& operator=(TileMapAsset&& other) noexcept 
            {
                if (this != &other) {
                    tileMapName = std::move(other.tileMapName);
                    tiles = std::move(other.tiles);
					other.tileMapName.clear();
                    other.tiles.clear();
                }

                return *this;
            }

			// Default Destructor
            ~TileMapAsset() = default;

            // Public member functions
            // GETTERS
			const std::string& GetTileMapName() const { return tileMapName; }
            const std::vector<RuleTile>& GetTiles() const { return tiles; }
            std::vector<RuleTile>& GetTilesMutable() { return tiles; }
            const unsigned int& GetLayerMask() { return layerMask; }
            unsigned int& GetTagMaskMutable() { return layerMask; }

            // SETTERS
            /*!
            @brief Adds a new rule tile. 
            @param ruleTile RuleTile to reference and add
            */
            void AddRuleTile(const RuleTile& ruleTile) {
                tiles.push_back(ruleTile);
                SaveTileMaps();
            }

            /*!
            @brief Removes rule tile
            @param id The ID of the tile which user wants removed.
            */
            void RemoveTileDefinition(unsigned int id) {
                if (id >= tiles.size()) return;

                std::swap(tiles[id], tiles.back());
                tiles.pop_back();

                SaveTileMaps();
            }

            /*!
            @brief Factory method to create a new Asset from file
            @param filePath Path to the TileMapAsset file to load
            @return unique_ptr to TileMapAsset object or nullptr on failure
            */
            static std::unique_ptr<TileMapAsset> Create(const std::string& filePath);

            /*!
            @brief Saves the tile map tiles data
            */
            void SaveTileMaps();
            
        private:
			std::string tileMapName;        // Name of the tilemap
            unsigned int layerMask = 0;
            std::vector<RuleTile> tiles;    // Vector of RuleTile s            
        };
    }
}

#endif // TileMapAsset_H