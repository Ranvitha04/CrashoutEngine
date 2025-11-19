/*!
@file   tilemapasset.h
@author lee.r@digipen.edu       
@date   13/11/2025

@brief	This file contains the declaration of the Tile Map Asset which stores 
        the asset name, as well as the key data which is a map of index to 
        texture name.

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

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

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
            }

			// Default Destructor
            ~TileMapAsset() = default;

            // Public member functions
            // GETTERS
			const std::string& GetTileMapName() const { return tileMapName; }
            const std::map<unsigned int, std::string>& GetTiles() const { return tiles; }

            // SETTERS
            /*!
            @brief Adds a new tile definition. Auto-generates an ID based on the highest existing ID.
            @param textureName The name of the texture.
            @return The ID assigned to this new tile.
            */
            unsigned int AddTileDefinition(const std::string& textureName) {
                unsigned int newId = 1; // Start at 1 (assuming 0 is reserved for empty)

                if (!tiles.empty()) {
                    // std::map is sorted. rbegin() points to the last element (highest key).
                    // We take that highest ID and add 1.
                    newId = tiles.rbegin()->first + 1;
                }

                tiles[newId] = textureName;
                SaveTileMaps();

                return newId; // Return it so the Editor/UI knows what ID was just created
            }

            /*!
            @brief Removes tile definition at id provided.
            @param id The ID of the tile which user wants removed.
            */
            void RemoveTileDefinition(unsigned int id) {
                tiles.erase(id);
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
			std::string tileMapName;                    // Name of the tilemap
            std::map<unsigned int, std::string> tiles;  // Map of id to texture name from texture asset
        };
    }
}

#endif // TileMapAsset_H