/*!
@file   tilemapasset.cpp
@author lee.r@digipen.edu
@date   16/09/2025

@brief	This file contains the definition of the tile map asset class, as well
        as the functions required to create it and use it.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */
#include "engine_pch.h"

#include "rendering.h"
#include "textureasset.h"

#include "assetmanager.h"
#include "tilemapasset.h"

// File I/O Util Functions
namespace {
    /*!
    @brief Reads the entire content of a file into a single string.
    @param filePath Path to the SceneAsset(JSON) file.
    @return empty string on failure.
    */
    inline std::string ReadFileToString(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cout << "ERROR::SCENE::FILE_NOT_READABLE: " << filePath << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }

    /*!
    @brief Writes content string to a file
    @param filePath Path to the SceneAsset(JSON) file.
    @param string containing data to send
    @return bool to determine if successful
    */
    inline bool WriteTextToFile(const std::string& filePath, const std::string& content) {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cout << "ERROR: Could not open file for writing: " << filePath << std::endl;
            return false;
        }
        file << content;
        return true;
    }
}

namespace CrashoutsEngine {
    namespace Assets {

        /*!
        @brief Creates a new TileMapAsset from a json file.
        @param filePath Path to the TileMapAsset(JSON) file.
        @return Unique pointer to a valid TileMapAsset, or nullptr on failure.
        */
        std::unique_ptr<TileMapAsset> TileMapAsset::Create(const std::string& filePath) {

            std::string normalizedRelativePath;

            // Check if the input already starts with "tilemaps" (handles both / and \)
            if (filePath.find("tilemaps/") == 0 || filePath.find("tilemaps\\") == 0) {
                normalizedRelativePath = filePath; // Input was already "tilemaps/level1.json"
            }
            else {
                normalizedRelativePath = "tilemaps/" + filePath; // Input was just "level1.json"
            }

            // Construct full OS path
            std::string fullpath = CrashoutsEngine::Assets::rootPath + normalizedRelativePath;

            // Load the JSON file from filePath
            std::string jsonContent = ReadFileToString(fullpath);
            if (jsonContent.empty()) {
                std::cout << "TileMapAsset::Create - File not found or empty: " << fullpath << std::endl;
                std::cout << "Creating a new empty tile map file..." << std::endl;

                // Define empty tile map content.
                const std::string emptyTileMapContent = "{\n  \"tilemap\": []\n}";

                if (!WriteTextToFile(fullpath, emptyTileMapContent)) {
                    // If we can't even write the new file, then we must fail
                    std::cout << "ERROR: Failed to write new tile map file: " << fullpath << std::endl;
                    return nullptr;
                }

                // Set the jsonContent to our new default string so the rest of the function can parse it.
                jsonContent = emptyTileMapContent;
            }

            // Parse JSON string into RapidJSON document
            rapidjson::Document doc;
            doc.Parse(jsonContent.c_str());

            if (doc.HasParseError()) {
                std::cout << "TileMapAsset::Create - Failed to parse JSON in " << filePath << std::endl;
                std::cout << "ERROR: " << rapidjson::GetParseError_En(doc.GetParseError())
                    << " at offset " << doc.GetErrorOffset() << std::endl;
                return nullptr;
            }

            if (!doc.IsObject()) {
                std::cout << "TileMapAsset::Create - JSON root is not an object in " << filePath << std::endl;
                return nullptr;
            }

            // 3. Create the Asset Object using the NORMALIZED relative path
            std::unique_ptr<TileMapAsset> newAsset(new TileMapAsset(normalizedRelativePath));

            if (doc.HasMember("layerMask") && doc["layerMask"].IsUint()) {
                newAsset->GetLayerMaskMutable() = doc["layerMask"].GetUint();
            }

            // 4. Populate Data 
            if (doc.HasMember("tilemap") && doc["tilemap"].IsArray()) {
                const auto& arr = doc["tilemap"];
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
                    const auto& obj = arr[i];
                    if (!obj.IsObject()) continue;

                    RuleTile ruleTile;

                    if (obj.HasMember("texAssetName") && obj["texAssetName"].IsString()) {
                        ruleTile.texAssetName = obj["texAssetName"].GetString();
                    }

                    if (obj.HasMember("colliderType") && obj["colliderType"].IsString()) {
                        std::string colliderType = obj["colliderType"].GetString();

                        if (colliderType == "ColliderType::AABB") ruleTile.colliderType = Collision::ColliderType::AABB;
                        else if (colliderType == "ColliderType::CIRCLE") ruleTile.colliderType = Collision::ColliderType::CIRCLE;
                        else if (colliderType == "ColliderType::OBB") ruleTile.colliderType = Collision::ColliderType::OBB;
                        else if (colliderType == "ColliderType::NONE") ruleTile.colliderType = Collision::ColliderType::NONE;
                    }

                    if (obj.HasMember("ruleBitMap") && obj["ruleBitMap"].IsUint()) {
                        ruleTile.ruleBitMap = obj["ruleBitMap"].GetUint();
                    }

                    // Store in the map
                    newAsset->tiles.push_back(std::move(ruleTile));
                }
            }

            std::cout << "Loaded TileMapAsset: " << filePath << " (" << newAsset->tiles.size() << " definitions)\n";
            return newAsset;
        }
    
        void TileMapAsset::SaveTileMaps() {
            std::string fullpath = CrashoutsEngine::Assets::rootPath + tileMapName;

            if (tileMapName.empty()) {
                std::cout << "ERROR: Cannot save TileMapAsset. No name found." << std::endl;
                return;
            }

            // 1. Setup RapidJSON Document
            rapidjson::Document doc;
            doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
            doc.AddMember("layerMask", layerMask, allocator);

            // 2. Create the "tilemap" array
            rapidjson::Value tileArray(rapidjson::kArrayType);

            // 3. Iterate over the map and create objects
            for (const RuleTile& ruleTile : tiles) {
                rapidjson::Value tileObj(rapidjson::kObjectType);

                // Add "texAssetName name"
                rapidjson::Value nameVal;
                nameVal.SetString(ruleTile.texAssetName.c_str(), static_cast<rapidjson::SizeType>(ruleTile.texAssetName.length()), allocator);
                tileObj.AddMember("texAssetName", nameVal, allocator);

                // Add "ColliderType"
                std::string colliderString{ "ColliderType::AABB" };
                if (ruleTile.colliderType == Collision::ColliderType::CIRCLE) {
                    colliderString = "ColliderType::CIRCLE";
                }
                else if (ruleTile.colliderType == Collision::ColliderType::OBB) {
                    colliderString = "ColliderType::OBB";
                }
                else if (ruleTile.colliderType == Collision::ColliderType::NONE) {
                    colliderString = "ColliderType::NONE";
                }

                rapidjson::Value colVal;
                colVal.SetString(colliderString.c_str(), static_cast<rapidjson::SizeType>(colliderString.length()), allocator);
                tileObj.AddMember("colliderType", colVal, allocator);

                // Add "Rule BITMAP"
                tileObj.AddMember("ruleBitMap", ruleTile.ruleBitMap, allocator);

                tileArray.PushBack(tileObj, allocator);
            }

            // 4. Add array to root object
            doc.AddMember("tilemap", tileArray, allocator);

            // 5. Serialize to String
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer); // Use PrettyWriter for readable formatting
            doc.Accept(writer);

            // 6. Write to Disk
            if (WriteTextToFile(fullpath, buffer.GetString())) {
                std::cout << "Saved TileMapAsset to: " << fullpath << "\n";
            }
            else {
                std::cout << "ERROR: Failed to write TileMapAsset to: " << fullpath << "\n";
            }
        }
    
    }
}
