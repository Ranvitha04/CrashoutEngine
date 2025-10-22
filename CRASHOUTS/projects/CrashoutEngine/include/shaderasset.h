///*!
//@file   fontasset.h
//@author lee.r@digipen.edu       
//@date   19/10/2025
//
//@brief	This file contains the declaration of the Font
////__________________________________________________________________________*/
////#pragma once
////
/////*                                                                   includes
////----------------------------------------------------------------------------- */
//
//#ifndef FONTASSET_H
//#define FONTASSET_H
//
//// LIBRARIES INCLUDES
//#include <string>
//
//#include "assetmanager.h"
//
//// Forward declarations
//namespace CrashoutsEngine {
//    namespace Assets {
//
//        class FontAsset : public IAsset {
//        public:
//            // Default Constructor
//            FontAsset() = default;
//
//            // Custom Constructor 
//            FontAsset(const std::string& fontName) :
//                fontName(fontName) {
//            }
//
//            // Deleted copy operations
//            SceneAsset(const SceneAsset&) = delete;
//            SceneAsset& operator=(const SceneAsset&) = delete;
//
//            // Move constructor and assignment
//            SceneAsset(SceneAsset&& other) noexcept :
//                sceneName{std::move(other.sceneName)}
//            {};
//
//            SceneAsset& operator=(SceneAsset&& other) noexcept 
//            {
//                if (this != &other) {
//                    sceneName = std::move(other.sceneName);
//					other.sceneName.clear();
//                }
//            }
//
//			// Default Destructor
//            ~SceneAsset() = default;
//
//            // Public member functions
//            // GETTERS
//			const std::string& GetFontName() const { return sceneName; }
//
//            //void SetSceneData(rapidjson::Document&& newDoc);
//
//            /*!
//            @brief Factory method to create a new Asset from file
//            @param filePath Path to the SceneAsset file to load
//            @return unique_ptr to SceneAsset object or nullptr on failure
//            */
//            static std::unique_ptr<SceneAsset> Create(const std::string& filePath);
//            
//        private:
//			std::string fontName;         // Name of the scene
//        };
//    }
//
//    namespace Serialization {
//        void LoadFont(std::string const& fontName);
//    }
//}
//
//#endif // SCENEASSET_H