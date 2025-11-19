/*!
@file   tilemapasset.cpp
@author lee.r@digipen.edu
@date   16/09/2025

@brief	This file contains the definition of the tile map asset class, as well
        as the functions required to create it and use it.
__________________________________________________________________________*/
#pragma once

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "engine_pch.h"

#include "assetmanager.h"
#include "tilemapasset.h"
#include "textureasset.h"
#include "rendering.h"

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

            std::string fullpath = CrashoutsEngine::Assets::rootPath + "tilemaps/" + filePath;

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

            // 3. Create the Asset Object
            // Use 'filePath' (the name) for the asset name, store 'fullpath' for saving later
            std::unique_ptr<TileMapAsset> newAsset(new TileMapAsset(filePath));

            // 4. Populate Data 
            if (doc.HasMember("tilemap") && doc["tilemap"].IsArray()) {
                const auto& arr = doc["tilemap"];
                for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
                    const auto& obj = arr[i];
                    if (obj.HasMember("id") && obj["id"].IsUint() &&
                        obj.HasMember("texture name") && obj["texture name"].IsString()) {

                        unsigned int id = obj["id"].GetUint();
                        std::string tex = obj["texture name"].GetString();

                        // Store in the map
                        newAsset->tiles[id] = tex;
                    }
                }
            }

            std::cout << "Loaded TileMapAsset: " << filePath << " (" << newAsset->tiles.size() << " definitions)\n";
            return newAsset;
        }
    
        void TileMapAsset::SaveTileMaps() {
            std::string fullpath = CrashoutsEngine::Assets::rootPath + "tilemaps/" + tileMapName;

            if (tileMapName.empty()) {
                std::cout << "ERROR: Cannot save TileMapAsset. No name found." << std::endl;
                return;
            }

            // 1. Setup RapidJSON Document
            rapidjson::Document doc;
            doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

            // 2. Create the "tilemap" array
            rapidjson::Value tileArray(rapidjson::kArrayType);

            // 3. Iterate over the map and create objects
            for (const auto& [id, texName] : tiles) {
                rapidjson::Value tileObj(rapidjson::kObjectType);

                // Add "id"
                tileObj.AddMember("id", id, allocator);

                // Add "texture name"
                rapidjson::Value nameVal;
                nameVal.SetString(texName.c_str(), static_cast<rapidjson::SizeType>(texName.length()), allocator);
                tileObj.AddMember("texture name", nameVal, allocator);

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
