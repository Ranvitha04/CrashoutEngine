/*!
@file   sceneasset.h
@author lee.r@digipen.edu       
=@date   16/09/2025

@brief	This file contains the declaration of the Scene class, a rough implementation
        of asset type that encapsulates a rapidjson value data. The class
        is designed to be used with the AssetManager and provides a static factory
        method for safe creation and resource management.
All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
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

#include "assetmanager.h"

// Forward declarations
namespace CrashoutsEngine {
    namespace Assets {

        class SceneAsset : public IAsset {
        public:
            // Default Constructor
            SceneAsset() = default;

            // Custom Constructor 
            SceneAsset(const std::string& sceneName) :
                sceneName(sceneName) {
            }

            // Deleted copy operations
            SceneAsset(const SceneAsset&) = delete;
            SceneAsset& operator=(const SceneAsset&) = delete;

            // Move constructor and assignment
            SceneAsset(SceneAsset&& other) noexcept :
                sceneName{std::move(other.sceneName)}
            {};

            SceneAsset& operator=(SceneAsset&& other) noexcept 
            {
                if (this != &other) {
                    sceneName = std::move(other.sceneName);
					other.sceneName.clear();
                }
            }

			// Default Destructor
            ~SceneAsset() = default;

            // Public member functions
            // GETTERS
			const std::string& GetSceneName() const { return sceneName; }

            //void SetSceneData(rapidjson::Document&& newDoc);

            /*!
            @brief Factory method to create a new Asset from file
            @param filePath Path to the SceneAsset file to load
            @return unique_ptr to SceneAsset object or nullptr on failure
            */
            static std::unique_ptr<SceneAsset> Create(const std::string& filePath);
            
        private:
			std::string sceneName;         // Name of the scene
        };
    }

    namespace Serialization {
        extern std::string currSceneName;

        void LoadScenes(std::string const& sceneName);
        void SaveScenes(std::string const& sceneName);
    }
}

#endif // SCENEASSET_H