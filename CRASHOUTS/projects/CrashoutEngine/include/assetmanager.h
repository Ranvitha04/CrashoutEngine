/*!
@file   assetmanager.h
@author lee.r@digipen.edu
@date   15/09/2025

@brief	This file contains the declaration of the AssetManager, a singleton
		class responsible for managing and providing access to various game assets.
		It uses a two-map approach for efficient asset retrieval by name.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>
#include <ft2build.h>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <iostream>

#include FT_FREETYPE_H

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {
		
	namespace Assets {

		extern std::string coreAssetPath;
		extern std::string gameAssetPath;
		extern std::string assetManifestPath;

		std::string ResolvePath(const std::string& relativePath);

		enum class AssetType {
			Texture,
			SpriteSheet,
			Audio,
			Scene,
			Font,
			Shader,
			TileMap
		};

		struct LoadJob {
			std::string name;
			std::string path;
			AssetType type;
		};

		// Function to load assets in a json
		/*!
		@brief Scans asset directories and generates a JSON manifest file.

		@param rootAssetPath The path to the main 'assets' folder.
		@param outputManifestPath The full path where the 'assets.json' file will be saved.
		*/
		void GenerateAssetManifest(const std::string& outputManifestPath = assetManifestPath);

		enum class AssetState {
			Unloaded,
			Loading,
			Loaded,
			Failed
		};

		/*!
		@brief Interface class for all assets.

		This serves as the base class for all asset types (e.g., textures,
		models, sounds). It provides a common interface for the AssetManager
		to handle them uniformly.
		*/
		class IAsset {
		public: 
			/*!
			@brief Virtual destructor.

			Ensures that derived asset classes are properly destroyed when
			their unique_ptr goes out of scope.
			*/
			virtual ~IAsset() = default;

			/*!
			@brief Gets the name of the asset.

			@return A constant reference to the asset's name.
			*/
			std::string GetName() const { return name; }

			std::string GetPath() const { return path; }
			bool IsCore() const { return isCore; }

			/*!
			@brief Gets the current state of the asset.
			@return The asset's state.
			*/
			AssetState GetState() const { return state; }

			/*!
			@brief Sets the state of the asset.
			@param newState The new state to set.
			*/
			void SetState(AssetState newState) { state = newState; }

			/*!
			@brief Checks if the asset is fully loaded and ready for use.
			@return True if loaded, false otherwise.
			*/
			bool IsReady() const { return state == AssetState::Loaded; }

		protected:
			AssetState state = AssetState::Unloaded;
			std::string path = "";   // The actual file path
			bool isCore = false;     // True if loaded from core-assets

		private:
			std::string name;
			friend class AssetManager;	// Allows AssetManager access to name
		};

		/*!
		@brief Manages all game assets.

		This class implements the Singleton pattern to ensure there is only one
		AssetManager instance. It stores assets using a two-map system for fast
		retrieval by name.
		*/
        class AssetManager {
        public:
			/*!
			@brief Gets the singleton instance of the AssetManager.

			The instance is created on the first call to this function.

			@return A reference to the single AssetManager instance.
			*/
            static AssetManager& GetInstance(){
                static AssetManager instance;
                return instance;
            }

			void LoadManifestAssets(const std::string& manifestPath = assetManifestPath);

			void LoadSingleAsset(const std::string& fullPath);

			/*!
			@brief Enqueues an asset for asynchronous loading.
			@param name The name to associate with the asset.
			@param path The path to the asset file.
			@param type The type of asset to load.
			*/
			void EnqueueLoad(const std::string& name, const std::string& path, AssetType type);

			/*!
			@brief Processes completed asset loads on the main thread.
			Should be called once per frame in the main loop.
			*/
			void Update();

			/*!
			@brief Loads and stores a new asset.

			This function takes a unique_ptr to an IAsset and stores it
			in the manager, associating it with a unique ID and a string name.
			The ownership of the asset is transferred to the manager via std::move.

			@param name The name to associate with the asset.
			@param asset The unique_ptr to the asset object to be stored.
			*/
			void Load(const std::string& name, std::unique_ptr<IAsset> asset, const std::string& filePath = "") {
				if (Exists(name)) {
					std::cout << "[AssetManager] Warning: Duplicate asset name: " << name << ". Skipping.\n";
					return;
				}

				unsigned int id = nextAssetID;
				asset->name = name;

				// --- SET THE PATH AND CORE FLAG ---
				if (!filePath.empty()) {
					asset->path = filePath;
					// If the path contains "core-assets", it's a permanent engine asset
					if (filePath.find("core-assets") != std::string::npos ||
						filePath.find("CrashoutEngine") != std::string::npos) { // Added extra check
						asset->isCore = true;
					}
				}
				// ----------------------------------

				assetNames[name] = id;
				if (asset->GetState() == AssetState::Unloaded) {
					asset->SetState(AssetState::Loaded);
				}
				assets[id] = std::move(asset);
				nextAssetID++;
			}

			/*!
			@brief Retrieves an asset by its name and type.

			This templated function performs a two-step lookup to efficiently retrieve
			a specific asset. It first finds the asset's unique ID using its name,
			then uses that ID to access the asset pointer. It then attempts to
			safely cast the pointer to the specified type.

			@tparam T The type of the asset to retrieve (e.g., TextureAsset, Model).

			@param name The name of the asset to find.

			@return A pointer to the requested asset if found and of the correct type,
			otherwise nullptr.
			*/
			template <typename T>
			T* Get(const std::string& name) {
				auto assetIdIt = assetNames.find(name);
				if (assetIdIt == assetNames.end()) {
					return nullptr;
				}

				unsigned int id = assetIdIt->second;

				auto it = assets.find(id);
				if (it != assets.end()) {
					return dynamic_cast<T*>(it->second.get());
				}
				return nullptr;
			}

			/*!
			@brief Checks if an asset with the given name already exists.

			@param name The name of the asset to check for.
			@return true if an asset with the name exists, false otherwise.
			*/
			bool Exists(const std::string& name) const {
				return assetNames.count(name) > 0;
			}

			/*!
			@brief Getter for ftLibrary
			@return Returns the FreeType library instance
			*/
            FT_Library GetFTLib() const{
                return ftLibrary;
            }

            void Shutdown();

        private:
            // Private construct for singleton
            AssetManager();
            ~AssetManager();
            AssetManager(const AssetManager&) = delete;
            AssetManager& operator=(const AssetManager&) = delete;

			void LoaderThreadLoop();

			std::unordered_map<unsigned int, std::unique_ptr<IAsset>> assets;
			std::unordered_map<std::string, unsigned int> assetNames;

			// Async loading members
			std::queue<LoadJob> loadQueue;
			std::queue<std::pair<std::string, std::unique_ptr<IAsset>>> completedQueue;
			std::mutex queueMutex;
			std::condition_variable condition;
			std::thread loaderThread;
			std::atomic<bool> exitThread{ false };

			// The single FreeType library instance (Only needs 1)
			FT_Library ftLibrary;

			unsigned int nextAssetID = 0;
		};		
	}
}