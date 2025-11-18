/*!
@file   assetmanager.cpp
@author lee.r@digipen.edu
@date   28/09/2025

@brief	This file implements an Asset Manager
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <iostream>
#include <fstream>
#include <filesystem>

#include "assetmanager.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"

// Include Asset Types if required
#include "textureasset.h"
#include "sceneasset.h"
#include "fontasset.h"
#include "shaderasset.h"
#include "audioSystem.h"

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {

	namespace Assets {

		namespace fs = std::filesystem;

		AssetManager::AssetManager() {
			// Load FT_Library
			if (FT_Init_FreeType(&ftLibrary)) {
				std::cerr << "ERROR::FREETYPE: Could not init FreeType library" << std::endl;
			}
		}

		AssetManager::~AssetManager() {
			// Delete FT_Library
			//FT_Done_FreeType(ftLibrary);
		}

		// Function to load assets in a json
		/*!
		@brief Scans asset directories and generates a JSON manifest file.

		@param rootAssetPath The path to the main 'assets' folder.
		@param outputManifestPath The full path where the 'assets.json' file will be saved.
		*/
		void GenerateAssetManifest(const std::string& rootAssetPath, const std::string& outputManifestPath) {
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

					// Special Handling for shaders
					if (assetType == "shaders") {
						// Iterate through all shader directories
						for (const auto& shaderDir : fs::directory_iterator(entry.path())) {
							if (shaderDir.is_directory()) {
								rapidjson::Value assetObject(rapidjson::kObjectType);

								// Asset name is the directory name
								std::string shaderName = shaderDir.path().filename().string();

								// Asset path is relative to the shaders directory
								fs::path vertPath = shaderDir.path() / (shaderName + ".vert");
								fs::path fragPath = shaderDir.path() / (shaderName + ".frag");

								if (fs::exists(vertPath) && fs::exists(fragPath)) {
									assetObject.AddMember("name", rapidjson::Value(shaderName.c_str(), allocator).Move(), allocator);
									// Path is just the directory name, matching LoadManifestAssets expectation
									assetObject.AddMember("path", rapidjson::Value(shaderName.c_str(), allocator).Move(), allocator);
									assetArray.PushBack(assetObject, allocator);
								}
								else {
									std::cerr << "Warning: Shader directory '" << shaderName
										<< "' is missing .vert or .frag file. Skipping." << std::endl;
								}
							}
						}
					}
					else {
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
					}

					// Add the array to the main JSON document with the asset type as the key
					if(!assetArray.Empty()) doc.AddMember(rapidjson::Value(assetType.c_str(), allocator).Move(), assetArray, allocator);
				}
			}
		
			// Write the JSON document to the output file
			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);
		
			// NEW: ensure output directory exists (e.g., assets/json)
			fs::path outPath(outputManifestPath);
			fs::path outDir = outPath.parent_path();
			if (!fs::exists(outDir)) {
			    try {
			        fs::create_directories(outDir);
			    } catch (const std::exception& e) {
			        std::cerr << "Error: Could not create manifest directory: " << outDir.string() << " - " << e.what() << std::endl;
			        return;
			    }
			}
		
			std::ofstream ofs(outPath.string());
			if (!ofs.is_open()) {
			    std::cerr << "Error: Could not open output file for writing: " << outPath.string() << std::endl;
			    return;
			}
			ofs << buffer.GetString();
			ofs.close();
			std::cout << "Successfully generated asset manifest at: " << outPath.string() << std::endl;
		}
	
		
		void AssetManager::LoadManifestAssets(const std::string& rootAssetPath, const std::string& manifestPath) {
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
				const rapidjson::Value& imageArray = doc["images"];
				for (rapidjson::SizeType i = 0; i < imageArray.Size(); ++i) {
					const rapidjson::Value& asset = imageArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();
						std::string fullPath = rootAssetPath + "images/" + relativePath;
						std::unique_ptr<TextureAsset> newTexture = TextureAsset::Create(fullPath);
						if (newTexture) {
							newTexture->UploadToGPU();
							AssetManager::GetInstance().Load(name, std::move(newTexture));
							std::cout << "AssetManager loaded texture: " << name << "(" << fullPath << ")" << std::endl;
						}
					}
				}
			}
			
			// NEW: --- Loading SPRITE_SHEET ---
			if (doc.HasMember("sprite_sheet") && doc["sprite_sheet"].IsArray()) {
				std::cout << "Loading Sprite Sheet Textures..." << std::endl;
				const rapidjson::Value& sheetArray = doc["sprite_sheet"];
				for (rapidjson::SizeType i = 0; i < sheetArray.Size(); ++i) {
					const rapidjson::Value& asset = sheetArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();
						std::string fullPath = rootAssetPath + "sprite_sheet/" + relativePath;
						std::unique_ptr<TextureAsset> newTexture = TextureAsset::Create(fullPath);
						if (newTexture) {
							newTexture->UploadToGPU();
							AssetManager::GetInstance().Load(name, std::move(newTexture));
							std::cout << "AssetManager loaded sprite sheet: " << name << "(" << fullPath << ")" << std::endl;
						}
					}
				}
			}

			// --- Loading AUDIO ----
			if (doc.HasMember("audio") && doc["audio"].IsArray()) {
				std::cout << "Loading Audio Banks..." << std::endl;

				// Point FMOD to the *root audio folder* once (e.g. assets/audio)
				CrashoutsEngine::Audio::AudioSystem::instance().init(rootAssetPath + "audio");

				const rapidjson::Value& audioArray = doc["audio"];

				for (rapidjson::SizeType i = 0; i < audioArray.Size(); ++i) {
					if (!audioArray[i].HasMember("path")) continue;
					std::string rel = audioArray[i]["path"].GetString();
					if (std::filesystem::path(rel).extension() != ".bank") continue;

					if (rel.find("Master.strings.bank") != std::string::npos) {
						CrashoutsEngine::Audio::AudioSystem::instance().loadBank(rel);
						std::cout << "Loaded FMOD bank: " << rel << std::endl;
					}
				}
				for (rapidjson::SizeType i = 0; i < audioArray.Size(); ++i) {
					if (!audioArray[i].HasMember("path")) continue;
					std::string rel = audioArray[i]["path"].GetString();
					if (std::filesystem::path(rel).extension() != ".bank") continue;

					if (rel.find("Master.bank") != std::string::npos &&
						rel.find("Master.strings.bank") == std::string::npos) {
						CrashoutsEngine::Audio::AudioSystem::instance().loadBank(rel);
						std::cout << "Loaded FMOD bank: " << rel << std::endl;
					}
				}
				for (rapidjson::SizeType i = 0; i < audioArray.Size(); ++i) {
					if (!audioArray[i].HasMember("path")) continue;
					std::string rel = audioArray[i]["path"].GetString();
					if (std::filesystem::path(rel).extension() != ".bank") continue;

					if (rel.find("Master.bank") == std::string::npos) {
						bool ok = CrashoutsEngine::Audio::AudioSystem::instance().loadBank(rel);
						std::cout << (ok ? "Loaded FMOD bank: " : "FAILED to load FMOD bank: ") << rel << std::endl;
					}
				}
			}

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
		
			// --- Loading FONTS ----
			if (doc.HasMember("fonts") && doc["fonts"].IsArray()) {
				std::cout << "Loading Fonts..." << std::endl;
				
				const rapidjson::Value& fontArray = doc["fonts"];

				for (rapidjson::SizeType i = 0; i < fontArray.Size(); ++i) {
					const rapidjson::Value& asset = fontArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();

						fs::path fullPath = fs::path(rootAssetPath) / "fonts" / relativePath;

						// Create FontAsset object from file data (JSON)
						std::unique_ptr<FontAsset> newFont = FontAsset::Create(fullPath.string());

						// Validate 
						if (newFont) {
							// Load the FontAsset into AssetManager
							AssetManager::GetInstance().Load(name, std::move(newFont));
							std::cout << "AssetManager loaded font data: " << name << "(" << fullPath.string() << ")"
								<< std::endl;
						}
						else {
							std::cerr << "Failed to create FontAsset: " << name << std::endl;
						}
					}
					else {
						std::cerr << "Warning: Font asset entry missing 'name' or 'path' in manifest." << std::endl;
					}
				}
			}
		
			// --- Loading Shader ---
			if (doc.HasMember("shaders") && doc["shaders"].IsArray()) {
				std::cout << "Loading Shaders..." << std::endl;

				const rapidjson::Value& shaderArray = doc["shaders"];

				for (rapidjson::SizeType i = 0; i < shaderArray.Size(); ++i) {
					const rapidjson::Value& asset = shaderArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();

						fs::path fullPath = fs::path(rootAssetPath) / "shaders" / relativePath;
						fs::path vertPath = fullPath / (relativePath + ".vert");
						fs::path fragPath = fullPath / (relativePath + ".frag");

						// Create ShaderAsset object from file data (JSON)
						std::unique_ptr<ShaderAsset> newShader = ShaderAsset::Create(vertPath.string(), fragPath.string());

						// Validate 
						if (newShader) {
							// Load the FontAsset into AssetManager
							AssetManager::GetInstance().Load(name, std::move(newShader));
							std::cout << "AssetManager loaded shader data: " << name << "(" << fullPath.string() << ")"
								<< std::endl;
						}
						else {
							std::cerr << "Failed to create ShaderAsset: " << name << std::endl;
						}
					}
					else {
						std::cerr << "Warning: Shader asset entry missing 'name' or 'path' in manifest." << std::endl;
					}
				}
			}
		}
	}
}
