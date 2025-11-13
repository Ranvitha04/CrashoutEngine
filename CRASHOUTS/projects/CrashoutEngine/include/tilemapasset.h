/*!
@file   tilemapasset.h
@author lee.r@digipen.edu       
@date   13/11/2025

@brief	This file contains the declaration of the 

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
                tileMapName(tileMapName) {
            }

            // Deleted copy operations
            TileMapAsset(const TileMapAsset&) = delete;
            TileMapAsset& operator=(const TileMapAsset&) = delete;

            // Move constructor and assignment
            TileMapAsset(TileMapAsset&& other) noexcept :
                tileMapName{std::move(other.tileMapName)}
            {};

            TileMapAsset& operator=(TileMapAsset&& other) noexcept 
            {
                if (this != &other) {
                    tileMapName = std::move(other.tileMapName);
					other.tileMapName.clear();
                }
            }

			// Default Destructor
            ~TileMapAsset() = default;

            // Public member functions
            // GETTERS
			const std::string& GettileMapName() const { return tileMapName; }

            //void SetSceneData(rapidjson::Document&& newDoc);

            /*!
            @brief Factory method to create a new Asset from file
            @param filePath Path to the TileMapAsset file to load
            @return unique_ptr to TileMapAsset object or nullptr on failure
            */
            static std::unique_ptr<TileMapAsset> Create(const std::string& filePath);
            
        private:
			std::string tileMapName;         // Name of the tilemap
        };
    }
}

#endif // TileMapAsset_H