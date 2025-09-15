/*!
@file       resourcemanager.h
@author     lee.r@digipen.edu
@date       15/09/2025

This file implements
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <unordered_map>
#include <memory>
#include <string>

// Include components here:

/*
----------------------------------------------------------------------------- */
namespace CrashoutsEngine {
	
	namespace Assets {

		// Interface (base) class for all assets
		class IAsset {
		public: 
			virtual ~IAsset() = default;
		};

		// Templated class to hold specific asset types
		template <typename T>
		class AssetManager {
		public:
			// Singleton instance access
			static AssetManager& GetInstance(){
				static AssetManager instance;
				return instance;
			}

			// Load and store an asset
			void Load(const std::string& name, std::unique_ptr<T> asset) {
				// Use unique_ptr for automatic memory management
				assets[name] = std::move(asset);
			}

			// Get an asset by name

		private:
			// Private construct for singleton
			AssetManager() = default;
			~AssetManager() = default;
			AssetManager(const AssetManager&) = delete;
			AssetManager& operator=(const AssetManager&) = delete;

			std::unordered_map<std::string, std::unique_ptr<T>> assets;
		};
			
	}

}