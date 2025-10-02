/*!
@file   sceneasset.h
@author lee.r@digipen.edu       
=@date   16/09/2025

@brief	This file contains the declaration of the Scene class, a rough implementation
        of asset type that encapsulates a rapidjson value data. The class
        is designed to be used with the AssetManager and provides a static factory
        method for safe creation and resource management.
//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */

#ifndef SCENEASSET_H
#define SCENEASSET_H

// LIBRARIES INCLUDES
#include <rapidjson/document.h>
#include <string>

#include "managers/assetmanager.h"

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

        class SceneAsset : public IAsset {
        public:

            // Public member functions
            // GETTERS
			rapidjson::Document& GetSceneData() { return sceneData; }
			const std::string& GetSceneName() const { return sceneName; }
			const std::string& GetSceneFilePath() const { return sceneFilePath; }

            // SETTERS
            bool SaveSceneData(const std::string& sceneName);

            // Constructor (matching implementation pattern)
            SceneAsset() : sceneData{}, sceneName{}, sceneFilePath{} {}
            
        private:
			rapidjson::Document sceneData; // RapidJSON document to hold scene data
			std::string sceneName;         // Name of the scene
			std::string sceneFilePath;     // File path to the scene file

        };
    }
}

#endif // SCENEASSET_H