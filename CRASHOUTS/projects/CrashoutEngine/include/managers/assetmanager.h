/*!
@file   resourcemanager.h
@author lee.r@digipen.edu
@date   15/09/2025

@brief	This file contains the declaration of the AssetManager, a singleton
		class responsible for managing and providing access to various game assets.
		It uses a two-map approach for efficient asset retrieval by name.
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {
		
	namespace Assets {

		const std::string rootPath = "../../../projects/CrashoutEngine/assets/";
		const std::string assetManifestPath = rootPath + "json/assetmanifest.json";
		
		// Function to load assets in a json
		/*!
		@brief Scans asset directories and generates a JSON manifest file.

		@param rootAssetPath The path to the main 'assets' folder.
		@param outputManifestPath The full path where the 'assets.json' file will be saved.
		*/
		void GenerateAssetManifest(const std::string& rootAssetPath = rootPath,
			const std::string& outputManifestPath = assetManifestPath);

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

			void LoadManifestAssets(const std::string& rootAssetPath = rootPath,
				const std::string& manifestPath = assetManifestPath);

			/*!
			@brief Loads and stores a new asset.

			This function takes a unique_ptr to an IAsset and stores it
			in the manager, associating it with a unique ID and a string name.
			The ownership of the asset is transferred to the manager via std::move.

			@param name The name to associate with the asset.
			@param asset The unique_ptr to the asset object to be stored.
			*/
			void Load(const std::string& name, std::unique_ptr<IAsset> asset) {
				if (Exists(name)) return;

				// Use a static counter to generate a unique ID
				// static unsigned int nextId = 0;
				unsigned int id = nextAssetID;
				 
				// Set the name of the asset
				asset->name = name;

				// Store the name-to-ID mapping
				assetNames[name] = id;

				// Store the unique_ptr in the main map
				// Use unique_ptr for automatic memory management
				assets[id] = std::move(asset);

				// Increment nextId
				nextAssetID++;
			}

			/*!
			@brief Retrieves an asset by its name and type.

			This templated function performs a two-step lookup to efficiently retrieve
			a specific asset. It first finds the asset's unique ID using its name,
			then uses that ID to access the asset pointer. It then attempts to
			safely cast the pointer to the specified type.

			@tparam T The type of the asset to retrieve (e.g., Texture, Model).

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

		private:
			// Private construct for singleton
			AssetManager() = default;
			~AssetManager() = default;
			AssetManager(const AssetManager&) = delete;
			AssetManager& operator=(const AssetManager&) = delete;

			std::unordered_map<unsigned int, std::unique_ptr<IAsset>> assets;
			std::unordered_map<std::string, unsigned int> assetNames;

			unsigned int nextAssetID = 0;
		};		
	}
}