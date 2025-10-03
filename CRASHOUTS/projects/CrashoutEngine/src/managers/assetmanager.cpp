/*!
@file   assetmanager.cpp
@author lee.r@digipen.edu
@date   28/09/2025

@brief	This file contains
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <iostream>
#include <fstream>
#include <filesystem>

#include "managers/assetmanager.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"

// Include Asset Types if required
#include "resources/textureasset.h"
#include "resources/sceneasset.h"

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {

	namespace Assets {

		namespace fs = std::filesystem;

		// Function to load assets in a json
		/*!
		@brief Scans asset directories and generates a JSON manifest file.

		@param rootAssetPath The path to the main 'assets' folder.
		@param outputManifestPath The full path where the 'assets.json' file will be saved.
		*/
		void GenerateAssetManifest(const std::string& rootAssetPath,
			const std::string& outputManifestPath) {
			rapidjson::Document doc;		
			doc.SetObject();														// Init as JSON object
			rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();		// Allocator required for memory management

			if (!fs::exists(rootAssetPath)) {
				std::cerr << "Error: Root asset path does not exist: " << rootAssetPath << std::endl;
				return;
			}

			// Iterate through subdirectories in the root asset path (e.g. "images", "audio", "font") 
			for (const auto& entry : fs::directory_iterator(rootAssetPath)) {
				if (entry.is_directory()) {
					// The directory name is the asset type
					std::string assetType = entry.path().filename().string();
					// JSON array created to hold all assets of this assetType
					rapidjson::Value assetArray(rapidjson::kArrayType);		

					// Iterate through all files in the asset type directory
					for (const auto& file : fs::recursive_directory_iterator(entry.path())) {
						if (file.is_regular_file()) {
							rapidjson::Value assetObject(rapidjson::kObjectType);

							// Asset name is the filename without extension
							std::string assetName = file.path().stem().string();

							// Asset path is relative to the type directory (e.g. "images/player.png")
							fs::path relativePath = fs::relative(file.path(), entry.path());

							assetObject.AddMember("name", rapidjson::Value(assetName.c_str(), allocator).Move(), allocator);
							assetObject.AddMember("path", rapidjson::Value(relativePath.string().c_str(), allocator).Move(), allocator);

							assetArray.PushBack(assetObject, allocator);
						}
					}

					// Add the array to the main JSON document with the asset type as the key
					doc.AddMember(rapidjson::Value(assetType.c_str(), allocator).Move(), assetArray, allocator);
				}
			}
		
			// Write the JSON document to the output file
			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);

			std::ofstream ofs(outputManifestPath);
			if (!ofs.is_open()) {
				std::cerr << "Error: Could not open output file for writing: " << outputManifestPath << std::endl;
				return;
			}

			ofs << buffer.GetString();
			ofs.close();

			std::cout << "Successfully generated asset manifest at: " << outputManifestPath << std::endl;
		}
	
		void AssetManager::LoadManifestAssets(const std::string& rootAssetPath,
			const std::string& manifestPath) {
			// Check manifest json file
			std::ifstream ifs(manifestPath);
			if (!ifs.is_open()) {
				std::cerr << "Error: Could not open manifest file: " << manifestPath << std::endl;
				return;
			}

			// Convert file to rapidJSON stream interface
			rapidjson::IStreamWrapper isw(ifs);
			rapidjson::Document doc;
			doc.ParseStream(isw);

			if (doc.HasParseError()) {
				std::cerr << "Error parsing manifest JSON." << std::endl;
				return;
			}

			// --- Loading IMAGES ---
			if (doc.HasMember("images") && doc["images"].IsArray()) {
				std::cout << "Loading Textures..." << std::endl;
				// Get reference to array of data under "images"
				const rapidjson::Value& imageArray = doc["images"];

				for (rapidjson::SizeType i = 0; i < imageArray.Size(); ++i) {
					const rapidjson::Value& asset = imageArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();

						// Full path is asset root path + "images/" + relativepath
						std::string fullPath = rootAssetPath + "images/" + relativePath;

						// 1. Create the TextureAsset object from file data
						std::unique_ptr<TextureAsset> newTexture = TextureAsset::Create(fullPath);

						// Validate
						if (newTexture) {
							// 2. Upload to GPU
							newTexture->UploadToGPU();

							// 3. Load the TextureAsset asset into the AssetManager
							AssetManager::GetInstance().Load(name, std::move(newTexture));

							// Optional: Preload handle into TextureRenderer Cache (IDK HOW)
							std::cout << "AssetManager loaded texture: " << name << "(" << fullPath << ")" << std::endl;
						}
					}
				}
			}

			// --- Loading AUDIO ----

			// --- Loading SCENES ---
			if (doc.HasMember("scenes") && doc["scenes"].IsArray()) {
				std::cout << "Loading Scenes..." << std::endl;

				const rapidjson::Value& sceneArray = doc["scenes"];

				for (rapidjson::SizeType i = 0; i < sceneArray.Size(); ++i) {
					const rapidjson::Value& asset = sceneArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();

						fs::path fullPath = fs::path(rootAssetPath) / "scenes" / relativePath;

						// Create SceneAsset object from file data (JSON)
						std::unique_ptr<SceneAsset> newScene = SceneAsset::Create(fullPath.string());

						// Validate 
						if (newScene) {
							// Load the SceneAsset into AssetManager
							AssetManager::GetInstance().Load(name, std::move(newScene));
							std::cout << "AssetManager loaded scene data: " << name << "(" << fullPath.string() << ")"
								<< std::endl;
						}
						else {
							std::cerr << "Failed to create SceneAsset: " << name << std::endl;
						}
					}
					else {
						std::cerr << "Warning: Scene asset entry missing 'name' or 'path' in manifest." << std::endl;
					}
				}
			}
		}
	}
}
