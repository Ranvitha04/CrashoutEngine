/*!
@file   sceneasset.cpp
@author lee.r@digipen.edu
=@date   16/09/2025

@brief	This file contains the definition of the Scene class, a rough implementation
        of asset type that encapsulates a rapidjson value data. The class
        is designed to be used with the AssetManager and provides a static factory
        method for safe creation and resource management.
//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */
#include "engine_pch.h"

#include "entitymanager.h"
#include "assetmanager.h"
#include "collisionmanager.h"
#include "sceneasset.h"
#include "CEVec3.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <utility>
#include <filesystem>

// Components
#include "namecomponent.h"
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "audiocomponent.h"
#include "textcomponent.h"
#include "scriptcomponent.h"
#include "cameracomponent.h"
#include "physicsmanager.h"
#include "ppcomponent.h"
#include "collidercomponent.h"
#include "layercomponent.h"
#include "tilemapcomponent.h"
#include "hierarchy_components.h"
#include "transientcomponent.h"

// FOR GAME GUI
#include "buttoncomponent.h"
#include "guicomponent.h"
#include "textinputcomponent.h"
#include "uilayoutcomponent.h"

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
        @brief Creates a new SceneAsset from a json file.
        @param filePath Path to the SceneAsset(JSON) file.
        @return Unique pointer to a valid SceneAsset, or nullptr on failure.
        */
        std::unique_ptr<SceneAsset> SceneAsset::Create(const std::string& filePath) {

            // Load the JSON file from filePath
            std::string jsonContent = ReadFileToString(filePath);
            if (jsonContent.empty()) {
                std::cout << "SceneAsset::Create - File not found or empty: " << filePath << std::endl;
                std::cout << "Creating a new empty scene file..." << std::endl;

                // Define empty scene content.
                const std::string emptySceneContent = "{\n  \"entities\": [],\n  \"layers\": {}\n}";

                if (!WriteTextToFile(filePath, emptySceneContent)) {
                    // If we can't even write the new file, then we must fail
                    std::cout << "ERROR: Failed to write new scene file: " << filePath << std::endl;
                    return nullptr;
                }

                // Set the jsonContent to our new default string so the rest of the function can parse it.
                jsonContent = emptySceneContent;
            }

            // Parse JSON string into RapidJSON document
            rapidjson::Document doc;
            doc.Parse(jsonContent.c_str());

            if (doc.HasParseError()) {
                std::cout << "SceneAsset::Create - Failed to parse JSON in " << filePath << std::endl;
                std::cout << "ERROR: " << rapidjson::GetParseError_En(doc.GetParseError())
                    << " at offset " << doc.GetErrorOffset() << std::endl;
                return nullptr;
            }

            if (!doc.IsObject()) {
                std::cout << "SceneAsset::Create - JSON root is not an object in " << filePath << std::endl;
                return nullptr;
            }

            // Extract the scene name by finding last slash
            size_t lastSlash = filePath.find_last_of("/\\");
            std::string sceneName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);

            return std::unique_ptr<SceneAsset>(new SceneAsset(sceneName));
        }

    }

    namespace Serialization {

        using namespace Component;

        std::string currSceneName = "";

        // JSON Structure Keys definition (for clean and centralized key management)
        namespace Keys {
            const char* const ENTITIES                  = "entities";
            const char* const ENTITY_ID                 = "id";
            const char* const NAME_COMPONENT            = "name";
            const char* const LAYER_COMPONENT           = "layer";
            const char* const TRANSFORM_COMPONENT       = "transform";
            const char* const RIGIDBODY_COMPONENT       = "rigidbody";
            const char* const COLLIDER_COMPONENT        = "collider";
            const char* const SHAPE_COMPONENT           = "shape";
            const char* const TEXT_COMPONENT            = "text";    
            const char* const AUDIO_COMPONENT           = "audio";  
            const char* const SCRIPT_COMPONENT          = "script";
            const char* const CAMERA_COMPONENT          = "camera";
            const char* const TILEMAP_COMPONENT          = "tilemap";
            // UI COMPONENT ITEMS
            const char* const BUTTON_COMPONENT          = "button";
            const char* const GUI_COMPONENT             = "gui";
            const char* const TEXT_INPUT_COMPONENT      = "textinput";
            const char* const UI_LAYOUT_COMPONENT       = "uilayout";
            // POST PROCESSING ITEMS
            const char* const PP_COMPONENT              = "postprocessing";
            // HIERACHY ITEMS
            const char* const PARENT_COMPONENT          = "parent";
        }

        /*!
        @brief Loads entities and their components from a specified scene asset into the ECS.
        @param sceneName The name of the scene asset to load (e.g., "level1.json").
        */
        void LoadScenes(std::string const& sceneName) {
            Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();

            Assets::SceneAsset* sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);

            if (!sceneAsset) {
                std::cout << "ERROR: Scene Asset '" << sceneName << "' not found." << std::endl;
                return;
            }

            // Cycle through sceneAsset sceneData (JSON document) for testing
            std::string fullPath = Assets::rootPath + "/scenes/" + sceneName;

            std::string jsonContent = ReadFileToString(fullPath);
            if (jsonContent.empty()) {
                std::cout << "ERROR: Failed to read scene file: " << fullPath << std::endl;
                return;
            }

            rapidjson::Document doc;
            doc.Parse(jsonContent.c_str());

            if (doc.HasParseError()) {
                std::cout << "ERROR: Failed to parse JSON for scene: " << fullPath << std::endl;
                std::cout << "Parse Error: " << rapidjson::GetParseError_En(doc.GetParseError())
                    << " at offset " << doc.GetErrorOffset() << std::endl;
                return;
            }

            // 5. Use the local document (replaces sceneAsset->GetSceneData())
            if (!doc.HasMember(Keys::ENTITIES) || !doc[Keys::ENTITIES].IsArray()) {
                std::cout << "Warning: Scene JSON has no '" << Keys::ENTITIES << "' array. Starting with empty scene." << std::endl;
                return;
            }


            if (doc.HasMember("layers")) {
                LayerComponent::layers.clear();
                const rapidjson::Value& layerNames = doc["layers"];
                if (layerNames.IsObject()) {
                    // iterate over all members (key-value pairs) in json object
                    for (rapidjson::Value::ConstMemberIterator itr = layerNames.MemberBegin();
                        itr != layerNames.MemberEnd(); ++itr) {
                        // Get the key
                        std::string layerName = itr->name.GetString();
                        // Get value
                        unsigned int layerVal = 0;
                        if (itr->value.IsUint()) layerVal = itr->value.GetUint();
                        // Insert the pair
                        LayerComponent::layers[layerName] = layerVal;
                    }
                }
            }

            const rapidjson::Value& entitiesArray = doc[Keys::ENTITIES];

            ECS& gECS = *ECS::getInstance();

            // Helper struct to link parents after all entities are created
            struct ParentLinkRequest {
                Entity childEntity;
                std::string parentName;
            };
            std::vector<ParentLinkRequest> parentLinksToProcess;

            for (rapidjson::SizeType i = 0; i < entitiesArray.Size(); ++i) {
                /*Utility*/
                struct ReadVectorFromJson {
                    CEVec3 operator()(const rapidjson::Value& val) {
                        CEVec3 newVal{};
                        if (val.HasMember("x")) newVal.x = val["x"].GetFloat();
                        if (val.HasMember("y")) newVal.y = val["y"].GetFloat();
                        if (val.HasMember("z")) newVal.z = val["z"].GetFloat();
                        return newVal;
                    }
                };

                // Create entity obj
                Entity newEntity = gECS.CreateEntity();
                // Add default components
                NameComponent& nameComp = gECS.AddComponent<NameComponent>(newEntity, ("Entity" + std::to_string(i)));
                LayerComponent& layerComp = gECS.AddComponent<LayerComponent>(newEntity);
                TransformComponent& transComp = gECS.AddComponent<TransformComponent>(newEntity);

                const rapidjson::Value& entityValue = entitiesArray[i];

                if (!entityValue.IsObject()) continue;

                // Set Entity Name
                if (entityValue.HasMember(Keys::NAME_COMPONENT) && entityValue[Keys::NAME_COMPONENT].IsString()) {
                    nameComp.name = entityValue[Keys::NAME_COMPONENT].GetString();
                }

                // Set Entity Layer
                if (entityValue.HasMember(Keys::LAYER_COMPONENT) && entityValue[Keys::LAYER_COMPONENT].IsUint()) {
                    layerComp.currentLayer = entityValue[Keys::LAYER_COMPONENT].GetUint();
                }

                // Set Entity Transform
                // { "transform": {rot}, {pos}, {scale} }
                if (entityValue.HasMember(Keys::TRANSFORM_COMPONENT) && entityValue[Keys::TRANSFORM_COMPONENT].IsObject()) {
                    const rapidjson::Value& transformValue = entityValue[Keys::TRANSFORM_COMPONENT];

                    // 1. Rotation (single float)
                    float rotation = 0.0f;
                    if (transformValue.HasMember("rotation") && transformValue["rotation"].IsFloat()) {
                        rotation = transformValue["rotation"].GetFloat();
                    }

                    // 2. Position (CEVec3)
                    CEVec3 pos{ 0.0f, 0.0f, 0.0f };
                    if (transformValue.HasMember("position") && transformValue["position"].IsObject()) {
                        const rapidjson::Value& posValue = transformValue["position"];
                        // Use HasMember check for safety
                        if (posValue.HasMember("x")) pos.x = posValue["x"].GetFloat();
                        if (posValue.HasMember("y")) pos.y = posValue["y"].GetFloat();
                        if (posValue.HasMember("z")) pos.z = posValue["z"].GetFloat();
                    }

                    // 3. size (CEVec2) - Corrected variable name and keys
                    CEVec2 size{ 1.0f, 1.0f }; // Initialize to default size
                    if (transformValue.HasMember("size") && transformValue["size"].IsObject()) {
                        const rapidjson::Value& sizeValue = transformValue["size"];
                        // Use HasMember check for safety
                        if (sizeValue.HasMember("x")) size.x = sizeValue["x"].GetFloat();
                        if (sizeValue.HasMember("y")) size.y = sizeValue["y"].GetFloat();
                    }

                    transComp.position = pos;
                    transComp.rotation = rotation;
                    transComp.size = size;
                }

                // Check if Rigid-Body
                if (entityValue.HasMember(Keys::RIGIDBODY_COMPONENT) && entityValue[Keys::RIGIDBODY_COMPONENT].IsObject()) {
                    const rapidjson::Value& rigidBodyValues = entityValue[Keys::RIGIDBODY_COMPONENT];

                    Component::RigidBodyComponent& rigidBodyComp{ gECS.AddComponent<RigidBodyComponent>(newEntity) };

                    if (rigidBodyValues.HasMember("maximumVelocity") && rigidBodyValues["maximumVelocity"].IsObject()) {
                        const rapidjson::Value& maximumVelVal{ rigidBodyValues["maximumVelocity"] };

                        CEVec3 maxVel{ ReadVectorFromJson{}(maximumVelVal)};
                        
                        rigidBodyComp.setMaxVelocity(maxVel);
                    }

                    if (rigidBodyValues.HasMember("mass") && rigidBodyValues["mass"].IsFloat()) {
                        rigidBodyComp.setMass(rigidBodyValues["mass"].GetFloat());
                    }

                    if (rigidBodyValues.HasMember("restitutionCoeff") && rigidBodyValues["restitutionCoeff"].IsFloat()) {
                        rigidBodyComp.setRestitution(rigidBodyValues["restitutionCoeff"].GetFloat());
                    }

                    if (rigidBodyValues.HasMember("staticFriction") && rigidBodyValues["staticFriction"].IsFloat()) {
                        rigidBodyComp.staticFriction = rigidBodyValues["staticFriction"].GetFloat();
                    }

                    if (rigidBodyValues.HasMember("dynamicFriction") && rigidBodyValues["dynamicFriction"].IsFloat()) {
                        rigidBodyComp.dynamicFriction = rigidBodyValues["dynamicFriction"].GetFloat();
                    }

                    if (rigidBodyValues.HasMember("inertia") && rigidBodyValues["inertia"].IsFloat()) {
                        rigidBodyComp.inertia = rigidBodyValues["inertia"].GetFloat();
                    }

                    if (rigidBodyValues.HasMember("invInertia") && rigidBodyValues["invInertia"].IsFloat()) {
                        rigidBodyComp.invInertia = rigidBodyValues["invInertia"].GetFloat();
                    }

                    if (rigidBodyValues.HasMember("isTrigger") && rigidBodyValues["isTrigger"].IsBool()) {
                        bool isTrigger{ rigidBodyValues["isTrigger"].GetBool() };
                        if (isTrigger) rigidBodyComp.getFlags().flip(Physics::SIMULATE_DYNAMICS);
                    }
                }

                // Check if Collider 
                if (entityValue.HasMember(Keys::COLLIDER_COMPONENT) && entityValue[Keys::COLLIDER_COMPONENT].IsObject()) {
                    const rapidjson::Value& colliderValue = entityValue[Keys::COLLIDER_COMPONENT];

                    // 1. Check Type Int
                    //int colliderType = 0;
                    if (colliderValue.HasMember("type") && colliderValue["type"].IsInt()) {
                        // 0 - Sphere Collider, 1 - AABB, 2 - Polygon, 3 - OBB, 4 - None?
                        int colliderType = colliderValue["type"].GetInt();
                        ColliderComponent& col = gECS.AddComponent<ColliderComponent>(newEntity);
                        Collision::ColliderFactory colFactory{};
                        Collision::ColliderFactory::ArgMap args;

                        switch (colliderType) {
                            case Collision::SPHERE: {               
                                    if (colliderValue.HasMember("radius") && colliderValue["radius"].IsFloat()) {
                                        args["radius"] = std::to_string(colliderValue["radius"].GetFloat());
                                    }
                                    if (colliderValue.HasMember("center") && colliderValue["center"].IsObject()) {
                                        const rapidjson::Value& centerVal{ colliderValue["center"] };
                                        CEVec3 newVal{ ReadVectorFromJson{}(centerVal) };
                                        args["centerX"] = std::to_string(newVal.x);
                                        args["centerY"] = std::to_string(newVal.y);
                                        args["centerZ"] = std::to_string(newVal.z);
                                    }
                                    colFactory.setArgs(args);
                                    col.collider = colFactory.CreateCollider(static_cast<Collision::ColliderType>(colliderType));
                                
                                }          
                                break;
                            case Collision::AABB: {    // AABB
                                if (colliderValue.HasMember("min") && colliderValue["min"].IsObject()) {
                                    const rapidjson::Value& minVal{ colliderValue["min"] };
                                    CEVec3 newVal{ ReadVectorFromJson{}(minVal) };
                                    args["minX"] = std::to_string(newVal.x);
                                    args["minY"] = std::to_string(newVal.y);
                                    args["minZ"] = std::to_string(newVal.z);
                                } if (colliderValue.HasMember("max") && colliderValue["max"].IsObject()) {
                                    const rapidjson::Value& maxVal{ colliderValue["max"] };
                                    CEVec3 newVal{ ReadVectorFromJson{}(maxVal) };
                                    args["maxX"] = std::to_string(newVal.x);
                                    args["maxY"] = std::to_string(newVal.y);
                                    args["maxZ"] = std::to_string(newVal.z);
                                }
                                    colFactory.setArgs(args);
                                    col.collider = colFactory.CreateCollider(static_cast<Collision::ColliderType>(colliderType));
                                }
                                break;
                            case Collision::OBB: {    // OBB
                                if (colliderValue.HasMember("min") && colliderValue["min"].IsObject()) {
                                    const rapidjson::Value& minVal{ colliderValue["min"] };
                                    CEVec3 newVal{ ReadVectorFromJson{}(minVal) };
                                    args["minX"] = std::to_string(newVal.x);
                                    args["minY"] = std::to_string(newVal.y);
                                    args["minZ"] = std::to_string(newVal.z);
                                } if (colliderValue.HasMember("max") && colliderValue["max"].IsObject()) {
                                    const rapidjson::Value& maxVal{ colliderValue["max"] };
                                    CEVec3 newVal{ ReadVectorFromJson{}(maxVal) };
                                    args["maxX"] = std::to_string(newVal.x);
                                    args["maxY"] = std::to_string(newVal.y);
                                    args["maxZ"] = std::to_string(newVal.z);
                                } if (colliderValue.HasMember("rotation") && colliderValue["rotation"].IsFloat()) {
                                    args["rotation"] = std::to_string(colliderValue["rotation"].GetFloat());
                                }
                                colFactory.setArgs(args);
                                col.collider = colFactory.CreateCollider(static_cast<Collision::ColliderType>(colliderType));
                                }
                                break;
                            case Collision::NONE: break;
                            default:
                                col.collider = colFactory.CreateCollider(Collision::AABB);
                        };
                    }
                }

                // Check if Shape
                if (entityValue.HasMember(Keys::SHAPE_COMPONENT) && entityValue[Keys::SHAPE_COMPONENT].IsObject()) {
                    ShapeComponent& s = gECS.AddComponent<ShapeComponent>(newEntity);
                    const rapidjson::Value& shapeValues = entityValue[Keys::SHAPE_COMPONENT];

                    // enable texture bool
                    if (shapeValues.HasMember("useTexture") && shapeValues["useTexture"].IsBool()) {
                        s.enableTexture = shapeValues["useTexture"].GetBool();
                    }

                    // enable isWorldSpace bool
                    if (shapeValues.HasMember("isWorldSpace") && shapeValues["isWorldSpace"].IsBool()) {
                        s.isWorldSpace = shapeValues["isWorldSpace"].GetBool();
                    }

                    // Set wrap mode int
                    if (shapeValues.HasMember("wrapModeU") && shapeValues["wrapModeU"].IsInt()) {
                        s.wrapModeU = shapeValues["wrapModeU"].GetInt();
                    }
                    if (shapeValues.HasMember("wrapModeV") && shapeValues["wrapModeV"].IsInt()) {
                        s.wrapModeV = shapeValues["wrapModeV"].GetInt();
                    }

                    // CEVec2 uvSize
                    if (shapeValues.HasMember("uvSize") && shapeValues["uvSize"].IsObject()) {
                        CEVec2 uvSize{ 1.0f, 1.0f }; // Initialize to default size
                        const rapidjson::Value& sizeValue = shapeValues["uvSize"];
                        // Use HasMember check for safety
                        if (sizeValue.HasMember("x")) uvSize.x = sizeValue["x"].GetFloat();
                        if (sizeValue.HasMember("y")) uvSize.y = sizeValue["y"].GetFloat();

                        s.uvSize = uvSize;
                    }

                    // Set texture name
                    if (shapeValues.HasMember("textureName") && shapeValues["textureName"].IsString()) {
                        s.textureName = shapeValues["textureName"].GetString();
                    }

                    // Set primitive type
                    if (shapeValues.HasMember("type") && shapeValues["type"].IsInt()) {
                        switch (shapeValues["type"].GetInt()) {
                        case 0:
                            s.type = ShapeType::Rectangle;
                            break;
                        case 1:
                            s.type = ShapeType::Line;
                            break;
                        case 2:
                            s.type = ShapeType::Triangle;
                            break;
                        default:
                            s.type = ShapeType::Rectangle;
                            break;
                        };
                    }

                    // Color
                    if (shapeValues.HasMember("color") && shapeValues["color"].IsObject()) {
                        CEColor color{ 0.0f, 0.0f, 0.0f };
                        const rapidjson::Value& colorValue = shapeValues["color"];
                        // Use HasMember check for safety
                        if (colorValue.HasMember("x")) color.x() = colorValue["x"].GetFloat();
                        if (colorValue.HasMember("y")) color.y() = colorValue["y"].GetFloat();
                        if (colorValue.HasMember("z")) color.z() = colorValue["z"].GetFloat();
                        if (colorValue.HasMember("w")) color.w() = colorValue["w"].GetFloat();

                        s.color = color;
                    }

                    // p1
                    if (shapeValues.HasMember("p1") && shapeValues["p1"].IsObject()) {
                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
                        const rapidjson::Value& p1Value = shapeValues["p1"];
                        // Use HasMember check for safety
                        if (p1Value.HasMember("x")) pos.x = p1Value["x"].GetFloat();
                        if (p1Value.HasMember("y")) pos.y = p1Value["y"].GetFloat();
                        if (p1Value.HasMember("z")) pos.z = p1Value["z"].GetFloat();

                        s.p1 = pos;
                    }
                    // p2
                    if (shapeValues.HasMember("p2") && shapeValues["p2"].IsObject()) {
                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
                        const rapidjson::Value& p2Value = shapeValues["p2"];
                        // Use HasMember check for safety
                        if (p2Value.HasMember("x")) pos.x = p2Value["x"].GetFloat();
                        if (p2Value.HasMember("y")) pos.y = p2Value["y"].GetFloat();
                        if (p2Value.HasMember("z")) pos.z = p2Value["z"].GetFloat();

                        s.p2 = pos;
                    }

                    // Thickness
                    if (shapeValues.HasMember("thickness") && shapeValues["thickness"].IsFloat()) {
                        s.thickness = shapeValues["thickness"].GetFloat();
                    }

                    // Preview
                    if (shapeValues.HasMember("preview") && shapeValues["preview"].IsBool()) {
                        s.preview = shapeValues["preview"].GetBool();
                    }
                }

                // Check if Text
                if (entityValue.HasMember(Keys::TEXT_COMPONENT) && entityValue[Keys::TEXT_COMPONENT].IsObject()) {
                    TextComponent& t = gECS.AddComponent<TextComponent>(newEntity);
                    const rapidjson::Value& textValue = entityValue[Keys::TEXT_COMPONENT];

                    // 1. Check Font
                    if (textValue.HasMember("font") && textValue["font"].IsString()) {
                        t.font = textValue["font"].GetString();
                    }

                    // 2. Check Text
                    if (textValue.HasMember("text") && textValue["text"].IsString()) {
                        t.text = textValue["text"].GetString();
                    }

                    // 3. Check Scale
                    if (textValue.HasMember("scale") && textValue["scale"].IsFloat()) {
                        t.scale = textValue["scale"].GetFloat();
                    }

                    // 4. Check Offset
                    if (textValue.HasMember("offset") && textValue["offset"].IsObject()) {
                        CEVec2 offset{ 0.0f, 0.0f };
                        const rapidjson::Value& offsetValue = textValue["offset"];
                        // Use HasMember check for safety
                        if (offsetValue.HasMember("x")) offset.x = offsetValue["x"].GetFloat();
                        if (offsetValue.HasMember("y")) offset.y = offsetValue["y"].GetFloat();
                        t.offset = offset;
                    }

                    // 5. Check Color
                    if (textValue.HasMember("color") && textValue["color"].IsObject()) {
                        CEColor color{ 0.0f, 0.0f, 0.0f };
                        const rapidjson::Value& colorValue = textValue["color"];
                        // Use HasMember check for safety
                        if (colorValue.HasMember("x")) color.x() = colorValue["x"].GetFloat();
                        if (colorValue.HasMember("y")) color.y() = colorValue["y"].GetFloat();
                        if (colorValue.HasMember("z")) color.z() = colorValue["z"].GetFloat();
                        if (colorValue.HasMember("w")) color.w() = colorValue["w"].GetFloat();

                        t.color = color;
                    }

                    // 6. Check Centered
                    if (textValue.HasMember("centered") && textValue["centered"].IsBool()) {
                        t.centered = textValue["centered"].GetBool();
                    }

                    // 7. Check World Space
                    if (textValue.HasMember("isworldspace") && textValue["isworldspace"].IsBool()) {
                        t.isWorldSpace = textValue["isworldspace"].GetBool();
                    }
                }
            
                // Check if Audio
                if (entityValue.HasMember(Keys::AUDIO_COMPONENT) && entityValue[Keys::AUDIO_COMPONENT].IsObject()) {

                }

                // Check if Script
                if (entityValue.HasMember(Keys::SCRIPT_COMPONENT) && entityValue[Keys::SCRIPT_COMPONENT].IsObject()) {

                }

                // Check if Camera
                if (entityValue.HasMember(Keys::CAMERA_COMPONENT) && entityValue[Keys::CAMERA_COMPONENT].IsObject()) {
                    CameraComponent& c = gECS.AddComponent<CameraComponent>(newEntity);
                    const rapidjson::Value& camValue = entityValue[Keys::CAMERA_COMPONENT];

                    // 1. Check Zoom
                    if (camValue.HasMember("zoom") && camValue["zoom"].IsFloat()) {
                        c.zoom = camValue["zoom"].GetFloat();
                    }

                    // 2. Check xBounds
                    if (camValue.HasMember("xBounds") && camValue["xBounds"].IsObject()) {
                        CEVec2 xBounds{ 0.0f, 0.0f };
                        const rapidjson::Value& xBoundsValue = camValue["xBounds"];
                        // Use HasMember check for safety
                        if (xBoundsValue.HasMember("x")) xBounds.x = xBoundsValue["x"].GetFloat();
                        if (xBoundsValue.HasMember("y")) xBounds.y = xBoundsValue["y"].GetFloat();
                        c.xBounds = xBounds;
                    }

                    // 2. Check yBounds
                    if (camValue.HasMember("yBounds") && camValue["yBounds"].IsObject()) {
                        CEVec2 yBounds{ 0.0f, 0.0f };
                        const rapidjson::Value& yBoundsValue = camValue["yBounds"];
                        // Use HasMember check for safety
                        if (yBoundsValue.HasMember("x")) yBounds.x = yBoundsValue["x"].GetFloat();
                        if (yBoundsValue.HasMember("y")) yBounds.y = yBoundsValue["y"].GetFloat();
                        c.yBounds = yBounds;
                    }
                }
            
                // Check if Button
                if (entityValue.HasMember(Keys::BUTTON_COMPONENT) && entityValue[Keys::BUTTON_COMPONENT].IsObject()) {
                    ButtonComponent& b = gECS.AddComponent<ButtonComponent>(newEntity);
                    const rapidjson::Value& buttonValue = entityValue[Keys::BUTTON_COMPONENT];

                    // 1. Check Button State
                    if (buttonValue.HasMember("buttonState") && buttonValue["buttonState"].IsUint()) {
                        b.buttonState = ButtonState::NORMAL; //static_cast<ButtonState>(buttonValue["buttonState"].GetUint());
                    }

                    // ----------------------------------------------------------------------------------------------
                    // 
                    // 2. Check Shape Normal Color 
                    if (buttonValue.HasMember("shapeNormalColor") && buttonValue["shapeNormalColor"].IsObject()) {
                        CEColor color { 0.f, 0.f, 0.f };
                        const rapidjson::Value& colorVal = buttonValue["shapeNormalColor"];
                        // Use HasMember check for safety
                        if (colorVal.HasMember("x")) color.x() = colorVal["x"].GetFloat();
                        if (colorVal.HasMember("y")) color.y() = colorVal["y"].GetFloat();
                        if (colorVal.HasMember("z")) color.z() = colorVal["z"].GetFloat();
                        if (colorVal.HasMember("w")) color.w() = colorVal["w"].GetFloat();
                        b.shapeNormalColor = color;
                    }

                    // 3. Check Shape Hover Color 
                    if (buttonValue.HasMember("shapeHoveredColor") && buttonValue["shapeHoveredColor"].IsObject()) {
                        CEColor color{ 0.f, 0.f, 0.f };
                        const rapidjson::Value& colorVal = buttonValue["shapeHoveredColor"];
                        // Use HasMember check for safety   
                        if (colorVal.HasMember("x")) color.x() = colorVal["x"].GetFloat();
                        if (colorVal.HasMember("y")) color.y() = colorVal["y"].GetFloat();
                        if (colorVal.HasMember("z")) color.z() = colorVal["z"].GetFloat();
                        if (colorVal.HasMember("w")) color.w() = colorVal["w"].GetFloat();
                        b.shapeHoveredColor = color;
                    }
                    
                    // 4. Check Shape Pressed Color 
                    if (buttonValue.HasMember("shapePressedColor") && buttonValue["shapePressedColor"].IsObject()) {
                        CEColor color{ 0.f, 0.f, 0.f };
                        const rapidjson::Value& colorVal = buttonValue["shapePressedColor"];
                        // Use HasMember check for safety
                        if (colorVal.HasMember("x")) color.x() = colorVal["x"].GetFloat();
                        if (colorVal.HasMember("y")) color.y() = colorVal["y"].GetFloat();
                        if (colorVal.HasMember("z")) color.z() = colorVal["z"].GetFloat();
                        if (colorVal.HasMember("w")) color.w() = colorVal["w"].GetFloat();
                        b.shapePressedColor = color;
                    }

                    // ----------------------------------------------------------------------------------------------
                    // 
                    // 5. Check Text Normal Color 
                    if (buttonValue.HasMember("textNormalColor") && buttonValue["textNormalColor"].IsObject()) {
                        CEColor color { 0.f, 0.f, 0.f };
                        const rapidjson::Value& colorVal = buttonValue["textNormalColor"];
                        // Use HasMember check for safety
                        if (colorVal.HasMember("x")) color.x() = colorVal["x"].GetFloat();
                        if (colorVal.HasMember("y")) color.y() = colorVal["y"].GetFloat();
                        if (colorVal.HasMember("z")) color.z() = colorVal["z"].GetFloat();
                        if (colorVal.HasMember("w")) color.w() = colorVal["w"].GetFloat();
                        b.textNormalColor = color;
                    }

                    // 6. Check Text Hover Color 
                    if (buttonValue.HasMember("textHoveredColor") && buttonValue["textHoveredColor"].IsObject()) {
                        CEColor color{ 0.f, 0.f, 0.f };
                        const rapidjson::Value& colorVal = buttonValue["textHoveredColor"];
                        // Use HasMember check for safety   
                        if (colorVal.HasMember("x")) color.x() = colorVal["x"].GetFloat();
                        if (colorVal.HasMember("y")) color.y() = colorVal["y"].GetFloat();
                        if (colorVal.HasMember("z")) color.z() = colorVal["z"].GetFloat();
                        if (colorVal.HasMember("w")) color.w() = colorVal["w"].GetFloat();
                        b.textHoveredColor = color;
                    }
                    
                    // 7. Check Text Pressed Color 
                    if (buttonValue.HasMember("textPressedColor") && buttonValue["textPressedColor"].IsObject()) {
                        CEColor color{ 0.f, 0.f, 0.f };
                        const rapidjson::Value& colorVal = buttonValue["textPressedColor"];
                        // Use HasMember check for safety   
                        if (colorVal.HasMember("x")) color.x() = colorVal["x"].GetFloat();
                        if (colorVal.HasMember("y")) color.y() = colorVal["y"].GetFloat();
                        if (colorVal.HasMember("z")) color.z() = colorVal["z"].GetFloat();
                        if (colorVal.HasMember("w")) color.w() = colorVal["w"].GetFloat();
                        b.textPressedColor = color;
                    }

                    // check shape component
                    ShapeComponent* s = gECS.GetComponent<ShapeComponent>(newEntity);
                    if (s) s->color = b.shapeNormalColor;
                    // check text component
                    TextComponent* t = gECS.GetComponent<TextComponent>(newEntity);
                    if (t) t->color = b.textNormalColor;

                }

                // Check if GUI
                if (entityValue.HasMember(Keys::GUI_COMPONENT) && entityValue[Keys::GUI_COMPONENT].IsObject()) {
                    GuiComponent& g = gECS.AddComponent<GuiComponent>(newEntity);
                    const rapidjson::Value& guiValue = entityValue[Keys::GUI_COMPONENT];

                    // 1. Check uiType
                    if (guiValue.HasMember("uiType") && guiValue["uiType"].IsUint()) {
                        g.uiType = static_cast<UIType>(guiValue["uiType"].GetUint());
                    }
                }

                // Check if Text Input
                if (entityValue.HasMember(Keys::TEXT_INPUT_COMPONENT) && entityValue[Keys::TEXT_INPUT_COMPONENT].IsObject()) {
                    TextInputComponent& t = gECS.AddComponent<TextInputComponent>(newEntity);
                    const rapidjson::Value& textInputValue = entityValue[Keys::TEXT_INPUT_COMPONENT];

                    // 1. Check isFocused
                    if (textInputValue.HasMember("isFocused") && textInputValue["isFocused"].IsBool()) {
                        t.isFocused = textInputValue["isFocused"].GetBool();
                    }
                    
                    // 2. Check cursorPos
                    if (textInputValue.HasMember("cursorPos") && textInputValue["cursorPos"].IsUint()) {
                        t.cursorPos = textInputValue["cursorPos"].GetUint();
                    }
                    
                    // 3. Check maxCharacters
                    if (textInputValue.HasMember("maxCharacters") && textInputValue["maxCharacters"].IsUint()) {
                        t.maxCharacters = textInputValue["maxCharacters"].GetUint();
                    }
                }

                // Check if UI Layout
                if (entityValue.HasMember(Keys::UI_LAYOUT_COMPONENT) && entityValue[Keys::UI_LAYOUT_COMPONENT].IsObject()) {
                    UILayoutComponent& u = gECS.AddComponent<UILayoutComponent>(newEntity);
                    const rapidjson::Value& uiLayoutValue = entityValue[Keys::UI_LAYOUT_COMPONENT];

                    // 1. Check paddingXY
                    if (uiLayoutValue.HasMember("paddingXY") && uiLayoutValue["paddingXY"].IsObject()) {
                        CEVec2 paddingXY{ 0.0f, 0.0f };
                        const rapidjson::Value& paddingXYValue = uiLayoutValue["paddingXY"];
                        // Use HasMember check for safety
                        if (paddingXYValue.HasMember("x")) paddingXY.x = paddingXYValue["x"].GetFloat();
                        if (paddingXYValue.HasMember("y")) paddingXY.y = paddingXYValue["y"].GetFloat();
                        u.paddingXY = paddingXY;
                    }
                    
                    // 2. Check uiLayout
                    if (uiLayoutValue.HasMember("uiLayout") && uiLayoutValue["uiLayout"].IsUint()) {
                        u.uiLayout = static_cast<UILayout>(uiLayoutValue["uiLayout"].GetUint());
                    }
                }

                // Check if Post Processing
                if (entityValue.HasMember(Keys::PP_COMPONENT) && entityValue[Keys::PP_COMPONENT].IsObject()) {
                    PPComponent& p = gECS.AddComponent<PPComponent>(newEntity);
                    const rapidjson::Value& ppValue = entityValue[Keys::PP_COMPONENT];

                    // 1. Check Brightness
                    if (ppValue.HasMember("brightness") && ppValue["brightness"].IsFloat()) {
                        p.brightness = ppValue["brightness"].GetFloat();
                    }

                    // 2. Check Saturation
                    if (ppValue.HasMember("saturation") && ppValue["saturation"].IsFloat()) {
                        p.saturation = ppValue["saturation"].GetFloat();
                    }

                    // 3. Check Contrast
                    if (ppValue.HasMember("contrast") && ppValue["contrast"].IsFloat()) {
                        p.contrast = ppValue["contrast"].GetFloat();
                    }

                    // 4. Check Active
                    if (ppValue.HasMember("active") && ppValue["active"].IsBool()) {
                        p.active = ppValue["active"].GetBool();
                    }

                }

                // Check if TileMapComponent
                if (entityValue.HasMember(Keys::TILEMAP_COMPONENT) && entityValue[Keys::TILEMAP_COMPONENT].IsObject()) {
                    TileMapComponent& tm = gECS.AddComponent<TileMapComponent>(newEntity);
                    const rapidjson::Value& tmValue = entityValue[Keys::TILEMAP_COMPONENT];

                    // --- Set Defaults ---
                    tm.activeAsset = "none";
                    tm.pOpen = false;
                    tm.isCacheDirty = true;

                    // 1. Tile Size (int)
                    if (tmValue.HasMember("tileSize") && tmValue["tileSize"].IsInt()) {
                        tm.tileSize = tmValue["tileSize"].GetInt();
                    }

                    // 2. Active Asset (string) - optional override if saved
                    if (tmValue.HasMember("activeAsset") && tmValue["activeAsset"].IsString()) {
                        tm.activeAsset = tmValue["activeAsset"].GetString();
                    }

                    // 3. Tile Asset Names (std::vector<std::string>)
                    if (tmValue.HasMember("tileAssetNames") && tmValue["tileAssetNames"].IsArray()) {
                        const rapidjson::Value& assetsArr = tmValue["tileAssetNames"];
                        for (rapidjson::SizeType k = 0; k < assetsArr.Size(); ++k) {
                            if (assetsArr[k].IsString()) {
                                tm.tileAssetNames.push_back(assetsArr[k].GetString());
                            }
                        }
                    }

                    // 4. Grid Data (std::map<std::pair<int, int>, std::string>)
                    // JSON Structure: [ { "x": 1, "y": 2, "asset": "grass.json" }, ... ]
                    if (tmValue.HasMember("gridData") && tmValue["gridData"].IsArray()) {
                        const rapidjson::Value& gridArr = tmValue["gridData"];
                        for (rapidjson::SizeType k = 0; k < gridArr.Size(); ++k) {
                            const rapidjson::Value& tileObj = gridArr[k];

                            if (tileObj.IsObject() &&
                                tileObj.HasMember("x") && tileObj["x"].IsInt() &&
                                tileObj.HasMember("y") && tileObj["y"].IsInt() &&
                                tileObj.HasMember("asset") && tileObj["asset"].IsString()) {

                                int x = tileObj["x"].GetInt();
                                int y = tileObj["y"].GetInt();
                                std::string asset = tileObj["asset"].GetString();

                                // Insert into sparse map
                                tm.gridData[{x, y}] = asset;
                            }
                        }
                    }
                }

                // Check if ParentComponent
                if (entityValue.HasMember(Keys::PARENT_COMPONENT) && entityValue[Keys::PARENT_COMPONENT].IsObject()) {
                    const rapidjson::Value& pVal = entityValue[Keys::PARENT_COMPONENT];

                    // Create the component on the child
                    Component::ParentComponent& pc = gECS.AddComponent<Component::ParentComponent>(newEntity);

                    // 1. Queue the link request
                    if (pVal.HasMember("parentName") && pVal["parentName"].IsString()) {
                        ParentLinkRequest req;
                        req.childEntity = newEntity;
                        req.parentName = pVal["parentName"].GetString();
                        parentLinksToProcess.push_back(req);
                    }

                    // 2. Load Local Data
                    if (pVal.HasMember("localPos") && pVal["localPos"].IsObject()) {
                        pc.localPos.x = pVal["localPos"]["x"].GetFloat();
                        pc.localPos.y = pVal["localPos"]["y"].GetFloat();
                        pc.localPos.z = pVal["localPos"]["z"].GetFloat();
                    }
                    if (pVal.HasMember("localRot") && pVal["localRot"].IsFloat()) {
                        pc.localRot = pVal["localRot"].GetFloat();
                    }
                    if (pVal.HasMember("localScale") && pVal["localScale"].IsObject()) {
                        pc.localScale.x = pVal["localScale"]["x"].GetFloat();
                        pc.localScale.y = pVal["localScale"]["y"].GetFloat();
                    }
                    if (pVal.HasMember("inheritColor") && pVal["inheritColor"].IsBool()) {
                        pc.inheritColor = pVal["inheritColor"].GetBool();
                    }
                    if (pVal.HasMember("colorMul") && pVal["colorMul"].IsObject()) {
                        pc.colorMul.x() = pVal["colorMul"]["x"].GetFloat();
                        pc.colorMul.y() = pVal["colorMul"]["y"].GetFloat();
                        pc.colorMul.z() = pVal["colorMul"]["z"].GetFloat();
                        pc.colorMul.w() = pVal["colorMul"]["w"].GetFloat();
                    }
                }
            }
        
            // =========================================================
            // POST-LOAD: Reconstruct Hierarchy Links
            // =========================================================
            // We need to find the entity ID for every parentName we saved.
            // Since NameComponent isn't usually a map, we iterate (or you can build a temporary map for speed).

            // Optimization: Build a map of Name -> EntityID for the loaded scene
            std::unordered_map<std::string, Entity> nameToEntityMap;
            const auto& nameArr = gECS.GetComponentArray<NameComponent>();
            for (Entity e : nameArr.indexToEntity) {
                if (gECS.HasComponent<NameComponent>(e)) {
                    nameToEntityMap[gECS.GetComponent<NameComponent>(e)->name] = e;
                }
            }

            for (const auto& req : parentLinksToProcess) {
                auto it = nameToEntityMap.find(req.parentName);
                if (it != nameToEntityMap.end()) {
                    Entity parentEntity = it->second;

                    // 1. Set the ID in the child's ParentComponent
                    if (gECS.HasComponent<Component::ParentComponent>(req.childEntity)) {
                        gECS.GetComponent<Component::ParentComponent>(req.childEntity)->parent = parentEntity;
                    }

                    // 2. Add to parent's ChildrenComponent
                    // We manually do this instead of calling AttachChild to avoid re-calculating 
                    // the local transforms (since we just loaded the correct local values from file).
                    if (!gECS.HasComponent<Component::ChildrenComponent>(parentEntity)) {
                        gECS.AddComponent<Component::ChildrenComponent>(parentEntity);
                    }

                    auto* childComp = gECS.GetComponent<Component::ChildrenComponent>(parentEntity);
                    childComp->children.push_back(req.childEntity);
                }
                else {
                    std::cout << "WARNING: Could not find parent '" << req.parentName
                        << "' for child Entity " << req.childEntity << std::endl;
                }
            }
        }

        /*!
        @brief Saves all current entities and their components from the ECS into a JSON scene file.
        @param sceneName The name of the scene asset to save to (e.g., "level1.json").
        */
        void SaveScenes(std::string const& sceneName) {
            ECS& gECS = *ECS::getInstance();
            Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();

            // 1. Get or create the SceneAsset to store the data
            Assets::SceneAsset* sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);
            if (!sceneAsset) {
                std::filesystem::path fullPath = std::filesystem::path(Assets::rootPath) / "scenes" / sceneName;

                // Handle case where the scene asset doesn't exist (e.g., create a new one)
                Assets::SceneAsset::Create(fullPath.string());

                // Create SceneAsset object from file data (JSON)
                std::unique_ptr<Assets::SceneAsset> newScene = Assets::SceneAsset::Create(fullPath.string());

                // Validate 
                if (newScene) {
                    // Load the SceneAsset into AssetManager
                    Assets::AssetManager::GetInstance().Load(sceneName, std::move(newScene));
                    std::cout << "AssetManager loaded scene data: " << sceneName << "(" << fullPath.string() << ")"
                        << std::endl;
                }
                else {
                    std::cerr << "Failed to create SceneAsset: " << sceneName << std::endl;
                }

                rapidjson::Document manifestDoc;
                std::filesystem::path manifestPath = std::filesystem::path(Assets::rootPath) / "json" / "assetmanifest.json";

                // Load the existing manifest
                std::ifstream ifs(manifestPath);
                if (!ifs.is_open()) {
                    std::cerr << "CRITICAL: Could not open manifest file to add new scene: " << manifestPath << std::endl;
                }
                else {
                    rapidjson::IStreamWrapper isw(ifs);
                    manifestDoc.ParseStream(isw);
                    ifs.close(); // Close before re-opening for write
                }

                if (manifestDoc.HasParseError()) {
                    std::cerr << "WARNING: Could not parse manifest file. Creating new one: " << manifestPath << std::endl;
                    manifestDoc.SetObject(); // Start fresh if parse failed
                }

                // Get "scenes" array, or create it if it doesn't exist
                if (!manifestDoc.HasMember("scenes")) {
                    manifestDoc.AddMember("scenes", rapidjson::Value(rapidjson::kArrayType), manifestDoc.GetAllocator());
                }
                rapidjson::Value& scenesArray = manifestDoc["scenes"];

                // Create the new entry for the manifest
                rapidjson::Value newEntry(rapidjson::kObjectType);
                newEntry.AddMember("name", rapidjson::Value(sceneName.c_str(), manifestDoc.GetAllocator()), manifestDoc.GetAllocator());
                newEntry.AddMember("path", rapidjson::Value(sceneName.c_str(), manifestDoc.GetAllocator()), manifestDoc.GetAllocator());

                // Add new entry to the array
                scenesArray.PushBack(newEntry, manifestDoc.GetAllocator());

                // --- 4. (NEW) Write the updated manifest back to disk ---
                std::ofstream ofs(manifestPath);
                if (!ofs.is_open()) {
                    std::cerr << "CRITICAL: Could not write updated manifest: " << manifestPath << std::endl;
                }
                else {
                    rapidjson::StringBuffer buffer;
                    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
                    manifestDoc.Accept(writer);
                    ofs << buffer.GetString();
                    ofs.close();
                    std::cout << "Successfully added '" << sceneName << "' to asset manifest." << std::endl;
                }

                // --- 5. Load the new asset into the manager ---
                Assets::AssetManager::GetInstance().Load(sceneName, std::move(newScene));
                std::cout << "AssetManager loaded scene data: " << sceneName << "(" << fullPath.string() << ")" << std::endl;

                sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);
            }

            // 2. Prepare the rapidjson Document
            rapidjson::Document doc;
            doc.SetObject();

            // rapidjson needs an allocator for creating values
            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

            // Create the top-level ENTITIES array
            rapidjson::Value entitiesArray(rapidjson::kArrayType);

            std::cout << "Saving entities for scene: " << sceneName << "..." << std::endl;


            // Cycle through entities
            for (Entity const& id : gECS.GetComponentArray<Component::NameComponent>().indexToEntity) {

                if (gECS.HasComponent<TransientComponent>(id)) continue;

                rapidjson::Value entityValue(rapidjson::kObjectType);

                // =========================================================
                // A. NameComponent
                // =========================================================
                if (gECS.HasComponent<NameComponent>(id)) {
                    const NameComponent& nameComp = *gECS.GetComponent<NameComponent>(id);
                    rapidjson::Value nameValue(nameComp.name.c_str(), allocator);
                    entityValue.AddMember(
                        rapidjson::StringRef(Keys::NAME_COMPONENT), // StringRef for the key
                        rapidjson::Value(nameComp.name.c_str(), allocator), // Creates an rvalue for the value
                        allocator
                    );
                }

                // =========================================================
                // B. LayerComponent
                // =========================================================
                if (gECS.HasComponent<LayerComponent>(id)) {
                    const LayerComponent& layerComp = *gECS.GetComponent<LayerComponent>(id);
                    rapidjson::Value layerValue(rapidjson::kObjectType);

                    // 1. Current Layer
                    layerValue.AddMember("currentLayer", layerComp.currentLayer, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::LAYER_COMPONENT),
                        std::move(layerValue), allocator);
                }

                // =========================================================
                // B. TransformComponent
                // =========================================================
                if (gECS.HasComponent<TransformComponent>(id)) {
                    const TransformComponent& transComp = *gECS.GetComponent<TransformComponent>(id);
                    rapidjson::Value transformValue(rapidjson::kObjectType);

                    // 1. Rotation
                    transformValue.AddMember("rotation", transComp.rotation, allocator);

                    // 2. Position (CEVec3 -> JSON Object)
                    rapidjson::Value posValue(rapidjson::kObjectType);
                    posValue.AddMember("x", transComp.position.x, allocator);
                    posValue.AddMember("y", transComp.position.y, allocator);
                    posValue.AddMember("z", transComp.position.z, allocator);
                    transformValue.AddMember("position", posValue, allocator);

                    // 3. Size (CEVec2 -> JSON Object)
                    rapidjson::Value sizeValue(rapidjson::kObjectType);
                    sizeValue.AddMember("x", transComp.size.x, allocator);
                    sizeValue.AddMember("y", transComp.size.y, allocator);
                    transformValue.AddMember("size", sizeValue, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::TRANSFORM_COMPONENT),
                        std::move(transformValue), allocator);
                }

                // =========================================================
                // C. RigidBodyComponent
                // =========================================================
                if (gECS.HasComponent<RigidBodyComponent>(id)) {
                    // RigidBody is stored as a boolean 'true' if the component exists
                    RigidBodyComponent& rigidBodyComp = *gECS.GetComponent<RigidBodyComponent>(id);

                    rapidjson::Value rigidBodyValue{ rapidjson::kObjectType };

                    rigidBodyValue.AddMember("mass", rigidBodyComp.getMass(), allocator);
                    rigidBodyValue.AddMember("restitutionCoeff", rigidBodyComp.getCoeff(), allocator);

                    rapidjson::Value maxVelVal{ rapidjson::kObjectType };

                    CEVec3 maxVel{ rigidBodyComp.getMaxVelocity() };
                    maxVelVal.AddMember("x", maxVel.x, allocator);
                    maxVelVal.AddMember("y", maxVel.y, allocator);
                    maxVelVal.AddMember("z", maxVel.z, allocator);

                    rigidBodyValue.AddMember("maximumVelocity", std::move(maxVelVal), allocator);

                    rigidBodyValue.AddMember("staticFriction", rigidBodyComp.staticFriction, allocator);
                    rigidBodyValue.AddMember("dynamicFriction", rigidBodyComp.dynamicFriction, allocator);

                    rigidBodyValue.AddMember("inertia", rigidBodyComp.inertia, allocator);
                    rigidBodyValue.AddMember("invInertia", rigidBodyComp.invInertia, allocator);

                    bool isTrigger{ rigidBodyComp.getFlags().test(Physics::SIMULATE_DYNAMICS) };
                    rigidBodyValue.AddMember("isTrigger", isTrigger, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::RIGIDBODY_COMPONENT), std::move(rigidBodyValue) , allocator);
                }

                // =========================================================
                // D. ColliderComponent
                // =========================================================
                if (gECS.HasComponent<ColliderComponent>(id)) {
                    const ColliderComponent& colComp = *gECS.GetComponent<ColliderComponent>(id);
                    rapidjson::Value colliderValue(rapidjson::kObjectType);

                    // 1. Type (integer)
                    // Assuming Collision::SPHERE = 0, Collision::AABB = 1
                    colliderValue.AddMember("type", (int)colComp.collider->type, allocator);

                    // NOTE: You can skip adding the 'sphere' or 'aabb' objects 
                    // if you don't save their properties (like radius or extents), 
                    // as your LoadScenes only reads the 'type' for now.
                    // If you later save extents/radius, they would go here.
                    switch (colComp.collider->type) {
                        case Collision::SPHERE: {
                            
                            Collision::Sphere const& sphereTemp = dynamic_cast<Collision::Sphere const&>(*(colComp.collider));
                            colliderValue.AddMember("radius", sphereTemp.radius, allocator);
                            rapidjson::Value centerVal{ rapidjson::kObjectType };
                            centerVal.AddMember("x", sphereTemp.position.x, allocator);
                            centerVal.AddMember("y", sphereTemp.position.y, allocator);
                            centerVal.AddMember("z", sphereTemp.position.z, allocator);
                            colliderValue.AddMember("center", centerVal, allocator);

                        }
                        break;
                        case Collision::AABB: {
                            Collision::AABBRect const& aabbTemp = dynamic_cast<Collision::AABBRect const&>(*(colComp.collider));
                            rapidjson::Value min{ rapidjson::kObjectType };
                            min.AddMember("x", aabbTemp.min.x, allocator);
                            min.AddMember("y", aabbTemp.min.y, allocator);
                            min.AddMember("z", aabbTemp.min.z, allocator);
                            colliderValue.AddMember("min", min, allocator);

                            rapidjson::Value max{ rapidjson::kObjectType };
                            max.AddMember("x", aabbTemp.max.x, allocator);
                            max.AddMember("y", aabbTemp.max.y, allocator);
                            max.AddMember("z", aabbTemp.max.z, allocator);
                            colliderValue.AddMember("max", max, allocator);
                        }
                        break;
                        case Collision::OBB: {
                            Collision::OBBRect const& obbTemp = dynamic_cast<Collision::OBBRect const&>(*(colComp.collider));
                            rapidjson::Value min{ rapidjson::kObjectType };
                            min.AddMember("x", obbTemp.min.x, allocator);
                            min.AddMember("y", obbTemp.min.y, allocator);
                            min.AddMember("z", obbTemp.min.z, allocator);
                            colliderValue.AddMember("min", min, allocator);

                            rapidjson::Value max{ rapidjson::kObjectType };
                            max.AddMember("x", obbTemp.max.x, allocator);
                            max.AddMember("y", obbTemp.max.y, allocator);
                            max.AddMember("z", obbTemp.max.z, allocator);
                            colliderValue.AddMember("max", max, allocator);

                            colliderValue.AddMember("rotation", obbTemp.rotation, allocator);
                            break;
                        }
                        case Collision::NONE: {
                            break;
                        }
                        default:
                        break;
                    }

                    entityValue.AddMember(rapidjson::StringRef(Keys::COLLIDER_COMPONENT), std::move(colliderValue), allocator);
                }

                // =========================================================
                // E. ShapeComponent
                // =========================================================
                if (gECS.HasComponent<ShapeComponent>(id)) {
                    const ShapeComponent& sComp = *gECS.GetComponent<ShapeComponent>(id);
                    rapidjson::Value shapeValues(rapidjson::kObjectType);

                    // Boolean, Integers, Floats
                    shapeValues.AddMember("useTexture", sComp.enableTexture, allocator);
                    shapeValues.AddMember("isWorldSpace", sComp.isWorldSpace, allocator);
                    shapeValues.AddMember("wrapModeU", (int)sComp.wrapModeU, allocator);
                    shapeValues.AddMember("wrapModeV", (int)sComp.wrapModeV, allocator);
                    shapeValues.AddMember("thickness", sComp.thickness, allocator);
                    shapeValues.AddMember("preview", sComp.preview, allocator);

                    // ShapeType (integer)
                    shapeValues.AddMember("type", (int)sComp.type, allocator);

                    // Texture Name (string)
                    rapidjson::Value textureNameValue(sComp.textureName.c_str(), allocator);
                    shapeValues.AddMember("textureName", textureNameValue, allocator);

                    // CEVec2 uvSize
                    rapidjson::Value uvSizeValue(rapidjson::kObjectType);
                    uvSizeValue.AddMember("x", sComp.uvSize.x, allocator);
                    uvSizeValue.AddMember("y", sComp.uvSize.y, allocator);
                    shapeValues.AddMember("uvSize", uvSizeValue, allocator);

                    // CEVec3 Color
                    rapidjson::Value colorValue(rapidjson::kObjectType);
                    colorValue.AddMember("x", sComp.color.x(), allocator);
                    colorValue.AddMember("y", sComp.color.y(), allocator);
                    colorValue.AddMember("z", sComp.color.z(), allocator);
                    colorValue.AddMember("w", sComp.color.w(), allocator);
                    shapeValues.AddMember("color", colorValue, allocator);

                    // Line/Triangle points (p1, p2, t1, t2, t3)
                    // NOTE: Your LoadScenes has a bug: it tries to read t1, t2, t3 even if the shape is a Line (which uses p1, p2). 
                    // We save all to be robust.

                    // p1
                    rapidjson::Value p1Value(rapidjson::kObjectType);
                    p1Value.AddMember("x", sComp.p1.x, allocator);
                    p1Value.AddMember("y", sComp.p1.y, allocator);
                    p1Value.AddMember("z", sComp.p1.z, allocator);
                    shapeValues.AddMember("p1", p1Value, allocator);

                    // p2
                    rapidjson::Value p2Value(rapidjson::kObjectType);
                    p2Value.AddMember("x", sComp.p2.x, allocator);
                    p2Value.AddMember("y", sComp.p2.y, allocator);
                    p2Value.AddMember("z", sComp.p2.z, allocator);
                    shapeValues.AddMember("p2", p2Value, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::SHAPE_COMPONENT), shapeValues, allocator);
                }

                // =========================================================
                // F. TextComponent
                // =========================================================
                if (gECS.HasComponent<TextComponent>(id)) {
                    const TextComponent& tComp = *gECS.GetComponent<TextComponent>(id);
                    rapidjson::Value textValue(rapidjson::kObjectType);

                    // 1. Font (string)
                    rapidjson::Value fontValue(tComp.font.c_str(), allocator);
                    textValue.AddMember("font", fontValue, allocator);

                    // 2. Text (string)
                    rapidjson::Value strValue(tComp.text.c_str(), allocator);
                    textValue.AddMember("text", strValue, allocator);

                    // 3. Scale (float)
                    textValue.AddMember("scale", tComp.scale, allocator);

                    // 4. Offset (CEVec2 -> JSON Object)
                    rapidjson::Value offsetValue(rapidjson::kObjectType);
                    offsetValue.AddMember("x", tComp.offset.x, allocator);
                    offsetValue.AddMember("y", tComp.offset.y, allocator);
                    textValue.AddMember("offset", offsetValue, allocator);

                    // 5. Color (CEVec3 -> JSON Object)
                    rapidjson::Value colorValue(rapidjson::kObjectType);
                    colorValue.AddMember("x", tComp.color.x(), allocator);
                    colorValue.AddMember("y", tComp.color.y(), allocator);
                    colorValue.AddMember("z", tComp.color.z(), allocator);
                    colorValue.AddMember("w", tComp.color.w(), allocator);
                    textValue.AddMember("color", colorValue, allocator);

                    // 6. Centered (bool)
                    textValue.AddMember("centered", tComp.centered, allocator);

                    // 7. IsWorldSpace (bool)
                    textValue.AddMember("isworldspace", tComp.isWorldSpace, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::TEXT_COMPONENT), textValue, allocator);
                }

                // =========================================================
                // G. AudioComponent
                // =========================================================
                if (gECS.HasComponent<AudioEmitterComponent>(id)) {

                }

                // =========================================================
                // H. ScriptComponent
                // =========================================================
                if (gECS.HasComponent<ScriptComponent>(id)) {

                }

                // =========================================================
                // I. CameraComponent
                // =========================================================
                if (gECS.HasComponent<CameraComponent>(id)) {
                    const CameraComponent& cameraComp = *gECS.GetComponent<CameraComponent>(id);
                    rapidjson::Value camValue(rapidjson::kObjectType);
                    // 1. Zoom (float)
                    camValue.AddMember("zoom", cameraComp.zoom, allocator);

                    // 2. xBounds (CEVec2 -> JSON Object)
                    rapidjson::Value xBoundsValue(rapidjson::kObjectType);
                    xBoundsValue.AddMember("x", cameraComp.xBounds.x, allocator);
                    xBoundsValue.AddMember("y", cameraComp.xBounds.y, allocator);
                    camValue.AddMember("xBounds", xBoundsValue, allocator);

                    // 2. yBounds (CEVec2 -> JSON Object)
                    rapidjson::Value yBoundsValue(rapidjson::kObjectType);
                    yBoundsValue.AddMember("x", cameraComp.yBounds.x, allocator);
                    yBoundsValue.AddMember("y", cameraComp.yBounds.y, allocator);
                    camValue.AddMember("yBounds", yBoundsValue, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::CAMERA_COMPONENT), camValue, allocator);
                }

                // =========================================================
                // J. ButtonComponent
                // =========================================================
                if (gECS.HasComponent<ButtonComponent>(id)) {
                    const ButtonComponent& buttonComp = *gECS.GetComponent<ButtonComponent>(id);
                    rapidjson::Value buttonVal(rapidjson::kObjectType);
                    // 1.  buttonState (unsigned int)
                    buttonVal.AddMember("buttonState", 
                        static_cast<std::underlying_type_t<ButtonState>>(buttonComp.buttonState), allocator);
                    
                    // -------------------------------------------------------------------------------
                    // 
                    // 2. shapeNormalColor (CEVec3 -> JSON Object)
                    rapidjson::Value shapeNormalColorValue(rapidjson::kObjectType);
                    shapeNormalColorValue.AddMember("x", buttonComp.shapeNormalColor.x(), allocator);
                    shapeNormalColorValue.AddMember("y", buttonComp.shapeNormalColor.y(), allocator);
                    shapeNormalColorValue.AddMember("z", buttonComp.shapeNormalColor.z(), allocator);
                    shapeNormalColorValue.AddMember("w", buttonComp.shapeNormalColor.w(), allocator);
                    buttonVal.AddMember("shapeNormalColor", shapeNormalColorValue, allocator);

                    // 3. shapeHoveredColor (CEVec2 -> JSON Object)
                    rapidjson::Value shapeHoveredColorValue(rapidjson::kObjectType);
                    shapeHoveredColorValue.AddMember("x", buttonComp.shapeHoveredColor.x(), allocator);
                    shapeHoveredColorValue.AddMember("y", buttonComp.shapeHoveredColor.y(), allocator);
                    shapeHoveredColorValue.AddMember("z", buttonComp.shapeHoveredColor.z(), allocator);
                    shapeHoveredColorValue.AddMember("w", buttonComp.shapeHoveredColor.w(), allocator);
                    buttonVal.AddMember("shapeHoveredColor", shapeHoveredColorValue, allocator);

                    // 4. shapePressedColor (CEVec2 -> JSON Object)
                    rapidjson::Value shapePressedColorValue(rapidjson::kObjectType);
                    shapePressedColorValue.AddMember("x", buttonComp.shapePressedColor.x(), allocator);
                    shapePressedColorValue.AddMember("y", buttonComp.shapePressedColor.y(), allocator);
                    shapePressedColorValue.AddMember("z", buttonComp.shapePressedColor.z(), allocator);
                    shapePressedColorValue.AddMember("w", buttonComp.shapePressedColor.w(), allocator);
                    buttonVal.AddMember("shapePressedColor", shapePressedColorValue, allocator);

                    // -------------------------------------------------------------------------------
                    //
                    // 5. textNormalColor (CEVec3 -> JSON Object)
                    rapidjson::Value textNormalColorValue(rapidjson::kObjectType);
                    textNormalColorValue.AddMember("x", buttonComp.textNormalColor.x(), allocator);
                    textNormalColorValue.AddMember("y", buttonComp.textNormalColor.y(), allocator);
                    textNormalColorValue.AddMember("z", buttonComp.textNormalColor.z(), allocator);
                    textNormalColorValue.AddMember("w", buttonComp.textNormalColor.w(), allocator);
                    buttonVal.AddMember("textNormalColor", textNormalColorValue, allocator);

                    // 6. textHoveredColor (CEVec2 -> JSON Object)
                    rapidjson::Value textHoveredColorValue(rapidjson::kObjectType);
                    textHoveredColorValue.AddMember("x", buttonComp.textHoveredColor.x(), allocator);
                    textHoveredColorValue.AddMember("y", buttonComp.textHoveredColor.y(), allocator);
                    textHoveredColorValue.AddMember("z", buttonComp.textHoveredColor.z(), allocator);
                    textHoveredColorValue.AddMember("w", buttonComp.textHoveredColor.w(), allocator);
                    buttonVal.AddMember("textHoveredColor", textHoveredColorValue, allocator);

                    // 7. textPressedColor (CEVec2 -> JSON Object)
                    rapidjson::Value textPressedColorValue(rapidjson::kObjectType);
                    textPressedColorValue.AddMember("x", buttonComp.textPressedColor.x(), allocator);
                    textPressedColorValue.AddMember("y", buttonComp.textPressedColor.y(), allocator);
                    textPressedColorValue.AddMember("z", buttonComp.textPressedColor.z(), allocator);
                    textPressedColorValue.AddMember("w", buttonComp.textPressedColor.w(), allocator);
                    buttonVal.AddMember("textPressedColor", textPressedColorValue, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::BUTTON_COMPONENT), buttonVal, allocator);
                }

                // =========================================================
                // K. GuiComponent
                // =========================================================
                if (gECS.HasComponent<GuiComponent>(id)) {
                    const GuiComponent& guiComp = *gECS.GetComponent<GuiComponent>(id);
                    rapidjson::Value guiValue(rapidjson::kObjectType);
                    // 1.  uiType (unsigned int)
                    guiValue.AddMember("uiType",
                        static_cast<std::underlying_type_t<UIType>>(guiComp.uiType), allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::GUI_COMPONENT), guiValue, allocator);
                }

                // =========================================================
                // L. TextInputComponent
                // =========================================================
                if (gECS.HasComponent<TextInputComponent>(id)) {
                    const TextInputComponent& textInputComp = 
                        *gECS.GetComponent<TextInputComponent>(id);
                    rapidjson::Value textInputValue(rapidjson::kObjectType);

                    // 1. isFocused (bool)
                    textInputValue.AddMember("isFocused", textInputComp.isFocused, allocator);
                    
                    // 2. cursorPos (size_t)
                    textInputValue.AddMember("cursorPos", textInputComp.cursorPos, allocator);

                    // 3. maxCharacters (size_t)
                    textInputValue.AddMember("maxCharacters", textInputComp.maxCharacters, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::TEXT_INPUT_COMPONENT), textInputValue, allocator);
                }

                // =========================================================
                // M. UILayoutComponent
                // =========================================================
                if (gECS.HasComponent<UILayoutComponent>(id)) {
                    const UILayoutComponent& uiLayoutComp = *gECS.GetComponent<UILayoutComponent>(id);
                    rapidjson::Value uiLayoutValue(rapidjson::kObjectType);
                    // 2. paddingXY (CEVec2 -> JSON Object)
                    rapidjson::Value paddingXYValue(rapidjson::kObjectType);
                    paddingXYValue.AddMember("x", uiLayoutComp.paddingXY.x, allocator);
                    paddingXYValue.AddMember("y", uiLayoutComp.paddingXY.y, allocator);
                    uiLayoutValue.AddMember("paddingXY", paddingXYValue, allocator);
                    
                    // 2.  uiLayout (unsigned int)
                    uiLayoutValue.AddMember("uiLayout",
                        static_cast<std::underlying_type_t<UILayout>>(uiLayoutComp.uiLayout), allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::UI_LAYOUT_COMPONENT), uiLayoutValue, allocator);
                }

                // =========================================================
                // N. PPComponent
                // =========================================================
                if (gECS.HasComponent<PPComponent>(id)) {
                    const PPComponent& ppComp = *gECS.GetComponent<PPComponent>(id);
                    rapidjson::Value ppValue(rapidjson::kObjectType);
                    
                    // 1.  brightness (float)
                    ppValue.AddMember("brightness", ppComp.brightness, allocator);
                    // 2.  contrast (float)
                    ppValue.AddMember("contrast", ppComp.contrast, allocator);
                    // 3.  saturation (float)
                    ppValue.AddMember("saturation", ppComp.saturation, allocator);
                    // 4.  active (bool)
                    ppValue.AddMember("active", ppComp.active, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::PP_COMPONENT), ppValue, allocator);
                }

                // =========================================================
                // O. TileMapComponent (Refactored for Sparse Grid)
                // =========================================================
                if (gECS.HasComponent<TileMapComponent>(id)) {
                    const TileMapComponent& tmComp = *gECS.GetComponent<TileMapComponent>(id);
                    rapidjson::Value tmValue(rapidjson::kObjectType);

                    // 1. tileSize (int)
                    tmValue.AddMember("tileSize", tmComp.tileSize, allocator);

                    // 2. activeAsset (string)
                    // We save this so the editor remembers what brush was selected, 
                    // though defaults reset it to "none" on load per your request, 
                    // saving it allows you to remove that override later if you want persistence.
                    rapidjson::Value activeAssetVal;
                    activeAssetVal.SetString(tmComp.activeAsset.c_str(), allocator);
                    tmValue.AddMember("activeAsset", activeAssetVal, allocator);

                    // 3. tileAssetNames (vector<string>)
                    rapidjson::Value assetsArray(rapidjson::kArrayType);
                    for (const std::string& assetName : tmComp.tileAssetNames) {
                        rapidjson::Value strVal;
                        strVal.SetString(assetName.c_str(), allocator);
                        assetsArray.PushBack(strVal, allocator);
                    }
                    tmValue.AddMember("tileAssetNames", assetsArray, allocator);

                    // 4. gridData (map<pair<int,int>, string>)
                    // Convert the map into an Array of Objects for JSON: [ {x:0, y:0, asset:"name"}, ... ]
                    rapidjson::Value gridDataArray(rapidjson::kArrayType);

                    for (const auto& [pos, assetName] : tmComp.gridData) {
                        rapidjson::Value tileObj(rapidjson::kObjectType);

                        tileObj.AddMember("x", pos.first, allocator);
                        tileObj.AddMember("y", pos.second, allocator);

                        rapidjson::Value assetVal;
                        assetVal.SetString(assetName.c_str(), allocator);
                        tileObj.AddMember("asset", assetVal, allocator);

                        gridDataArray.PushBack(tileObj, allocator);
                    }
                    tmValue.AddMember("gridData", gridDataArray, allocator);

                    // Note: We do NOT save pOpen, isCacheDirty, or renderCache as these are runtime/editor only.

                    entityValue.AddMember(rapidjson::StringRef(Keys::TILEMAP_COMPONENT), tmValue, allocator);
                }

                // =========================================================
                // P. ParentComponent
                // =========================================================
                if (gECS.HasComponent<Component::ParentComponent>(id)) {
                    const Component::ParentComponent& parentComp = *gECS.GetComponent<Component::ParentComponent>(id);

                    // Only save if there is a valid parent
                    if (parentComp.parent != 0) {
                        rapidjson::Value parentValue(rapidjson::kObjectType);

                        // 1. Save Parent Name (Critical for linking back up)
                        // We assume the parent MUST have a NameComponent
                        if (gECS.HasComponent<NameComponent>(parentComp.parent)) {
                            std::string pName = gECS.GetComponent<NameComponent>(parentComp.parent)->name;
                            rapidjson::Value pNameVal(pName.c_str(), allocator);
                            parentValue.AddMember("parentName", pNameVal, allocator);
                        }

                        // 2. Local Position
                        rapidjson::Value localPosVal(rapidjson::kObjectType);
                        localPosVal.AddMember("x", parentComp.localPos.x, allocator);
                        localPosVal.AddMember("y", parentComp.localPos.y, allocator);
                        localPosVal.AddMember("z", parentComp.localPos.z, allocator);
                        parentValue.AddMember("localPos", localPosVal, allocator);

                        // 3. Local Rotation
                        parentValue.AddMember("localRot", parentComp.localRot, allocator);

                        // 4. Local Scale
                        rapidjson::Value localScaleVal(rapidjson::kObjectType);
                        localScaleVal.AddMember("x", parentComp.localScale.x, allocator);
                        localScaleVal.AddMember("y", parentComp.localScale.y, allocator);
                        parentValue.AddMember("localScale", localScaleVal, allocator);

                        // 5. Color Inheritance
                        parentValue.AddMember("inheritColor", parentComp.inheritColor, allocator);

                        // 6. Color Multiplier
                        rapidjson::Value colVal(rapidjson::kObjectType);
                        colVal.AddMember("x", parentComp.colorMul.x(), allocator);
                        colVal.AddMember("y", parentComp.colorMul.y(), allocator);
                        colVal.AddMember("z", parentComp.colorMul.z(), allocator);
                        colVal.AddMember("w", parentComp.colorMul.w(), allocator);
                        parentValue.AddMember("colorMul", colVal, allocator);

                        entityValue.AddMember(rapidjson::StringRef(Keys::PARENT_COMPONENT), parentValue, allocator);
                    }
                }

                // Add the finished entity JSON object to the array
                entitiesArray.PushBack(std::move(entityValue), allocator);
            }

            size_t entityCount = entitiesArray.Size();

            // 4. Add the ENTITIES array to the main document
            doc.AddMember(rapidjson::StringRef(Keys::ENTITIES), std::move(entitiesArray), allocator);

            rapidjson::Value layerNames(rapidjson::kObjectType);
            for (auto& layer : LayerComponent::layers) {
                layerNames.AddMember(rapidjson::Value(layer.first.c_str(), allocator).Move()
                    , layer.second, allocator);
            }

            doc.AddMember("layers", std::move(layerNames), allocator);

            // =========================================================
            // 5. Serialize the Document directly to a JSON string
            // (REPLACING sceneAsset->SetSceneData(std::move(doc));)
            // =========================================================
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

            if (!doc.Accept(writer)) {
                std::cout << "ERROR: Failed to serialize RapidJSON document for scene: " << sceneName << std::endl;
                return; // Exit if serialization fails
            }

            // Get the resulting JSON string
            std::string content = buffer.GetString();
            std::string fullPath = Assets::rootPath + "/scenes/" + sceneName;

            // =========================================================
            // 6. Write the string to the file directly
            // (REPLACING sceneAsset->SaveSceneData(fullPath);)
            // =========================================================
            if (WriteTextToFile(fullPath, content)) {
                std::cout << "Successfully saved " << entityCount
                    << " entities to scene: " << fullPath << std::endl;
            }
            else {
                std::cout << "ERROR: Failed to write scene data to file: " << fullPath << std::endl;
            }
        }
    }
}
