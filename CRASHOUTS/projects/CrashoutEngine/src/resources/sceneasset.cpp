/*!
@file   sceneasset.cpp
@author lee.r@digipen.edu
=@date   16/09/2025

@brief	This file contains the definition of the Scene class, a rough implementation
        of asset type that encapsulates a rapidjson value data. The class
        is designed to be used with the AssetManager and provides a static factory
        method for safe creation and resource management.
//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */

#include "resources/sceneasset.h"

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

        /*!
        @brief Creates a new SceneAsset from a json file.
        @param filePath Path to the SceneAsset(JSON) file.
        @return Unique pointer to a valid TextureAsset, or nullptr on failure.
        */

        std::unique_ptr<SceneAsset> SceneAsset::Create(const std::string& filePath) {

			// Load the JSON file from filePath
            SceneAsset loadedData;

            if (!loadedData) {
                return nullptr;
            }

            return std::unique_ptr<SceneAsset>(new SceneAsset(loadedData));
        }

        bool SceneAsset::SaveSceneData(const std::string& sceneName) {
			// Read the rapidjson::document member variable and save it to a file
            // at main path + sceneName
        }
        
    }
}
