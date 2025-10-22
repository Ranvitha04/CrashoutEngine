///*!
//@file   sceneasset.cpp
//@author lee.r@digipen.edu
//=@date   16/09/2025
//
//@brief	This file contains the definition of the Scene class, a rough implementation
//        of asset type that encapsulates a rapidjson value data. The class
//        is designed to be used with the AssetManager and provides a static factory
//        method for safe creation and resource management.
////__________________________________________________________________________*/
////#pragma once
////
/////*                                                                   includes
////----------------------------------------------------------------------------- */
//
//#include "entitymanager.h"
//#include "assetmanager.h"
//#include "collisionmanager.h"
//#include "sceneasset.h"
//#include <fstream>
//#include <sstream>
//#include <iostream>
//
//#include <rapidjson/error/en.h>
//#include <rapidjson/stringbuffer.h>
//#include <rapidjson/writer.h>
//#include <rapidjson/prettywriter.h>
//
//
//// File I/O Util Functions
//namespace util {
//    /*!
//    @brief Reads the entire content of a file into a single string.
//    @param filePath Path to the SceneAsset(JSON) file.
//    @return empty string on failure.
//    */
//    std::string ReadFileToString(const std::string& filePath) {
//        std::ifstream file(filePath);
//        if (!file.is_open()) {
//            std::cout << "ERROR: Could not open file: " << filePath << std::endl;
//            return "";
//        }
//        std::stringstream buffer;
//        buffer << file.rdbuf();
//        return buffer.str();
//    }
//
//    /*!
//    @brief Writes content string to a file
//    @param filePath Path to the SceneAsset(JSON) file.
//    @param string containing data to send
//    @return bool to determine if successful
//    */
//    bool WriteTextToFile(const std::string& filePath, const std::string& content) {
//        std::ofstream file(filePath);
//        if (!file.is_open()) {
//            std::cout << "ERROR: Could not open file for writing: " << filePath << std::endl;
//            return false;
//        }
//        file << content;
//        return true;
//    }
//}
//
//namespace CrashoutsEngine {
//    namespace Assets {
//
//        /*!
//        @brief Creates a new SceneAsset from a json file.
//        @param filePath Path to the SceneAsset(JSON) file.
//        @return Unique pointer to a valid TextureAsset, or nullptr on failure.
//        */
//        std::unique_ptr<SceneAsset> SceneAsset::Create(const std::string& filePath) {
//
//			// Load the JSON file from filePath
//            std::string jsonContent = util::ReadFileToString(filePath);
//            if (jsonContent.empty()) {
//                std::cout << "SceneAsset::Create - Failed to read file: " << filePath << std::endl;
//                return nullptr;
//            }
//
//            // Parse JSON string into RapidJSON document
//            rapidjson::Document doc;
//            doc.Parse(jsonContent.c_str());
//
//            if (doc.HasParseError()) {
//                std::cout << "SceneAsset::Create - Failed to parse JSON in " << filePath << std::endl;
//                std::cout << "ERROR: " << rapidjson::GetParseError_En(doc.GetParseError())
//                    << " at offset " << doc.GetErrorOffset() << std::endl;
//                return nullptr;
//            } 
//
//            if (!doc.IsObject()) {
//                std::cout << "SceneAsset::Create - JSON root is not an object in " << filePath << std::endl;
//                return nullptr;
//            }
//
//            // Extract the scene name by finding last slash
//            size_t lastSlash = filePath.find_last_of("/\\");
//            std::string sceneName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);
//
//            return std::unique_ptr<SceneAsset>(new SceneAsset(sceneName));
//        }
//        
//    }
//
//    namespace Serialization {
//
//        // JSON Structure Keys definition (for clean and centralized key management)
//        namespace Keys {
//            const char* const ENTITIES              = "entities";
//            const char* const ENTITY_ID             = "id";
//            const char* const NAME_COMPONENT        = "name";
//            const char* const TRANSFORM_COMPONENT   = "transform";
//            const char* const RIGIDBODY_COMPONENT   = "rigidbody";
//            const char* const COLLIDER_COMPONENT    = "collider";
//            const char* const SHAPE_COMPONENT       = "shape";
//        }
//
//        /*!
//        @brief Loads entities and their components from a specified scene asset into the ECS.
//        @param sceneName The name of the scene asset to load (e.g., "level1.json").
//        */
//        void LoadScenes(std::string const& sceneName) {
//            Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();
//
//            Assets::SceneAsset* sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);
//
//            if (!sceneAsset) {
//                std::cout << "ERROR: Scene Asset '" << sceneName << "' not found." << std::endl;
//                return;
//            }
//
//            // Cycle through sceneAsset sceneData (JSON document) for testing
//            std::string fullPath = Assets::rootPath + "" + sceneName;
//
//            std::string jsonContent = util::ReadFileToString(fullPath);
//            if (jsonContent.empty()) {
//                std::cout << "ERROR: Failed to read scene file: " << fullPath << std::endl;
//                return;
//            }
//
//            rapidjson::Document doc;
//            doc.Parse(jsonContent.c_str());
//
//            if (doc.HasParseError()) {
//                std::cout << "ERROR: Failed to parse JSON for scene: " << fullPath << std::endl;
//                std::cout << "Parse Error: " << rapidjson::GetParseError_En(doc.GetParseError())
//                    << " at offset " << doc.GetErrorOffset() << std::endl;
//                return;
//            }
//
//            // 5. Use the local document (replaces sceneAsset->GetSceneData())
//            if (!doc.HasMember(Keys::ENTITIES) || !doc[Keys::ENTITIES].IsArray()) {
//                std::cout << "Warning: Scene JSON has no '" << Keys::ENTITIES << "' array. Starting with empty scene." << std::endl;
//                return;
//            }
//
//            const rapidjson::Value& entitiesArray = doc[Keys::ENTITIES];
//
//            ECS& gECS = *ECS::getInstance();
//
//            for (rapidjson::SizeType i = 0; i < entitiesArray.Size(); ++i) {
//                // Create entity obj
//                Entity newEntity = gECS.CreateEntity();
//                // Add default components
//                NameComponent& nameComp = gECS.AddComponent<NameComponent>(newEntity, ("Entity" + std::to_string(i)));
//                TransformComponent& transComp = gECS.AddComponent<TransformComponent>(newEntity);
//
//                const rapidjson::Value& entityValue = entitiesArray[i];
//
//                if (!entityValue.IsObject()) continue;
//
//                // Set Entity Name
//                if (entityValue.HasMember(Keys::NAME_COMPONENT) && entityValue[Keys::NAME_COMPONENT].IsString()) {
//                    nameComp.name = entityValue[Keys::NAME_COMPONENT].GetString();
//                }
//
//                // Set Entity Transform
//                // { "transform": {rot}, {pos}, {scale} }
//                if (entityValue.HasMember(Keys::TRANSFORM_COMPONENT) && entityValue[Keys::TRANSFORM_COMPONENT].IsObject()) {
//                    const rapidjson::Value& transformValue = entityValue[Keys::TRANSFORM_COMPONENT];
//
//                    // 1. Rotation (single float)
//                    float rotation = 0.0f;
//                    if (transformValue.HasMember("rotation") && transformValue["rotation"].IsFloat()) {
//                        rotation = transformValue["rotation"].GetFloat();
//                    }
//
//                    // 2. Position (CEVec3)
//                    CEVec3 pos{ 0.0f, 0.0f, 0.0f };
//                    if (transformValue.HasMember("position") && transformValue["position"].IsObject()) {
//                        const rapidjson::Value& posValue = transformValue["position"];
//                        // Use HasMember check for safety
//                        if (posValue.HasMember("x")) pos.x = posValue["x"].GetFloat();
//                        if (posValue.HasMember("y")) pos.y = posValue["y"].GetFloat();
//                        if (posValue.HasMember("z")) pos.z = posValue["z"].GetFloat();
//                    }
//
//                    // 3. size (CEVec2) - Corrected variable name and keys
//                    CEVec2 size{ 1.0f, 1.0f }; // Initialize to default size
//                    if (transformValue.HasMember("size") && transformValue["size"].IsObject()) {
//                        const rapidjson::Value& sizeValue = transformValue["size"];
//                        // Use HasMember check for safety
//                        if (sizeValue.HasMember("x")) size.x = sizeValue["x"].GetFloat();
//                        if (sizeValue.HasMember("y")) size.y = sizeValue["y"].GetFloat();
//                    }
//               
//                    transComp.position = pos;
//                    transComp.rotation = rotation;
//                    transComp.size = size;
//                }
//            
//                // Check if Rigid-Body
//                if (entityValue.HasMember(Keys::RIGIDBODY_COMPONENT) && entityValue[Keys::RIGIDBODY_COMPONENT].IsBool()) {
//                    if (entityValue[Keys::RIGIDBODY_COMPONENT].GetBool()) {
//                        gECS.AddComponent<RigidBodyComponent>(newEntity);
//                    }
//                }
//
//                // Check if Collider 
//                if (entityValue.HasMember(Keys::COLLIDER_COMPONENT) && entityValue[Keys::COLLIDER_COMPONENT].IsObject()) {
//                    const rapidjson::Value& colliderValue = entityValue[Keys::COLLIDER_COMPONENT];
//
//                    // 1. Check Type Int
//                    //int colliderType = 0;
//                    if (colliderValue.HasMember("type") && colliderValue["type"].IsInt()) {
//                        // 0 - Sphere Collider, 1 - AABB, 2 - OBB?
//                        int colliderType = colliderValue["type"].GetInt();
//                        ColliderComponent& col = gECS.AddComponent<ColliderComponent>(newEntity);
//
//                        switch (colliderType) {
//                        case 0:     
//                            // Sphere collider (TENTATIVELY NOT ACTIVE)
//                            //if (colliderValue.HasMember("sphere") && colliderValue["sphere"].IsObject()) {
//                            //    const rapidjson::Value& sphereValue = colliderValue["sphere"];
//
//                            //    float radius = 1.f;
//                            //    if (sphereValue.HasMember("radius") && sphereValue["radius"].IsFloat()) {
//                            //        radius = sphereValue["radius"].GetFloat();
//                            //    }
//
//                            //    CEVec2 center{ 0.0f, 0.0f }; // Initialize to default size
//                            //    if (sphereValue.HasMember("center") && sphereValue["center"].IsObject()) {
//                            //        const rapidjson::Value& centerValue = sphereValue["center"];
//                            //        // Use HasMember check for safety
//                            //        if (centerValue.HasMember("x")) center.x = centerValue["x"].GetFloat();
//                            //        if (centerValue.HasMember("y")) center.y = centerValue["y"].GetFloat();
//                            //    }
//                            //
//                            //    col.collider->type = Collision::SPHERE;
//                            //}
//                            col.collider->type = Collision::SPHERE;
//                            break;
//                        case 1:     // AABB
//                            col.collider->type = Collision::AABB;
//                            break;
//                        default:
//                            col.collider->type = Collision::AABB;
//                        };
//                    }
//                }
//            
//                // Check if Shape
//                if (entityValue.HasMember(Keys::SHAPE_COMPONENT) && entityValue[Keys::SHAPE_COMPONENT].IsObject()) {
//                    ShapeComponent& s = gECS.AddComponent<ShapeComponent>(newEntity);
//                    const rapidjson::Value& shapeValues = entityValue[Keys::SHAPE_COMPONENT];
//
//                    // enable texture bool
//                    if (shapeValues.HasMember("useTexture") && shapeValues["useTexture"].IsBool()) {
//                        s.enableTexture = shapeValues["useTexture"].GetBool();
//                    }
//
//                    // Set wrap mode int
//                    if (shapeValues.HasMember("wrapModeU") && shapeValues["wrapModeU"].IsInt()) {
//                        s.wrapModeU = shapeValues["wrapModeU"].GetInt();
//                    }
//                    if (shapeValues.HasMember("wrapModeV") && shapeValues["wrapModeV"].IsInt()) {
//                        s.wrapModeV = shapeValues["wrapModeV"].GetInt();
//                    }
//
//                    // CEVec2 uvSize
//                    if (shapeValues.HasMember("uvSize") && shapeValues["uvSize"].IsObject()) {
//                        CEVec2 uvSize{ 1.0f, 1.0f }; // Initialize to default size
//                        const rapidjson::Value& sizeValue = shapeValues["uvSize"];
//                        // Use HasMember check for safety
//                        if (sizeValue.HasMember("x")) uvSize.x = sizeValue["x"].GetFloat();
//                        if (sizeValue.HasMember("y")) uvSize.y = sizeValue["y"].GetFloat();
//
//                        s.uvSize = uvSize;
//                    }
//
//                    // Set texture name
//                    if (shapeValues.HasMember("textureName") && shapeValues["textureName"].IsString()) {
//                        s.textureName = shapeValues["textureName"].GetString();
//                    }
//
//                    // Set primitive type
//                    if (shapeValues.HasMember("type") && shapeValues["type"].IsInt()) {
//                        switch (shapeValues["type"].GetInt()) {
//                        case 0:
//                            s.type = ShapeType::Square;
//                            break;
//                        case 1:
//                            s.type = ShapeType::Rectangle;
//                            break;
//                        case 2:
//                            s.type = ShapeType::Line;
//                            break;
//                        case 3:
//                            s.type = ShapeType::Triangle;
//                            break;
//                        default:
//                            s.type = ShapeType::Rectangle;
//                            break;
//                        };
//                    }
//                
//                    // Color
//                    if (shapeValues.HasMember("color") && shapeValues["color"].IsObject()) {
//                        CEVec3 color{ 0.0f, 0.0f, 0.0f };
//                        const rapidjson::Value& colorValue = shapeValues["color"];
//                        // Use HasMember check for safety
//                        if (colorValue.HasMember("x")) color.x = colorValue["x"].GetFloat();
//                        if (colorValue.HasMember("y")) color.y = colorValue["y"].GetFloat();
//                        if (colorValue.HasMember("z")) color.z = colorValue["z"].GetFloat();
//
//                        s.color = color;
//                    }
//
//                    // Size
//                    if (shapeValues.HasMember("size") && shapeValues["size"].IsFloat()) {
//                        s.size = shapeValues["size"].GetFloat();
//                    }
//
//                    // Width n Height
//                    // Size - Width
//                    if (shapeValues.HasMember("width") && shapeValues["width"].IsFloat()) {
//                        s.width = shapeValues["width"].GetFloat();
//                    }
//                    // Size - height
//                    if (shapeValues.HasMember("height") && shapeValues["height"].IsFloat()) {
//                        s.size = shapeValues["height"].GetFloat();
//                    }
//                
//                    // p1
//                    if (shapeValues.HasMember("p1") && shapeValues["p1"].IsObject()) {
//                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
//                        const rapidjson::Value& p1Value = shapeValues["p1"];
//                        // Use HasMember check for safety
//                        if (p1Value.HasMember("x")) pos.x = p1Value["x"].GetFloat();
//                        if (p1Value.HasMember("y")) pos.y = p1Value["y"].GetFloat();
//                        if (p1Value.HasMember("z")) pos.z = p1Value["z"].GetFloat();
//
//                        s.p1 = pos;
//                    }
//                    // p2
//                    if (shapeValues.HasMember("p2") && shapeValues["p2"].IsObject()) {
//                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
//                        const rapidjson::Value& p2Value = shapeValues["p2"];
//                        // Use HasMember check for safety
//                        if (p2Value.HasMember("x")) pos.x = p2Value["x"].GetFloat();
//                        if (p2Value.HasMember("y")) pos.y = p2Value["y"].GetFloat();
//                        if (p2Value.HasMember("z")) pos.z = p2Value["z"].GetFloat();
//
//                        s.p2 = pos;
//                    }
//
//                    // Thickness
//                    if (shapeValues.HasMember("thickness") && shapeValues["thickness"].IsFloat()) {
//                        s.thickness = shapeValues["thickness"].GetFloat();
//                    }
//
//                    // t1
//                    if (shapeValues.HasMember("t1") && shapeValues["t1"].IsObject()) {
//                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
//                        const rapidjson::Value& t1Value = shapeValues["t1"];
//                        // Use HasMember check for safety
//                        if (t1Value.HasMember("x")) pos.x = t1Value["x"].GetFloat();
//                        if (t1Value.HasMember("y")) pos.y = t1Value["y"].GetFloat();
//                        if (t1Value.HasMember("z")) pos.z = t1Value["z"].GetFloat();
//
//                        s.t1 = pos;
//                    }
//                    // t2
//                    if (shapeValues.HasMember("t2") && shapeValues["t2"].IsObject()) {
//                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
//                        const rapidjson::Value& t2Value = shapeValues["t2"];
//                        // Use HasMember check for safety
//                        if (t2Value.HasMember("x")) pos.x = t2Value["x"].GetFloat();
//                        if (t2Value.HasMember("y")) pos.y = t2Value["y"].GetFloat();
//                        if (t2Value.HasMember("z")) pos.z = t2Value["z"].GetFloat();
//
//                        s.t2 = pos;
//                    }
//                    // t3
//                    if (shapeValues.HasMember("t3") && shapeValues["t3"].IsObject()) {
//                        CEVec3 pos{ 0.0f, 0.0f, 0.0f };
//                        const rapidjson::Value& t3Value = shapeValues["t3"];
//                        // Use HasMember check for safety
//                        if (t3Value.HasMember("x")) pos.x = t3Value["x"].GetFloat();
//                        if (t3Value.HasMember("y")) pos.y = t3Value["y"].GetFloat();
//                        if (t3Value.HasMember("z")) pos.z = t3Value["z"].GetFloat();
//
//                        s.t3 = pos;
//                    }
//                
//                    // Preview
//                    if (shapeValues.HasMember("preview") && shapeValues["preview"].IsBool()) {
//                        s.preview = shapeValues["preview"].GetBool();
//                    }
//                }
//            }
//        }
//
//        /*!
//        @brief Saves all current entities and their components from the ECS into a JSON scene file.
//        @param sceneName The name of the scene asset to save to (e.g., "level1.json").
//        */
//        void SaveScenes(std::string const& sceneName) {
//            ECS& gECS = *ECS::getInstance();
//            Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();
//
//            // 1. Get or create the SceneAsset to store the data
//            Assets::SceneAsset* sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);
//            if (!sceneAsset) {
//                // Handle case where the scene asset doesn't exist (e.g., create a new one)
//                Assets::SceneAsset::Create(sceneName);
//                sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);
//            }
//
//            // 2. Prepare the rapidjson Document
//            rapidjson::Document doc;
//            doc.SetObject();
//
//            // rapidjson needs an allocator for creating values
//            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
//
//            // Create the top-level ENTITIES array
//            rapidjson::Value entitiesArray(rapidjson::kArrayType);
//
//            std::cout << "Saving entities for scene: " << sceneName << "..." << std::endl;
//
//
//            // Cycle through entities
//            for (Entity const& id : gECS.GetComponentArray<Component::NameComponent>().indexToEntity) {
//                
//                // Save name component data into document
//
//                // Save transform component data into document if it exists
//                rapidjson::Value entityValue(rapidjson::kObjectType);
//
//                // =========================================================
//                // A. NameComponent
//                // =========================================================
//                if (gECS.HasComponent<NameComponent>(id)) {
//                    const NameComponent& nameComp = *gECS.GetComponent<NameComponent>(id);
//                    rapidjson::Value nameValue(nameComp.name.c_str(), allocator);
//                    entityValue.AddMember(
//                        rapidjson::StringRef(Keys::NAME_COMPONENT), // StringRef for the key
//                        rapidjson::Value(nameComp.name.c_str(), allocator), // Creates an rvalue for the value
//                        allocator
//                    );
//                }
//
//                // =========================================================
//                // B. TransformComponent
//                // =========================================================
//                if (gECS.HasComponent<TransformComponent>(id)) {
//                    const TransformComponent& transComp = *gECS.GetComponent<TransformComponent>(id);
//                    rapidjson::Value transformValue(rapidjson::kObjectType);
//
//                    // 1. Rotation
//                    transformValue.AddMember("rotation", transComp.rotation, allocator);
//
//                    // 2. Position (CEVec3 -> JSON Object)
//                    rapidjson::Value posValue(rapidjson::kObjectType);
//                    posValue.AddMember("x", transComp.position.x, allocator);
//                    posValue.AddMember("y", transComp.position.y, allocator);
//                    posValue.AddMember("z", transComp.position.z, allocator);
//                    transformValue.AddMember("position", posValue, allocator);
//
//                    // 3. Size (CEVec2 -> JSON Object)
//                    rapidjson::Value sizeValue(rapidjson::kObjectType);
//                    sizeValue.AddMember("x", transComp.size.x, allocator);
//                    sizeValue.AddMember("y", transComp.size.y, allocator);
//                    transformValue.AddMember("size", sizeValue, allocator);
//
//                    entityValue.AddMember(rapidjson::StringRef(Keys::TRANSFORM_COMPONENT),
//                        std::move(transformValue), allocator);
//                }
//
//                // =========================================================
//                // C. RigidBodyComponent
//                // =========================================================
//                if (gECS.HasComponent<RigidBodyComponent>(id)) {
//                    // RigidBody is stored as a boolean 'true' if the component exists
//                    entityValue.AddMember(rapidjson::StringRef(Keys::RIGIDBODY_COMPONENT), true, allocator);
//                }
//
//                // =========================================================
//                // D. ColliderComponent
//                // =========================================================
//                if (gECS.HasComponent<ColliderComponent>(id)) {
//                    const ColliderComponent& colComp = *gECS.GetComponent<ColliderComponent>(id);
//                    rapidjson::Value colliderValue(rapidjson::kObjectType);
//
//                    // 1. Type (integer)
//                    // Assuming Collision::SPHERE = 0, Collision::AABB = 1
//                    colliderValue.AddMember("type", (int)colComp.collider->type, allocator);
//
//                    // NOTE: You can skip adding the 'sphere' or 'aabb' objects 
//                    // if you don't save their properties (like radius or extents), 
//                    // as your LoadScenes only reads the 'type' for now.
//                    // If you later save extents/radius, they would go here.
//
//                    entityValue.AddMember(rapidjson::StringRef(Keys::COLLIDER_COMPONENT), colliderValue, allocator);
//                }
//
//                // =========================================================
//                // E. ShapeComponent
//                // =========================================================
//                if (gECS.HasComponent<ShapeComponent>(id)) {
//                    const ShapeComponent& sComp = *gECS.GetComponent<ShapeComponent>(id);
//                    rapidjson::Value shapeValues(rapidjson::kObjectType);
//
//                    // Boolean, Integers, Floats
//                    shapeValues.AddMember("useTexture", sComp.enableTexture, allocator);
//                    shapeValues.AddMember("wrapModeU", (int)sComp.wrapModeU, allocator);
//                    shapeValues.AddMember("wrapModeV", (int)sComp.wrapModeV, allocator);
//                    shapeValues.AddMember("size", sComp.size, allocator);
//                    shapeValues.AddMember("width", sComp.width, allocator);
//                    shapeValues.AddMember("height", sComp.height, allocator);
//                    shapeValues.AddMember("thickness", sComp.thickness, allocator);
//                    shapeValues.AddMember("preview", sComp.preview, allocator);
//
//                    // ShapeType (integer)
//                    shapeValues.AddMember("type", (int)sComp.type, allocator);
//
//                    // Texture Name (string)
//                    rapidjson::Value textureNameValue(sComp.textureName.c_str(), allocator);
//                    shapeValues.AddMember("textureName", textureNameValue, allocator);
//
//                    // CEVec2 uvSize
//                    rapidjson::Value uvSizeValue(rapidjson::kObjectType);
//                    uvSizeValue.AddMember("x", sComp.uvSize.x, allocator);
//                    uvSizeValue.AddMember("y", sComp.uvSize.y, allocator);
//                    shapeValues.AddMember("uvSize", uvSizeValue, allocator);
//
//                    // CEVec3 Color
//                    rapidjson::Value colorValue(rapidjson::kObjectType);
//                    colorValue.AddMember("x", sComp.color.x, allocator);
//                    colorValue.AddMember("y", sComp.color.y, allocator);
//                    colorValue.AddMember("z", sComp.color.z, allocator);
//                    shapeValues.AddMember("color", colorValue, allocator);
//
//                    // Line/Triangle points (p1, p2, t1, t2, t3)
//                    // NOTE: Your LoadScenes has a bug: it tries to read t1, t2, t3 even if the shape is a Line (which uses p1, p2). 
//                    // We save all to be robust.
//
//                    // p1
//                    rapidjson::Value p1Value(rapidjson::kObjectType);
//                    p1Value.AddMember("x", sComp.p1.x, allocator);
//                    p1Value.AddMember("y", sComp.p1.y, allocator);
//                    p1Value.AddMember("z", sComp.p1.z, allocator);
//                    shapeValues.AddMember("p1", p1Value, allocator);
//
//                    // p2
//                    rapidjson::Value p2Value(rapidjson::kObjectType);
//                    p2Value.AddMember("x", sComp.p2.x, allocator);
//                    p2Value.AddMember("y", sComp.p2.y, allocator);
//                    p2Value.AddMember("z", sComp.p2.z, allocator);
//                    shapeValues.AddMember("p2", p2Value, allocator);
//
//                    // t1
//                    rapidjson::Value t1Value(rapidjson::kObjectType);
//                    t1Value.AddMember("x", sComp.t1.x, allocator);
//                    t1Value.AddMember("y", sComp.t1.y, allocator);
//                    t1Value.AddMember("z", sComp.t1.z, allocator);
//                    shapeValues.AddMember("t1", t1Value, allocator);
//
//                    // t2
//                    rapidjson::Value t2Value(rapidjson::kObjectType);
//                    t2Value.AddMember("x", sComp.t2.x, allocator);
//                    t2Value.AddMember("y", sComp.t2.y, allocator);
//                    t2Value.AddMember("z", sComp.t2.z, allocator);
//                    shapeValues.AddMember("t2", t2Value, allocator);
//
//                    // t3
//                    rapidjson::Value t3Value(rapidjson::kObjectType);
//                    t3Value.AddMember("x", sComp.t3.x, allocator);
//                    t3Value.AddMember("y", sComp.t3.y, allocator);
//                    t3Value.AddMember("z", sComp.t3.z, allocator);
//                    shapeValues.AddMember("t3", t3Value, allocator);
//
//                    entityValue.AddMember(rapidjson::StringRef(Keys::SHAPE_COMPONENT), shapeValues, allocator);
//                }
//
//                // Add the finished entity JSON object to the array
//                entitiesArray.PushBack(std::move(entityValue), allocator);
//            }
//
//            size_t entityCount = entitiesArray.Size();
//
//            // 4. Add the ENTITIES array to the main document
//            doc.AddMember(rapidjson::StringRef(Keys::ENTITIES), std::move(entitiesArray), allocator);
//
//            // =========================================================
//            // 5. Serialize the Document directly to a JSON string
//            // (REPLACING sceneAsset->SetSceneData(std::move(doc));)
//            // =========================================================
//            rapidjson::StringBuffer buffer;
//            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
//
//            if (!doc.Accept(writer)) {
//                std::cout << "ERROR: Failed to serialize RapidJSON document for scene: " << sceneName << std::endl;
//                return; // Exit if serialization fails
//            }
//
//            // Get the resulting JSON string
//            std::string content = buffer.GetString();
//            std::string fullPath = Assets::rootPath + "" + sceneName;
//
//            // =========================================================
//            // 6. Write the string to the file directly
//            // (REPLACING sceneAsset->SaveSceneData(fullPath);)
//            // =========================================================
//            if (util::WriteTextToFile(fullPath, content)) {
//                std::cout << "Successfully saved " << entityCount
//                    << " entities to scene: " << fullPath << std::endl;
//            }
//            else {
//                std::cout << "ERROR: Failed to write scene data to file: " << fullPath << std::endl;
//            }
//        }
//    }
//}
