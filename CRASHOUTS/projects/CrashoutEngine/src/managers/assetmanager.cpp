/*!
@file   assetmanager.cpp
@author lee.r@digipen.edu
@date   28/09/2025

@brief	This file contains
//__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <fstream>
#include <filesystem>

#include "managers/assetmanager.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

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
	}
}
