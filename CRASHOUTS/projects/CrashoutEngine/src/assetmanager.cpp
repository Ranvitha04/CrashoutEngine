/*!
@file   assetmanager.cpp
@author lee.r@digipen.edu (95%)
@co-auther mohamadamirakmak.b (5%)
@date   28/09/2025

@brief	This file implements an Asset Manager
All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.
//__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_set>
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
#include "tilemapasset.h"
#include "audioSystem.h"

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {

	namespace Assets {

#ifdef USE_IMGUI_EDITOR
		const std::string rootPath = "../../projects/Descent/assets/";
#else	// GAME MODE
		const std::string rootPath = "assets/";
#endif

		const std::string assetManifestPath = rootPath + "json/assetmanifest.json";

		namespace fs = std::filesystem;
		static bool EndsWithJson(const std::string& s)
		{
			if (s.size() < 5) return false;
			return s.compare(s.size() - 5, 5, ".json") == 0;
		}

		static std::string NormalizeSceneRelPath(std::string p)
		{
			// Turn "descentmenu" into "descentmenu.json"
			if (!EndsWithJson(p))
				p += ".json";
			return p;
		}

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

		void AssetManager::Shutdown() {
			assets.clear();
			assetNames.clear();

			std::unordered_map<unsigned int, std::unique_ptr<IAsset>>().swap(assets);
			std::unordered_map<std::string, unsigned int>().swap(assetNames);

			nextAssetID = 0;
			
			if (ftLibrary) {
				FT_Done_FreeType(ftLibrary);
				ftLibrary = nullptr; 
			}
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
                            if (newTexture->UploadToGPU()) {
                                std::cout << "AssetManager loaded texture: " << name << "(" << fullPath << ") ["
                                          << newTexture->GetWidth() << "x" << newTexture->GetHeight() << "]" << std::endl;
                                AssetManager::GetInstance().Load(name, std::move(newTexture));
                            } else {
                                std::cerr << "FAILED to upload texture: " << name << " (" << fullPath << ")" << std::endl;
                            }
                        } else {
                            std::cerr << "FAILED to load texture: " << name << " (" << fullPath << ") — possibly exceeds 4096x4096 or unsupported format." << std::endl;
                        }
					}
				}
			}
			
			// NEW: --- Loading SPRITE_SHEET ---
			if (doc.HasMember("spritesheet") && doc["spritesheet"].IsArray()) {
				std::cout << "Loading Sprite Sheet Textures..." << std::endl;
				const rapidjson::Value& sheetArray = doc["sprite_sheet"];
				for (rapidjson::SizeType i = 0; i < sheetArray.Size(); ++i) {
					const rapidjson::Value& asset = sheetArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();
						std::string fullPath = rootAssetPath + "spritesheet/" + relativePath;
                        std::unique_ptr<TextureAsset> newTexture = TextureAsset::Create(fullPath);
                        if (newTexture) {
                            if (newTexture->UploadToGPU()) {
                                std::cout << "AssetManager loaded sprite sheet: " << name << "(" << fullPath << ") ["
                                          << newTexture->GetWidth() << "x" << newTexture->GetHeight() << "]" << std::endl;
                                AssetManager::GetInstance().Load(name, std::move(newTexture));
                            } else {
                                std::cerr << "FAILED to upload sprite sheet: " << name << " (" << fullPath << ")" << std::endl;
                            }
                        } else {
                            std::cerr << "FAILED to load sprite sheet: " << name << " (" << fullPath << ") — possibly exceeds 4096x4096 or unsupported format." << std::endl;
                        }
					}
				}
			}

			// --- Loading AUDIO ----
			if (doc.HasMember("audio") && doc["audio"].IsArray()) {
				using CrashoutsEngine::Audio::AudioSystem;
				namespace fs = std::filesystem;

				std::cout << "Loading audio (banks + raw)..." << std::endl;

				auto& audioSys = AudioSystem::instance();

				// Point FMOD to assets/audio once
				//audioSys.init(rootAssetPath + "audio");

				const auto& audioArray = doc["audio"];

				// Collect paths
				std::vector<std::string> bankPaths;
				std::vector<std::string> rawPaths;

				for (rapidjson::SizeType i = 0; i < audioArray.Size(); ++i) {
					if (!audioArray[i].HasMember("path"))
						continue;

					std::string rel = audioArray[i]["path"].GetString();
					fs::path p(rel);
					std::string ext = p.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (ext == ".bank") {
						bankPaths.push_back(rel);
					}
					else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
						rawPaths.push_back(rel);
					}
				}

				// --- Load banks in proper order ---

				// 1) Master.strings.bank
				for (const auto& rel : bankPaths) {
					if (rel.find("Master.strings.bank") != std::string::npos) {
						bool ok = audioSys.loadBank(rel);
						std::cout << (ok ? "Loaded FMOD bank: " : "FAILED to load FMOD bank: ") << rel << '\n';
					}
				}

				// 2) Master.bank (but not strings)
				for (const auto& rel : bankPaths) {
					if (rel.find("Master.bank") != std::string::npos &&
						rel.find("Master.strings.bank") == std::string::npos) {
						bool ok = audioSys.loadBank(rel);
						std::cout << (ok ? "Loaded FMOD bank: " : "FAILED to load FMOD bank: ") << rel << '\n';
					}
				}

				// 3) All other banks
				for (const auto& rel : bankPaths) {
					if (rel.find("Master.bank") == std::string::npos &&
						rel.find("Master.strings.bank") == std::string::npos) {
						bool ok = audioSys.loadBank(rel);
						std::cout << (ok ? "Loaded FMOD bank: " : "FAILED to load FMOD bank: ") << rel << '\n';
					}
				}

				// --- Register RAW audio files ---

				for (const auto& rel : rawPaths) {
					audioSys.registerRawAudio(rel); // rel is path relative to assets/audio
					std::cout << "[Manifest] Registered RAW audio: " << rel << '\n';
				}
			}

			// --- Loading SCENES ---
			if (doc.HasMember("scenes") && doc["scenes"].IsArray()) {
				std::cout << "Loading Scenes..." << std::endl;

				const rapidjson::Value& sceneArray = doc["scenes"];
				std::unordered_set<std::string> loadedSceneNames;
				loadedSceneNames.reserve(sceneArray.Size());
				for (rapidjson::SizeType i = 0; i < sceneArray.Size(); ++i) {
					const rapidjson::Value& asset = sceneArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();
						if (!loadedSceneNames.insert(name).second) {
							std::cout << "[AssetManager] Duplicate scene in manifest, skipping: " << name << "\n";
							continue;
						}
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
		
			// --- Loading TileMaps ---
			if (doc.HasMember("tilemaps") && doc["tilemaps"].IsArray()) {
				std::cout << "Loading TileMap Assets..." << std::endl;

				const rapidjson::Value& tilemapArray = doc["tilemaps"];

				for (rapidjson::SizeType i = 0; i < tilemapArray.Size(); ++i) {
					const rapidjson::Value& asset = tilemapArray[i];
					if (asset.HasMember("name") && asset.HasMember("path")) {
						std::string name = "tilemaps/";
						name += asset["name"].GetString();
						std::string relativePath = asset["path"].GetString();

						// Create TileMapAsset object from file data (JSON)
						std::unique_ptr<TileMapAsset> newTileMap = TileMapAsset::Create(relativePath);

						// Validate 
						if (newTileMap) {
							// Load the TileMapAsset into AssetManager
							AssetManager::GetInstance().Load(name, std::move(newTileMap));
							std::cout << "AssetManager loaded tilemap data: " << name << "(" << relativePath << ")"
								<< std::endl;
						}
						else {
							std::cerr << "Failed to create TileMapAsset: " << name << " at " << relativePath << std::endl;
						}
					}
					else {
						std::cerr << "Warning: TileMap asset entry missing 'name' or 'path' in manifest." << std::endl;
					}
				}
			}
		}

		/*!
		@brief Loads a single asset given its full file path.
		@param fullPath The full path to the asset file to be loaded.
		*/
		void AssetManager::LoadSingleAsset(const std::string& fullPath)
		{
			namespace fs = std::filesystem;

			fs::path p(fullPath);
			if (!fs::exists(p)) return;

			std::string name = p.stem().string();
			std::string ext = p.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			fs::path root(rootPath);
			fs::path rel;

			try {
				rel = fs::relative(p, root);
			}
			catch (...) {
				std::cerr << "SingleAsset: Path not under asset root: " << fullPath << "\n";
				return;
			}

			if (rel.empty() || rel.begin() == rel.end()) {
				std::cerr << "SingleAsset: Invalid relative path\n";
				return;
			}

			std::string folder = rel.begin()->string();

			// TEXTURES (images/)
			if (folder == "images")
			{
				if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
				{
					std::unique_ptr<TextureAsset> tex = TextureAsset::Create(fullPath);
					if (tex && tex->UploadToGPU()) {
						Load(name, std::move(tex));
						std::cout << "[SingleAsset] Loaded image: " << name << std::endl;
					}
				}
				return;
			}

			// SPRITE SHEET (sprite_sheet/)
			if (folder == "spritesheet")
			{
				if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
				{
					std::unique_ptr<TextureAsset> tex = TextureAsset::Create(fullPath);
					if (tex && tex->UploadToGPU()) {
						Load(name, std::move(tex));
						std::cout << "[SingleAsset] Loaded sprite sheet: " << name << std::endl;
					}
				}
				return;
			}

			// SCENES (scenes/)
			if (folder == "scenes")
			{
				if (ext == ".json" || ext == ".scene")
				{
					std::unique_ptr<SceneAsset> scene = SceneAsset::Create(fullPath);
					if (scene) {
						Load(name, std::move(scene));
						std::cout << "[SingleAsset] Loaded scene: " << name << std::endl;
					}
				}
				return;
			}

			// FONTS  (fonts/)
			if (folder == "fonts")
			{
				std::unique_ptr<FontAsset> font = FontAsset::Create(fullPath);
				if (font) {
					Load(name, std::move(font));
					std::cout << "[SingleAsset] Loaded font: " << name << std::endl;
				}
				return;
			}

			// AUDIO (audio/)
			if (folder == "audio")
			{
				// rel is something like "audio/Music/Menu.wav"
				std::string relStr = rel.generic_string();

				std::string extLow = ext;
				std::transform(extLow.begin(), extLow.end(), extLow.begin(), ::tolower);

				// BANKS -> load FMOD Studio bank
				if (extLow == ".bank")
				{
					bool ok = CrashoutsEngine::Audio::AudioSystem::instance().loadBank(relStr);
					std::cout << (ok ? "[SingleAsset] Loaded FMOD bank: ": "[SingleAsset] FAILED FMOD bank: ") << relStr << "\n";
				}
				// RAW AUDIO -> register only (no loadBank)
				else if (extLow == ".wav" || extLow == ".ogg" || extLow == ".mp3")
				{
					// Strip "audio/" so your panel & C# use "Music/Menu.wav" style
					std::string relAudio = relStr;
					const std::string prefix = "audio/";
					if (relAudio.rfind(prefix, 0) == 0)
						relAudio = relAudio.substr(prefix.size());

					CrashoutsEngine::Audio::AudioSystem::instance().registerRawAudio(relAudio);
					std::cout << "[SingleAsset] Registered RAW audio: " << relAudio << "\n";
				}

				return;
			}

			// SHADERS  (shaders/<name>/<name>.vert & .frag)
			if (folder == "shaders")
			{
				// Only load if either .vert or .frag was dropped
				if (ext == ".vert" || ext == ".frag")
				{
					fs::path shaderDir = p.parent_path();
					std::string shaderName = shaderDir.filename().string();

					fs::path vert = shaderDir / (shaderName + ".vert");
					fs::path frag = shaderDir / (shaderName + ".frag");

					if (fs::exists(vert) && fs::exists(frag))
					{
						std::unique_ptr<ShaderAsset> shader =
							ShaderAsset::Create(vert.string(), frag.string());

						if (shader)
						{
							Load(shaderName, std::move(shader));
							std::cout << "[SingleAsset] Loaded shader: " << shaderName << "\n";
						}
					}
				}
				return;
			}

			std::cout << "[SingleAsset] No loader for: " << fullPath
				<< " (folder=" << folder << ", ext=" << ext << ")\n";
		}
	}
}
