/*!
@file   sceneasset.cpp
@author lee.r@digipen.edu
=@date   16/09/2025

@brief	This file contains the definition of the Scene class, a rough implementation
        of asset type that encapsulates a rapidjson value data. The class
        is designed to be used with the AssetManager and provides a static factory
        method for safe creation and resource management.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/
//#pragma once
//
///*                                                                   includes
//----------------------------------------------------------------------------- */
#include "engine_pch.h"

#include "spriteAnimation.h"

#include "entitymanager.h"
#include "assetmanager.h"
#include "collisionmanager.h"
#include "monomanager.h"
#include "sceneasset.h"
#include "CEVec3.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <utility>
#include <filesystem>
#include "keyframe.h"
#include <unordered_map> 
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
#include "hierarchycomponent.h"
#include "transientcomponent.h"
#include "particleComponent.h"
#include "prefabcomponent.h"


// FOR GAME GUI
#include "buttoncomponent.h"
#include "guicomponent.h"
#include "textinputcomponent.h"
#include "uilayoutcomponent.h"

// Script + Reflection
#include <mono/metadata/reflection.h>
#include <mono/metadata/attrdefs.h>

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
        std::ofstream file(filePath, std::ios::out | std::ios::trunc); // (optional but recommended)
        if (!file.is_open()) {
            std::cout << "ERROR: Could not open file for writing: " << filePath << std::endl;
            return false;
        }

        file << content;

        file.flush();                 // <-- ADD THIS LINE
        bool ok = file.good();         // <-- ADD THIS LINE
        file.close();                  // <-- ADD THIS LINE

        if (!ok) {                     // <-- ADD THIS BLOCK
            std::cout << "ERROR: Write failed for file: " << filePath << std::endl;
            return false;
        }
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

#if USE_IMGUI_EDITOR
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
#else
                std::cout << "FATAL ERROR: Required scene file missing in release build: " << filePath << std::endl;
                return nullptr;
#endif
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
            const char* const TILEMAP_COMPONENT         = "tilemap";
            const char* const SPRITE_ANIMATOR_COMPONENT = "sprite_animator";
            const char* const KEYFRAME_ANIMATOR_COMPONENT = "keyframe_animator";
            const char* const PARTICLE_COMPONENT = "particle";
            // UI COMPONENT ITEMS
            const char* const BUTTON_COMPONENT          = "button";
            const char* const GUI_COMPONENT             = "gui";
            const char* const TEXT_INPUT_COMPONENT      = "textinput";
            const char* const UI_LAYOUT_COMPONENT       = "uilayout";
            // POST PROCESSING ITEMS
            const char* const PP_COMPONENT              = "postprocessing";
            // HIERACHY ITEMS
            const char* const PARENT_COMPONENT          = "parent";
            const char* const PREFAB_COMPONENT          = "prefab"; 

        }

        namespace JsonHelpers {
            using Allocator = rapidjson::Document::AllocatorType;

            // Serialization
            void WriteVec3(rapidjson::Value& obj, const char* key, const CEVec3& v, Allocator& alloc) {
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("x", v.x, alloc); val.AddMember("y", v.y, alloc); val.AddMember("z", v.z, alloc);
                obj.AddMember(rapidjson::StringRef(key), val, alloc);
            }
            void WriteVec2(rapidjson::Value& obj, const char* key, const CEVec2& v, Allocator& alloc) {
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("x", v.x, alloc); val.AddMember("y", v.y, alloc);
                obj.AddMember(rapidjson::StringRef(key), val, alloc);
            }
            void WriteColor(rapidjson::Value& obj, const char* key, const CEColor& c, Allocator& alloc) {
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("x", c.x(), alloc); val.AddMember("y", c.y(), alloc);
                val.AddMember("z", c.z(), alloc); val.AddMember("w", c.w(), alloc);
                obj.AddMember(rapidjson::StringRef(key), val, alloc);
            }

            // Deserialization
            float GetFloat(const rapidjson::Value& obj, const char* key, float def = 0.0f) {
                if (obj.HasMember(key) && obj[key].IsNumber()) return obj[key].GetFloat();
                return def;
            }
            int GetInt(const rapidjson::Value& obj, const char* key, int def = 0) {
                if (obj.HasMember(key) && obj[key].IsInt()) return obj[key].GetInt();
                return def;
            }
            bool GetBool(const rapidjson::Value& obj, const char* key, bool def = false) {
                if (obj.HasMember(key) && obj[key].IsBool()) return obj[key].GetBool();
                return def;
            }
            const char* GetString(const rapidjson::Value& obj, const char* key, const char* def = "") {
                if (obj.HasMember(key) && obj[key].IsString()) return obj[key].GetString();
                return def;
            }

            CEVec3 ReadVec3(const rapidjson::Value& v, CEVec3 def = {}) {
                if (v.IsObject()) {
                    if (v.HasMember("x")) def.x = v["x"].GetFloat();
                    if (v.HasMember("y")) def.y = v["y"].GetFloat();
                    if (v.HasMember("z")) def.z = v["z"].GetFloat();
                }
                return def;
            }
            CEVec2 ReadVec2(const rapidjson::Value& v, CEVec2 def = {}) {
                if (v.IsObject()) {
                    if (v.HasMember("x")) def.x = v["x"].GetFloat();
                    if (v.HasMember("y")) def.y = v["y"].GetFloat();
                }
                return def;
            }
            CEColor ReadColor(const rapidjson::Value& v, CEColor def = {}) {
                if (v.IsObject()) {
                    if (v.HasMember("x")) def.x() = v["x"].GetFloat();
                    if (v.HasMember("y")) def.y() = v["y"].GetFloat();
                    if (v.HasMember("z")) def.z() = v["z"].GetFloat();
                    if (v.HasMember("w")) def.w() = v["w"].GetFloat();
                }
                return def;
            }
        }

        rapidjson::Value SerializeEntity(uint32_t entityId, rapidjson::Document::AllocatorType& allocator) {
            ECS& gECS = *ECS::getInstance();
            Entity id = static_cast<Entity>(entityId);
            rapidjson::Value entityValue(rapidjson::kObjectType);
            using namespace JsonHelpers;

            if (gECS.HasComponent<NameComponent>(id)) {
                entityValue.AddMember(rapidjson::StringRef(Keys::NAME_COMPONENT), 
                    rapidjson::Value(gECS.GetComponent<NameComponent>(id)->name.c_str(), allocator), allocator);
            }

            if (gECS.HasComponent<LayerComponent>(id)) {
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("currentLayer", gECS.GetComponent<LayerComponent>(id)->currentLayer, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::LAYER_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<TransformComponent>(id)) {
                const auto& c = *gECS.GetComponent<TransformComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("rotation", c.rotation, allocator);
                WriteVec3(val, "position", c.position, allocator);
                WriteVec2(val, "size", c.size, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::TRANSFORM_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<RigidBodyComponent>(id)) {
                auto& c = *gECS.GetComponent<RigidBodyComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("mass", c.getMass(), allocator);
                val.AddMember("restitutionCoeff", c.GetCoeff(), allocator);
                WriteVec3(val, "maximumVelocity", c.getMaxVelocity(), allocator);
                val.AddMember("staticFriction", c.staticFriction, allocator);
                val.AddMember("dynamicFriction", c.dynamicFriction, allocator);
                val.AddMember("inertia", c.inertia, allocator);
                val.AddMember("invInertia", c.invInertia, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::RIGIDBODY_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<ColliderComponent>(id)) {
                const auto& c = *gECS.GetComponent<ColliderComponent>(id);
                if (c.collider) {
                    rapidjson::Value val(rapidjson::kObjectType);
                    val.AddMember("ColliderType", (int)c.collider->ColliderType, allocator);
                    val.AddMember("isTrigger", c.collider->isTrigger, allocator);
                    val.AddMember("enabled", c.collider->enabled, allocator);
                    WriteVec3(val, "position", c.collider->position, allocator);
                    WriteVec3(val, "offset", c.collider->offset, allocator);

                    switch (c.collider->ColliderType) {
                    case Collision::ColliderType::CIRCLE: {
                        auto* s = static_cast<Collision::Circle*>(c.collider.get());
                        val.AddMember("radius", s->radius, allocator);
                        WriteVec3(val, "center", s->position, allocator);
                        break;
                    }
                    case Collision::ColliderType::AABB: {
                        auto* a = static_cast<Collision::AABBRect*>(c.collider.get());
                        WriteVec3(val, "min", a->min, allocator);
                        WriteVec3(val, "max", a->max, allocator);
                        break;
                    }
                    case Collision::ColliderType::OBB: {
                        auto* p = static_cast<Collision::Polygon*>(c.collider.get());
                        CEVec3 extents = Collision::Polygon::CalculateOBBExtents(*p);
                        WriteVec3(val, "min", p->position - extents, allocator);
                        WriteVec3(val, "max", p->position + extents, allocator);
                        val.AddMember("rotation", p->rotation, allocator);
                        break;
                    }
                    case Collision::ColliderType::LINE: {
                        auto* l = static_cast<Collision::LineSegment*>(c.collider.get());
                        // Save the local start and end points
                        WriteVec3(val, "p1", l->p1, allocator);
                        WriteVec3(val, "p2", l->p2, allocator);
                        break;
                    }
                    case Collision::ColliderType::POLYGON:
                    case Collision::ColliderType::TRIANGLE: {
                        auto* p = static_cast<Collision::Polygon*>(c.collider.get());
                        val.AddMember("rotation", p->rotation, allocator);
                        rapidjson::Value verts(rapidjson::kArrayType);
                        for (int k = 0; k < p->vertexCount; ++k) {
                            rapidjson::Value v(rapidjson::kObjectType);
                            v.AddMember("x", p->vertices[k].x, allocator);
                            v.AddMember("y", p->vertices[k].y, allocator);
                            v.AddMember("z", p->vertices[k].z, allocator);
                            verts.PushBack(v, allocator);
                        }
                        val.AddMember("vertices", verts, allocator);
                        break;
                    }
                    default: break;
                    }
                    entityValue.AddMember(rapidjson::StringRef(Keys::COLLIDER_COMPONENT), val, allocator);
                }
            }

            if (gECS.HasComponent<ShapeComponent>(id)) {
                const auto& c = *gECS.GetComponent<ShapeComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("useTexture", c.enableTexture, allocator);
                val.AddMember("isWorldSpace", c.isWorldSpace, allocator);
                val.AddMember("wrapModeU", (int)c.wrapModeU, allocator);
                val.AddMember("wrapModeV", (int)c.wrapModeV, allocator);
                val.AddMember("thickness", c.thickness, allocator);
                val.AddMember("preview", c.preview, allocator);
                val.AddMember("type", (int)c.type, allocator);
                val.AddMember("textureName", rapidjson::Value(c.textureName.c_str(), allocator), allocator);
                WriteVec2(val, "uvSize", c.uvSize, allocator);
                WriteVec2(val, "uvOffset", c.uvOffset, allocator);
                WriteColor(val, "color", c.color, allocator);
                WriteVec3(val, "p1", c.p1, allocator);
                WriteVec3(val, "p2", c.p2, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::SHAPE_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<TextComponent>(id)) {
                const auto& c = *gECS.GetComponent<TextComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("font", rapidjson::Value(c.font.c_str(), allocator), allocator);
                val.AddMember("text", rapidjson::Value(c.text.c_str(), allocator), allocator);
                val.AddMember("scale", c.scale, allocator);
                WriteVec2(val, "offset", c.offset, allocator);
                WriteColor(val, "color", c.color, allocator);
                val.AddMember("centered", c.centered, allocator);
                val.AddMember("isworldspace", c.isWorldSpace, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::TEXT_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<AudioEmitterComponent>(id)) {
                const auto& c = *gECS.GetComponent<AudioEmitterComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("eventPath", rapidjson::Value(c.eventPath.c_str(), allocator), allocator);
                val.AddMember("volume", c.volume, allocator);
                val.AddMember("pitch", c.pitch, allocator);
                val.AddMember("spatial3D", c.spatial3D, allocator);
                val.AddMember("looping", c.looping, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::AUDIO_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<ScriptComponent>(id)) {
                const auto& c = *gECS.GetComponent<ScriptComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                rapidjson::Value arr(rapidjson::kArrayType);
                for (const auto& inst : c.Scripts) {
                    if (inst.ClassName.empty()) continue;
                    rapidjson::Value obj(rapidjson::kObjectType);
                    obj.AddMember("className", rapidjson::Value(inst.ClassName.c_str(), allocator), allocator);
                    if (inst.Instance && inst.Class) {
                        rapidjson::Value fields(rapidjson::kObjectType);
                        void* iter = nullptr;
                        while (MonoClassField* field = mono_class_get_fields(inst.Class, &iter)) {
                            const char* fName = mono_field_get_name(field);
                            uint32_t flags = mono_field_get_flags(field);
                            if (flags & MONO_FIELD_ATTR_STATIC) continue;
                            bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
                            bool hasAttr = false;
                            static MonoClass* attrClass = mono_class_from_name(MonoManager::GetImage(), "CrashoutsEngine", "SerializeField");
                            if (attrClass) {
                                MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(inst.Class, field);
                                if (attrInfo) {
                                    hasAttr = mono_custom_attrs_has_attr(attrInfo, attrClass);
                                    mono_custom_attrs_free(attrInfo);
                                }
                            }
                            if (isPublic || hasAttr) {
                                MonoType* type = mono_field_get_type(field);
                                int tEnum = mono_type_get_type(type);
                                rapidjson::Value key(fName, allocator);
                                if (tEnum == MONO_TYPE_I4) { int v; mono_field_get_value(inst.Instance, field, &v); fields.AddMember(key, v, allocator); }
                                else if (tEnum == MONO_TYPE_R4) { float v; mono_field_get_value(inst.Instance, field, &v); fields.AddMember(key, v, allocator); }
                                else if (tEnum == MONO_TYPE_BOOLEAN) { bool v; mono_field_get_value(inst.Instance, field, &v); fields.AddMember(key, v, allocator); }
                                else if (tEnum == MONO_TYPE_STRING) {
                                    MonoString* ms = nullptr; mono_field_get_value(inst.Instance, field, &ms);
                                    if (ms) { char* s = mono_string_to_utf8(ms); fields.AddMember(key, rapidjson::Value(s, allocator), allocator); mono_free(s); }
                                }
                            }
                        }
                        obj.AddMember("fields", fields, allocator);
                    }
                    arr.PushBack(obj, allocator);
                }
                val.AddMember("scripts", arr, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::SCRIPT_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<CameraComponent>(id)) {
                const auto& c = *gECS.GetComponent<CameraComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("zoom", c.zoom, allocator);
                WriteVec2(val, "xBounds", c.xBounds, allocator);
                WriteVec2(val, "yBounds", c.yBounds, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::CAMERA_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<ButtonComponent>(id)) {
                const auto& c = *gECS.GetComponent<ButtonComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("buttonState", static_cast<std::underlying_type_t<ButtonState>>(c.buttonState), allocator);
                WriteColor(val, "shapeNormalColor", c.shapeNormalColor, allocator);
                WriteColor(val, "shapeHoveredColor", c.shapeHoveredColor, allocator);
                WriteColor(val, "shapePressedColor", c.shapePressedColor, allocator);
                WriteColor(val, "textNormalColor", c.textNormalColor, allocator);
                WriteColor(val, "textHoveredColor", c.textHoveredColor, allocator);
                WriteColor(val, "textPressedColor", c.textPressedColor, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::BUTTON_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<GuiComponent>(id)) {
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("uiType", static_cast<std::underlying_type_t<UIType>>(gECS.GetComponent<GuiComponent>(id)->uiType), allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::GUI_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<TextInputComponent>(id)) {
                const auto& c = *gECS.GetComponent<TextInputComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("isFocused", c.isFocused, allocator);
                val.AddMember("cursorPos", c.cursorPos, allocator);
                val.AddMember("maxCharacters", c.maxCharacters, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::TEXT_INPUT_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<UILayoutComponent>(id)) {
                const auto& c = *gECS.GetComponent<UILayoutComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                WriteVec2(val, "paddingXY", c.paddingXY, allocator);
                val.AddMember("uiLayout", static_cast<std::underlying_type_t<UILayout>>(c.uiLayout), allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::UI_LAYOUT_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<PPComponent>(id)) {
                const auto& c = *gECS.GetComponent<PPComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("brightness", c.brightness, allocator);
                val.AddMember("contrast", c.contrast, allocator);
                val.AddMember("saturation", c.saturation, allocator);
                val.AddMember("active", c.active, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::PP_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<TileMapComponent>(id)) {
                const auto& c = *gECS.GetComponent<TileMapComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("tileSize", c.tileSize, allocator);
                val.AddMember("activeAsset", rapidjson::Value(c.activeAsset.c_str(), allocator), allocator);
                rapidjson::Value assets(rapidjson::kArrayType);
                for (const auto& name : c.tileAssetNames) assets.PushBack(rapidjson::Value(name.c_str(), allocator), allocator);
                val.AddMember("tileAssetNames", assets, allocator);
                rapidjson::Value grid(rapidjson::kArrayType);
                for (const auto& [pos, name] : c.gridData) {
                    rapidjson::Value t(rapidjson::kObjectType);
                    t.AddMember("x", pos.first, allocator); t.AddMember("y", pos.second, allocator);
                    t.AddMember("asset", rapidjson::Value(name.c_str(), allocator), allocator);
                    grid.PushBack(t, allocator);
                }
                val.AddMember("gridData", grid, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::TILEMAP_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<SpriteAnimatorComponent>(id)) {
                const auto& c = *gECS.GetComponent<SpriteAnimatorComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("currentClip", rapidjson::Value(c.currentClipName.c_str(), allocator), allocator);
                val.AddMember("flipH", c.flipH, allocator);
                val.AddMember("playing", c.playing, allocator);
                rapidjson::Value clips(rapidjson::kArrayType);
                for (const auto& clip : c.clips) {
                    rapidjson::Value obj(rapidjson::kObjectType);
                    obj.AddMember("name", rapidjson::Value(clip.name.c_str(), allocator), allocator);
                    obj.AddMember("start", clip.startFrame, allocator);
                    obj.AddMember("count", clip.frameCount, allocator);
                    obj.AddMember("fps", clip.fps, allocator);
                    obj.AddMember("loop", clip.loop, allocator);
                    clips.PushBack(obj, allocator);
                }
                val.AddMember("clips", clips, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::SPRITE_ANIMATOR_COMPONENT), val, allocator);
            }

            if (gECS.HasComponent<Component::ParentComponent>(id)) {
                const auto& c = *gECS.GetComponent<Component::ParentComponent>(id);
                if (c.parent != 0) {
                    rapidjson::Value val(rapidjson::kObjectType);
                    if (gECS.HasComponent<NameComponent>(c.parent)) {
                        val.AddMember("parentName", rapidjson::Value(gECS.GetComponent<NameComponent>(c.parent)->name.c_str(), allocator), allocator);
                    }
                    WriteVec3(val, "localPos", c.localPos, allocator);
                    val.AddMember("localRot", c.localRot, allocator);
                    WriteVec2(val, "localScale", c.localScale, allocator);
                    val.AddMember("inheritColor", c.inheritColor, allocator);
                    WriteColor(val, "colorMul", c.colorMul, allocator);
                    entityValue.AddMember(rapidjson::StringRef(Keys::PARENT_COMPONENT), val, allocator);
                }
            }

            if (gECS.HasComponent<KeyframeAnimatorComponent>(id)) {
                const auto& c = *gECS.GetComponent<KeyframeAnimatorComponent>(id);
                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("current", rapidjson::Value(c.current.c_str(), allocator), allocator);
                val.AddMember("t", c.t, allocator);
                val.AddMember("playing", c.playing, allocator);
                rapidjson::Value clips(rapidjson::kArrayType);
                for (const auto& pair : c.clips) {
                    const auto& clip = pair.second;
                    rapidjson::Value cVal(rapidjson::kObjectType);
                    cVal.AddMember("name", rapidjson::Value(clip.name.c_str(), allocator), allocator);
                    cVal.AddMember("wrap", (int)clip.wrap, allocator);
                    if (!clip.textureAsset.empty()) cVal.AddMember("textureAsset", rapidjson::Value(clip.textureAsset.c_str(), allocator), allocator);
                    cVal.AddMember("spriteCols", clip.spriteCols, allocator);
                    cVal.AddMember("spriteRows", clip.spriteRows, allocator);
                    cVal.AddMember("useCustomSize", clip.useCustomSize, allocator);
                    cVal.AddMember("targetWidth", clip.targetWidth, allocator);
                    cVal.AddMember("targetHeight", clip.targetHeight, allocator);

                    rapidjson::Value bindArr(rapidjson::kArrayType);
                    for (const auto& binding : clip.bindings) {
                        rapidjson::Value bVal(rapidjson::kObjectType);
                        bVal.AddMember("property", (int)binding.property, allocator);
                        rapidjson::Value keys(rapidjson::kArrayType);
                        for (const auto& k : binding.curve.keys) {
                            rapidjson::Value kVal(rapidjson::kObjectType);
                            kVal.AddMember("t", k.time, allocator);
                            kVal.AddMember("v", k.value, allocator);
                            kVal.AddMember("i", (int)k.interp, allocator);
                            kVal.AddMember("in", k.inTan, allocator);
                            kVal.AddMember("out", k.outTan, allocator);
                            keys.PushBack(kVal, allocator);
                        }
                        bVal.AddMember("keys", keys, allocator);
                        bindArr.PushBack(bVal, allocator);
                    }
                    cVal.AddMember("bindings", bindArr, allocator);
                    clips.PushBack(cVal, allocator);
                }
                val.AddMember("clips", clips, allocator);
                entityValue.AddMember(rapidjson::StringRef(Keys::KEYFRAME_ANIMATOR_COMPONENT), val, allocator);
            }
           
            if (gECS.HasComponent<PrefabInstanceComponent>(id)) {
                const auto& p = *gECS.GetComponent<PrefabInstanceComponent>(id);

                rapidjson::Value val(rapidjson::kObjectType);
                val.AddMember("name", rapidjson::Value(p.prefabName.c_str(), allocator), allocator);

                entityValue.AddMember(rapidjson::StringRef(Keys::PREFAB_COMPONENT), val, allocator);
            }

            return entityValue;
        }

        uint32_t DeserializeEntity(const rapidjson::Value& entityValue, std::vector<ParentLinkInfo>* outParentLinks) {
            ECS& gECS = *ECS::getInstance();
            Entity newEntity = gECS.CreateEntity();
            
            NameComponent& nameComp = gECS.AddComponent<NameComponent>(newEntity, ("Entity" + std::to_string(newEntity)));
            gECS.AddComponent<LayerComponent>(newEntity);
            auto& transComp = gECS.AddComponent<TransformComponent>(newEntity);

            if (!entityValue.IsObject()) return newEntity;
            using namespace JsonHelpers;

            // Name
            if (entityValue.HasMember(Keys::NAME_COMPONENT)) {
                const char* n = GetString(entityValue, Keys::NAME_COMPONENT);
                if (n && *n) nameComp.name = n;
            }
            // Prefab
            if (entityValue.HasMember(Keys::PREFAB_COMPONENT)) {
                const auto& v = entityValue[Keys::PREFAB_COMPONENT];

                auto& pc = gECS.AddComponent<PrefabInstanceComponent>(newEntity);

              
                pc.prefabName = JsonHelpers::GetString(v, "name");
            }
            if (entityValue.HasMember(Keys::PREFAB_COMPONENT)) {
                const auto& v = entityValue[Keys::PREFAB_COMPONENT];
                auto& p = gECS.AddComponent<Component::PrefabInstanceComponent>(newEntity);
                p.prefabName = JsonHelpers::GetString(v, "name");
            }

            // Layer
            if (entityValue.HasMember(Keys::LAYER_COMPONENT)) {
                const auto& v = entityValue[Keys::LAYER_COMPONENT];
                if (v.IsUint()) gECS.GetComponent<LayerComponent>(newEntity)->currentLayer = v.GetUint();
                else if (v.IsObject()) gECS.GetComponent<LayerComponent>(newEntity)->currentLayer = GetInt(v, "currentLayer");
            }

            // Transform
            if (entityValue.HasMember(Keys::TRANSFORM_COMPONENT)) {
                const auto& v = entityValue[Keys::TRANSFORM_COMPONENT];
                transComp.rotation = GetFloat(v, "rotation");
                if (v.HasMember("position")) transComp.position = ReadVec3(v["position"]);
                if (v.HasMember("size")) transComp.size = ReadVec2(v["size"], {1.0f, 1.0f});
            }

            // RigidBody
            if (entityValue.HasMember(Keys::RIGIDBODY_COMPONENT)) {
                const auto& v = entityValue[Keys::RIGIDBODY_COMPONENT];
                auto& c = gECS.AddComponent<RigidBodyComponent>(newEntity);
                if (v.HasMember("maximumVelocity")) c.setMaxVelocity(ReadVec3(v["maximumVelocity"]));
                if (v.HasMember("mass")) c.setMass(GetFloat(v, "mass"));
                if (v.HasMember("restitutionCoeff")) c.setRestitution(GetFloat(v, "restitutionCoeff"));
                c.staticFriction = GetFloat(v, "staticFriction");
                c.dynamicFriction = GetFloat(v, "dynamicFriction");
                c.inertia = GetFloat(v, "inertia");
                c.invInertia = GetFloat(v, "invInertia");
            }

            // Collider
            if (entityValue.HasMember(Keys::COLLIDER_COMPONENT)) {
                const auto& v = entityValue[Keys::COLLIDER_COMPONENT];
                auto type = static_cast<Collision::ColliderType>(GetInt(v, "ColliderType", (int)Collision::ColliderType::CIRCLE));
                auto& c = gECS.AddComponent<ColliderComponent>(newEntity);
                
                std::unique_ptr<Collision::Collider> newCol;
                switch (type) {
                case Collision::ColliderType::CIRCLE: {
                    float r = GetFloat(v, "radius", 1.0f);
                    CEVec3 cen = ReadVec3(v["center"]);
                    auto s = std::make_unique<Collision::Circle>(r, cen);
                    s->ColliderType = Collision::ColliderType::CIRCLE;
                    newCol = std::move(s);
                    break;
                }
                case Collision::ColliderType::AABB: {
                    auto s = std::make_unique<Collision::AABBRect>(ReadVec3(v["min"]), ReadVec3(v["max"]));
                    s->ColliderType = Collision::ColliderType::AABB;
                    newCol = std::move(s);
                    break;
                }
                case Collision::ColliderType::OBB: {
                    float rot = GetFloat(v, "rotation");
                    auto s = std::make_unique<Collision::Polygon>(Collision::Polygon::CreateOBB(ReadVec3(v["min"]), ReadVec3(v["max"]), rot, CEVec3{}));
                    s->position = ReadVec3(v["position"]);
                    s->ColliderType = Collision::ColliderType::OBB;
                    newCol = std::move(s);
                    break;
                }
                case Collision::ColliderType::LINE: {
                    CEVec3 p1{ -0.5f, 0.0f, 0.0f }; // Default start
                    CEVec3 p2{ 0.5f, 0.0f, 0.0f }; // Default end

                    if (v.HasMember("p1")) p1 = ReadVec3(v["p1"]);
                    if (v.HasMember("p2")) p2 = ReadVec3(v["p2"]);

                    auto l = std::make_unique<Collision::LineSegment>(p1, p2);
                    l->ColliderType = Collision::ColliderType::LINE;

                    // Position/Offset logic is already handled after the switch in your file
                    newCol = std::move(l);
                    break;
                }
                case Collision::ColliderType::POLYGON:
                case Collision::ColliderType::TRIANGLE: {
                    std::vector<CEVec3> verts;
                    if (v.HasMember("vertices") && v["vertices"].IsArray()) {
                        for (const auto& vert : v["vertices"].GetArray()) verts.push_back(ReadVec3(vert));
                    }
                    if (verts.empty()) verts = {{-0.5f,-0.5f,0}, {0.5f,-0.5f,0}, {0.5f,0.5f,0}};
                    auto s = std::make_unique<Collision::Polygon>();
                    s->vertexCount = std::min((int)verts.size(), Collision::Polygon::MAX_VERTEX);
                    for (int k = 0; k < s->vertexCount; ++k) s->vertices[k] = verts[k];
                    s->rotation = GetFloat(v, "rotation");
                    s->position = ReadVec3(v["position"]);
                    s->offset = ReadVec3(v["offset"]);
                    s->ForceUpdateRotation(s->rotation);
                    s->ColliderType = type;
                    newCol = std::move(s);
                    break;
                }
                default: break;
                }

                if (newCol) {
                    if (v.HasMember("offset")) newCol->offset = ReadVec3(v["offset"]);
                    if (v.HasMember("position")) newCol->position = ReadVec3(v["position"]);
                    newCol->isTrigger = GetBool(v, "isTrigger");
                    newCol->enabled = GetBool(v, "enabled");
                    c.collider = std::move(newCol);
                }
            }

            // Shape
            if (entityValue.HasMember(Keys::SHAPE_COMPONENT)) {
                const auto& v = entityValue[Keys::SHAPE_COMPONENT];
                auto& s = gECS.AddComponent<ShapeComponent>(newEntity);
                s.enableTexture = GetBool(v, "useTexture");
                s.isWorldSpace = GetBool(v, "isWorldSpace");
                s.wrapModeU = GetInt(v, "wrapModeU");
                s.wrapModeV = GetInt(v, "wrapModeV");
                if (v.HasMember("uvSize")) s.uvSize = ReadVec2(v["uvSize"]);
                if (v.HasMember("uvOffset")) s.uvOffset = ReadVec2(v["uvOffset"]);
                s.textureName = GetString(v, "textureName");
                int t = GetInt(v, "type");
                s.type = (t >= 0 && t <= 3) ? static_cast<ShapeType>(t) : ShapeType::Rectangle;
                if (v.HasMember("color")) s.color = ReadColor(v["color"]);
                if (v.HasMember("p1")) s.p1 = ReadVec3(v["p1"]);
                if (v.HasMember("p2")) s.p2 = ReadVec3(v["p2"]);
                s.thickness = GetFloat(v, "thickness");
                s.preview = GetBool(v, "preview");
            }

            // Text
            if (entityValue.HasMember(Keys::TEXT_COMPONENT)) {
                const auto& v = entityValue[Keys::TEXT_COMPONENT];
                auto& t = gECS.AddComponent<TextComponent>(newEntity);
                t.font = GetString(v, "font");
                t.text = GetString(v, "text");
                t.scale = GetFloat(v, "scale");
                if (v.HasMember("offset")) t.offset = ReadVec2(v["offset"]);
                if (v.HasMember("color")) t.color = ReadColor(v["color"]);
                t.centered = GetBool(v, "centered");
                t.isWorldSpace = GetBool(v, "isworldspace");
            }

            // Audio
            if (entityValue.HasMember(Keys::AUDIO_COMPONENT)) {
                const auto& v = entityValue[Keys::AUDIO_COMPONENT];
                auto& a = gECS.AddComponent<AudioEmitterComponent>(newEntity);
                a.eventPath = GetString(v, "eventPath");
                a.volume = GetFloat(v, "volume");
                a.pitch = GetFloat(v, "pitch");
                a.spatial3D = GetBool(v, "spatial3D");
                a.looping = GetBool(v, "looping");
            }

            // Script
            if (entityValue.HasMember(Keys::SCRIPT_COMPONENT)) {
                const auto& v = entityValue[Keys::SCRIPT_COMPONENT];
                auto& s = gECS.AddComponent<ScriptComponent>(newEntity);
                s.EntityID = static_cast<uint32_t>(newEntity);
                if (v.HasMember("scripts") && v["scripts"].IsArray()) {
                    for (const auto& item : v["scripts"].GetArray()) {
                        if (item.IsString()) s.AddScript(item.GetString());
                        else if (item.IsObject() && item.HasMember("className")) s.AddScript(item["className"].GetString());
                    }
                }
                if (MonoManager::GetDomain() && MonoManager::GetAssembly()) s.Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
                if (v.HasMember("scripts") && v["scripts"].IsArray()) {
                    const auto& arr = v["scripts"];
                    for (rapidjson::SizeType k = 0; k < arr.Size(); ++k) {
                        if (k >= s.Scripts.size()) break;
                        auto& inst = s.Scripts[k];
                        const auto& item = arr[k];
                        if (item.IsObject() && item.HasMember("fields") && inst.Instance && inst.Class) {
                            const auto& fields = item["fields"];
                            for (auto it = fields.MemberBegin(); it != fields.MemberEnd(); ++it) {
                                MonoClassField* field = mono_class_get_field_from_name(inst.Class, it->name.GetString());
                                if (field) {
                                    int typeEnum = mono_type_get_type(mono_field_get_type(field));
                                    if (typeEnum == MONO_TYPE_I4 && it->value.IsInt()) { int val = it->value.GetInt(); mono_field_set_value(inst.Instance, field, &val); }
                                    else if (typeEnum == MONO_TYPE_R4 && it->value.IsNumber()) { float val = it->value.GetFloat(); mono_field_set_value(inst.Instance, field, &val); }
                                    else if (typeEnum == MONO_TYPE_BOOLEAN && it->value.IsBool()) { bool val = it->value.GetBool(); mono_field_set_value(inst.Instance, field, &val); }
                                    else if (typeEnum == MONO_TYPE_STRING && it->value.IsString()) {
                                        MonoString* str = mono_string_new(MonoManager::GetDomain(), it->value.GetString());
                                        mono_field_set_value(inst.Instance, field, str);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Camera
            if (entityValue.HasMember(Keys::CAMERA_COMPONENT)) {
                const auto& v = entityValue[Keys::CAMERA_COMPONENT];
                auto& c = gECS.AddComponent<CameraComponent>(newEntity);
                c.zoom = GetFloat(v, "zoom");
                if (v.HasMember("xBounds")) c.xBounds = ReadVec2(v["xBounds"]);
                if (v.HasMember("yBounds")) c.yBounds = ReadVec2(v["yBounds"]);
            }

            // Button
            if (entityValue.HasMember(Keys::BUTTON_COMPONENT)) {
                const auto& v = entityValue[Keys::BUTTON_COMPONENT];
                auto& b = gECS.AddComponent<ButtonComponent>(newEntity);
                if (v.HasMember("buttonState")) b.buttonState = ButtonState::NORMAL;
                if (v.HasMember("shapeNormalColor")) b.shapeNormalColor = ReadColor(v["shapeNormalColor"]);
                if (v.HasMember("shapeHoveredColor")) b.shapeHoveredColor = ReadColor(v["shapeHoveredColor"]);
                if (v.HasMember("shapePressedColor")) b.shapePressedColor = ReadColor(v["shapePressedColor"]);
                if (v.HasMember("textNormalColor")) b.textNormalColor = ReadColor(v["textNormalColor"]);
                if (v.HasMember("textHoveredColor")) b.textHoveredColor = ReadColor(v["textHoveredColor"]);
                if (v.HasMember("textPressedColor")) b.textPressedColor = ReadColor(v["textPressedColor"]);
                if (auto* s = gECS.GetComponent<ShapeComponent>(newEntity)) s->color = b.shapeNormalColor;
                if (auto* t = gECS.GetComponent<TextComponent>(newEntity)) t->color = b.textNormalColor;
            }

            // GUI
            if (entityValue.HasMember(Keys::GUI_COMPONENT)) {
                 gECS.AddComponent<GuiComponent>(newEntity).uiType = static_cast<UIType>(GetInt(entityValue[Keys::GUI_COMPONENT], "uiType"));
            }

            // TextInput
            if (entityValue.HasMember(Keys::TEXT_INPUT_COMPONENT)) {
                const auto& v = entityValue[Keys::TEXT_INPUT_COMPONENT];
                auto& t = gECS.AddComponent<TextInputComponent>(newEntity);
                t.isFocused = GetBool(v, "isFocused");
                t.cursorPos = GetInt(v, "cursorPos");
                t.maxCharacters = GetInt(v, "maxCharacters");
            }

            // UILayout
            if (entityValue.HasMember(Keys::UI_LAYOUT_COMPONENT)) {
                const auto& v = entityValue[Keys::UI_LAYOUT_COMPONENT];
                auto& u = gECS.AddComponent<UILayoutComponent>(newEntity);
                if (v.HasMember("paddingXY")) u.paddingXY = ReadVec2(v["paddingXY"]);
                u.uiLayout = static_cast<UILayout>(GetInt(v, "uiLayout"));
            }

            // PostProcessing
            if (entityValue.HasMember(Keys::PP_COMPONENT)) {
                const auto& v = entityValue[Keys::PP_COMPONENT];
                auto& p = gECS.AddComponent<PPComponent>(newEntity);
                p.brightness = GetFloat(v, "brightness");
                p.contrast = GetFloat(v, "contrast");
                p.saturation = GetFloat(v, "saturation");
                p.active = GetBool(v, "active");
            }

            // TileMap
            if (entityValue.HasMember(Keys::TILEMAP_COMPONENT)) {
                const auto& v = entityValue[Keys::TILEMAP_COMPONENT];
                auto& t = gECS.AddComponent<TileMapComponent>(newEntity);
                t.activeAsset = "none"; t.pOpen = false; t.isCacheDirty = true;
                t.tileSize = GetInt(v, "tileSize");
                t.activeAsset = GetString(v, "activeAsset");
                if (v.HasMember("tileAssetNames") && v["tileAssetNames"].IsArray()) {
                    for (const auto& a : v["tileAssetNames"].GetArray()) if (a.IsString()) t.tileAssetNames.push_back(a.GetString());
                }
                if (v.HasMember("gridData") && v["gridData"].IsArray()) {
                    for (const auto& item : v["gridData"].GetArray()) {
                        if (item.IsObject() && item.HasMember("x") && item.HasMember("y") && item.HasMember("asset")) {
                            t.gridData[{item["x"].GetInt(), item["y"].GetInt()}] = item["asset"].GetString();
                        }
                    }
                }
            }

            // SpriteAnimator
            if (entityValue.HasMember(Keys::SPRITE_ANIMATOR_COMPONENT)) {
                const auto& v = entityValue[Keys::SPRITE_ANIMATOR_COMPONENT];
                auto& c = gECS.AddComponent<SpriteAnimatorComponent>(newEntity);
                c.currentClipName = GetString(v, "currentClip");
                c.flipH = GetBool(v, "flipH");
                c.playing = GetBool(v, "playing");
                if (v.HasMember("clips") && v["clips"].IsArray()) {
                    for (const auto& cl : v["clips"].GetArray()) {
                        c.AddClip(GetString(cl, "name"), GetInt(cl, "start"), GetInt(cl, "count"), GetFloat(cl, "fps"), GetBool(cl, "loop"));
                    }
                }
            }

            // KeyframeAnimator
            if (entityValue.HasMember(Keys::KEYFRAME_ANIMATOR_COMPONENT)) {
                const auto& v = entityValue[Keys::KEYFRAME_ANIMATOR_COMPONENT];
                auto& c = gECS.AddComponent<KeyframeAnimatorComponent>(newEntity);
                c.current = GetString(v, "current");
                c.t = GetFloat(v, "t");
                c.playing = GetBool(v, "playing");
                if (v.HasMember("clips") && v["clips"].IsArray()) {
                    for (const auto& clipVal : v["clips"].GetArray()) {
                        AnimationClip nc;
                        nc.name = GetString(clipVal, "name");
                        nc.wrap = static_cast<KFWrap>(GetInt(clipVal, "wrap"));
                        nc.textureAsset = GetString(clipVal, "textureAsset");
                        nc.spriteCols = GetInt(clipVal, "spriteCols");
                        nc.spriteRows = GetInt(clipVal, "spriteRows");
                        nc.useCustomSize = GetBool(clipVal, "useCustomSize");
                        nc.targetWidth = GetFloat(clipVal, "targetWidth");
                        nc.targetHeight = GetFloat(clipVal, "targetHeight");
                        if (clipVal.HasMember("bindings") && clipVal["bindings"].IsArray()) {
                            for (const auto& bVal : clipVal["bindings"].GetArray()) {
                                KFBinding nb;
                                nb.property = static_cast<KFProperty>(GetInt(bVal, "property"));
                                if (bVal.HasMember("keys") && bVal["keys"].IsArray()) {
                                    for (const auto& kVal : bVal["keys"].GetArray()) {
                                        KFKey k;
                                        k.time = GetFloat(kVal, "t");
                                        k.value = GetFloat(kVal, "v");
                                        k.interp = static_cast<KFInterp>(GetInt(kVal, "i"));
                                        k.inTan = GetFloat(kVal, "in");
                                        k.outTan = GetFloat(kVal, "out");
                                        nb.curve.Add(k);
                                    }
                                }
                                nc.bindings.push_back(nb);
                            }
                        }
                        c.clips[nc.name] = nc;
                    }
                }
            }

            // Parent
            if (entityValue.HasMember(Keys::PARENT_COMPONENT)) {
                const auto& v = entityValue[Keys::PARENT_COMPONENT];
                auto& p = gECS.AddComponent<Component::ParentComponent>(newEntity);
                if (outParentLinks) {
                    const char* pName = GetString(v, "parentName");
                    if (*pName) outParentLinks->push_back({ static_cast<uint32_t>(newEntity), pName });
                }
                if (v.HasMember("localPos")) p.localPos = ReadVec3(v["localPos"]);
                p.localRot = GetFloat(v, "localRot");
                if (v.HasMember("localScale")) p.localScale = ReadVec2(v["localScale"]);
                p.inheritColor = GetBool(v, "inheritColor");
                if (v.HasMember("colorMul")) p.colorMul = ReadColor(v["colorMul"]);
            }

            return newEntity;
        }
        
        /*!
        @brief Loads entities and their components from a specified scene asset into the ECS.
        @param sceneName The name of the scene asset to load (e.g., "level1.json").
        */
        void LoadScenes(std::string const& sceneName) {
            currSceneName = sceneName;
            Assets::AssetManager& assetManager = Assets::AssetManager::GetInstance();

            Assets::SceneAsset* sceneAsset = assetManager.Get<Assets::SceneAsset>(sceneName);

            if (!sceneAsset) {
                std::cout << "ERROR: Scene Asset '" << sceneName << "' not found." << std::endl;
                return;
            }

            // Cycle through sceneAsset sceneData (JSON document) for testing
            std::string fullPath = Assets::rootPath + "/scenes/" + sceneName;

            char logBuffer[256];
            // Format the string into the buffer
            std::snprintf(logBuffer, sizeof(logBuffer),
                "Scene changed to %s", sceneName.c_str());

            // Pass the formatted string to the macro
            LOGI(Core, logBuffer);

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

            if (doc.HasMember("quadtree") && doc["quadtree"].IsObject()) {
                const rapidjson::Value& quadtree = doc["quadtree"];

                Collision::AABBRect bounds{ CEVec3{-2000.f, -2000.f, 0.f}, CEVec3{2000.f, 2000.f, 0.f} };
                Collision::CollisionHandler& colHandler{ Collision::CollisionHandler::GetInstance() };
        

                if (quadtree.HasMember("min") && quadtree["min"].IsObject()) {
                    CEVec3 newMin{};
                    const rapidjson::Value& min = quadtree["min"];

                    if (min.HasMember("x") && min["x"].IsFloat()) {
                        newMin.x = min["x"].GetFloat();
                    }

                    if (min.HasMember("y") && min["y"].IsFloat()) {
                        newMin.y = min["y"].GetFloat();
                    }

                    bounds.min = newMin;

                }

                if (quadtree.HasMember("max") && quadtree["max"].IsObject()) {
                    CEVec3 newMax{};
                    const rapidjson::Value& max = quadtree["max"];

                    if (max.HasMember("x") && max["x"].IsFloat()) {
                        newMax.x = max["x"].GetFloat();
                    }

                    if (max.HasMember("y") && max["y"].IsFloat()) {
                        newMax.y = max["y"].GetFloat();
                    }

                    bounds.max = newMax;
                }

                colHandler.quadTree.Resize(bounds);
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
            std::vector<ParentLinkInfo> parentLinksToProcess;


            for (rapidjson::SizeType i = 0; i < entitiesArray.Size(); ++i) {
                DeserializeEntity(entitiesArray[i], &parentLinksToProcess);

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
        @param sceneName The name of the scene asset to save to
        */
        void SaveScenes(std::string const& sceneName) {
            LOGW(Core, "SaveScenes called with name='%s'", sceneName.c_str());
            LOGW(Core, "Assets::rootPath = '%s'", Assets::rootPath.c_str());

            // Always build a filesystem path once
            std::filesystem::path scenePath = std::filesystem::path(Assets::rootPath) / "scenes" / sceneName;

            // Make sure folder exists
            std::filesystem::create_directories(scenePath.parent_path());

            // Enforce .json extension (on the PATH, not the string)
            if (scenePath.extension().empty()) {
                scenePath.replace_extension(".json");
            }

            // Use string only when needed (logging / file IO helpers)
            std::string fullPath = scenePath.string();

            LOGW(Core, "Saving scene file to fullPath = '%s'", fullPath.c_str());

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
                // A.5 PrefabComponent  (ADD THIS BLOCK)
                // =========================================================
                if (gECS.HasComponent<Component::PrefabInstanceComponent>(id)) {
                    const auto& pc = *gECS.GetComponent<Component::PrefabInstanceComponent>(id);

                    rapidjson::Value prefabVal(rapidjson::kObjectType);

                    // IMPORTANT: change these field names to match your PrefabComponent fields
                    // Common field names are: prefabName / source / prefabAsset / prefabPath
                    prefabVal.AddMember("name", rapidjson::Value(pc.prefabName.c_str(), allocator), allocator);

                    // Optional extras if you have them:
                    // prefabVal.AddMember("isRoot", pc.isPrefabRoot, allocator);
                    // prefabVal.AddMember("allowOverrides", pc.allowOverrides, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::PREFAB_COMPONENT), prefabVal, allocator);
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
                    rigidBodyValue.AddMember("restitutionCoeff", rigidBodyComp.GetCoeff(), allocator);

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

                    //bool isTrigger{ rigidBodyComp.getFlags().test(Physics::SIMULATE_DYNAMICS) };
                    //rigidBodyValue.AddMember("isTrigger", isTrigger, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::RIGIDBODY_COMPONENT), std::move(rigidBodyValue) , allocator);
                }

                // =========================================================
                // D. ColliderComponent
                // =========================================================
                if (gECS.HasComponent<ColliderComponent>(id)) {
                    const ColliderComponent& colComp = *gECS.GetComponent<ColliderComponent>(id);
                    rapidjson::Value colliderValue(rapidjson::kObjectType);

                    // 1. Helper Type (The Source of Truth)
                    colliderValue.AddMember("ColliderType", (int)colComp.collider->ColliderType, allocator);

                    // Save isTrigger
                    colliderValue.AddMember("isTrigger", colComp.collider->isTrigger, allocator);

                    // Save enabled
                    colliderValue.AddMember("enabled", colComp.collider->enabled, allocator);

                    // Common fields
                    rapidjson::Value posVal(rapidjson::kObjectType);
                    posVal.AddMember("x", colComp.collider->position.x, allocator);
                    posVal.AddMember("y", colComp.collider->position.y, allocator);
                    posVal.AddMember("z", colComp.collider->position.z, allocator);
                    colliderValue.AddMember("position", posVal, allocator);

                    rapidjson::Value offVal(rapidjson::kObjectType);
                    offVal.AddMember("x", colComp.collider->offset.x, allocator);
                    offVal.AddMember("y", colComp.collider->offset.y, allocator);
                    offVal.AddMember("z", colComp.collider->offset.z, allocator);
                    colliderValue.AddMember("offset", offVal, allocator);

                    switch (colComp.collider->ColliderType) {
                    case Collision::ColliderType::CIRCLE: {
                        auto* s = static_cast<Collision::Circle*>(colComp.collider.get());
                        colliderValue.AddMember("radius", s->radius, allocator);

                        // Save Center specifically if needed, though position covers it
                        rapidjson::Value cVal(rapidjson::kObjectType);
                        cVal.AddMember("x", s->position.x, allocator);
                        cVal.AddMember("y", s->position.y, allocator);
                        cVal.AddMember("z", s->position.z, allocator);
                        colliderValue.AddMember("center", cVal, allocator);
                        break;
                    }
                    case Collision::ColliderType::AABB: {
                        auto* a = static_cast<Collision::AABBRect*>(colComp.collider.get());

                        rapidjson::Value min(rapidjson::kObjectType);
                        min.AddMember("x", a->min.x, allocator);
                        min.AddMember("y", a->min.y, allocator);
                        min.AddMember("z", a->min.z, allocator);
                        colliderValue.AddMember("min", min, allocator);

                        rapidjson::Value max(rapidjson::kObjectType);
                        max.AddMember("x", a->max.x, allocator);
                        max.AddMember("y", a->max.y, allocator);
                        max.AddMember("z", a->max.z, allocator);
                        colliderValue.AddMember("max", max, allocator);
                        break;
                    }
                    case Collision::ColliderType::OBB: {
                        // Cast to Polygon (since OBB is now a Polygon backend)
                        auto* p = static_cast<Collision::Polygon*>(colComp.collider.get());

                        // Save Min/Max/Rotation so CreateOBB can reconstruct it
                        CEVec3 extents = Collision::Polygon::CalculateOBBExtents(*p);

                        // Reconstruct Min/Max for saving
                        CEVec3 center = p->position;
                        CEVec3 min = center - extents;
                        CEVec3 max = center + extents;

                        rapidjson::Value minV(rapidjson::kObjectType);
                        minV.AddMember("x", min.x, allocator); minV.AddMember("y", min.y, allocator); minV.AddMember("z", min.z, allocator);
                        colliderValue.AddMember("min", minV, allocator);

                        rapidjson::Value maxV(rapidjson::kObjectType);
                        maxV.AddMember("x", max.x, allocator); maxV.AddMember("y", max.y, allocator); maxV.AddMember("z", max.z, allocator);
                        colliderValue.AddMember("max", maxV, allocator);

                        colliderValue.AddMember("rotation", p->rotation, allocator);
                        break;
                    }
                    case Collision::ColliderType::LINE: {
                        auto* l = static_cast<Collision::LineSegment*>(colComp.collider.get());

                        // Save the local start and end points using your WriteVec3 helper
                        JsonHelpers::WriteVec3(colliderValue, "p1", l->p1, allocator);
                        JsonHelpers::WriteVec3(colliderValue, "p2", l->p2, allocator);
                        break;
                    }
                    case Collision::ColliderType::POLYGON:
                    case Collision::ColliderType::TRIANGLE: {
                        auto* p = static_cast<Collision::Polygon*>(colComp.collider.get());
                        colliderValue.AddMember("rotation", p->rotation, allocator);

                        // Save Vertices Array
                        rapidjson::Value verts(rapidjson::kArrayType);
                        for (int k = 0; k < p->vertexCount; ++k) {
                            rapidjson::Value v(rapidjson::kObjectType);
                            v.AddMember("x", p->vertices[k].x, allocator);
                            v.AddMember("y", p->vertices[k].y, allocator);
                            v.AddMember("z", p->vertices[k].z, allocator);
                            verts.PushBack(v, allocator);
                        }
                        colliderValue.AddMember("vertices", verts, allocator);
                        break;
                    }
                    default: break;
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

                    // CEVec2 uvOffset
                    rapidjson::Value uvOffsetValue(rapidjson::kObjectType);
                    uvOffsetValue.AddMember("x", sComp.uvOffset.x, allocator);
                    uvOffsetValue.AddMember("y", sComp.uvOffset.y, allocator);
                    shapeValues.AddMember("uvOffset", uvOffsetValue, allocator);

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
                    const AudioEmitterComponent& audioComp = *gECS.GetComponent<AudioEmitterComponent>(id);
                    rapidjson::Value audioValue(rapidjson::kObjectType);

                    // 1. Event Path
                    rapidjson::Value pathVal;
                    pathVal.SetString(audioComp.eventPath.c_str(), allocator);
                    audioValue.AddMember("eventPath", pathVal, allocator);

                    // 2. Properties
                    audioValue.AddMember("volume", audioComp.volume, allocator);
                    audioValue.AddMember("pitch", audioComp.pitch, allocator);
                    audioValue.AddMember("spatial3D", audioComp.spatial3D, allocator);
                    audioValue.AddMember("looping", audioComp.looping, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::AUDIO_COMPONENT), audioValue, allocator);
                }

                // =========================================================
                // H. ScriptComponent
                // =========================================================
                if (gECS.HasComponent<ScriptComponent>(id)) {
                    const ScriptComponent& scriptComp = *gECS.GetComponent<ScriptComponent>(id);
                    rapidjson::Value scriptValue(rapidjson::kObjectType);
                    rapidjson::Value scriptsArray(rapidjson::kArrayType);

                    for (const auto& inst : scriptComp.Scripts) {
                        if (inst.ClassName.empty()) continue;

                        // 1. Create Script Object
                        rapidjson::Value scriptObj(rapidjson::kObjectType);

                        // Save ClassName
                        rapidjson::Value nameVal;
                        nameVal.SetString(inst.ClassName.c_str(), allocator);
                        scriptObj.AddMember("className", nameVal, allocator);

                        // 2. Serialize Fields
                        if (inst.Instance && inst.Class) {
                            rapidjson::Value fieldsObj(rapidjson::kObjectType);

                            void* iter = nullptr;
                            while (MonoClassField* field = mono_class_get_fields(inst.Class, &iter)) {
                                const char* fieldName = mono_field_get_name(field);
                                uint32_t flags = mono_field_get_flags(field);

                                if (flags & MONO_FIELD_ATTR_STATIC) continue;

                                // Check Visibility (Public or Attribute)
                                bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
                                bool hasAttribute = false;

                                // Look up the attribute class (Cache this static pointer for speed!)
                                static MonoClass* attrClass = mono_class_from_name(
                                    MonoManager::GetImage(), "CrashoutsEngine", "SerializeField");

                                if (attrClass) {
                                    // Get all attributes on this specific field
                                    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(inst.Class, field);
                                    if (attrInfo) {
                                        // Check if our specific attribute is present
                                        hasAttribute = mono_custom_attrs_has_attr(attrInfo, attrClass);
                                        mono_custom_attrs_free(attrInfo); // Important: Clean up!
                                    }
                                }

                                if (isPublic || hasAttribute) {
                                    MonoType* type = mono_field_get_type(field);
                                    int typeEnum = mono_type_get_type(type);
                                    rapidjson::Value key(fieldName, allocator);

                                    if (typeEnum == MONO_TYPE_I4) {
                                        int val;
                                        mono_field_get_value(inst.Instance, field, &val);
                                        fieldsObj.AddMember(key, val, allocator);
                                    }
                                    else if (typeEnum == MONO_TYPE_R4) {
                                        float val;
                                        mono_field_get_value(inst.Instance, field, &val);
                                        fieldsObj.AddMember(key, val, allocator);
                                    }
                                    else if (typeEnum == MONO_TYPE_BOOLEAN) {
                                        bool val;
                                        mono_field_get_value(inst.Instance, field, &val);
                                        fieldsObj.AddMember(key, val, allocator);
                                    }
                                    else if (typeEnum == MONO_TYPE_STRING) {
                                        MonoString* monoStr = nullptr;
                                        mono_field_get_value(inst.Instance, field, &monoStr);
                                        if (monoStr) {
                                            char* str = mono_string_to_utf8(monoStr);
                                            rapidjson::Value sVal(str, allocator);
                                            fieldsObj.AddMember(key, sVal, allocator);
                                            mono_free(str);
                                        }
                                    }
                                }
                            }
                            scriptObj.AddMember("fields", fieldsObj, allocator);
                        }
                        scriptsArray.PushBack(scriptObj, allocator);
                    }

                    scriptValue.AddMember("scripts", scriptsArray, allocator);
                    entityValue.AddMember(rapidjson::StringRef(Keys::SCRIPT_COMPONENT), scriptValue, allocator);
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
                // P. SpriteAnimatorComponent
                // =========================================================
                if (gECS.HasComponent<SpriteAnimatorComponent>(id)) {
                    const auto& comp = *gECS.GetComponent<SpriteAnimatorComponent>(id);
                    rapidjson::Value compVal(rapidjson::kObjectType);

                    // Save Playback State
                    rapidjson::Value currName;
                    currName.SetString(comp.currentClipName.c_str(), allocator);
                    compVal.AddMember("currentClip", currName, allocator);
                    compVal.AddMember("flipH", comp.flipH, allocator);
                    compVal.AddMember("playing", comp.playing, allocator);

                    // Save Clips Array
                    rapidjson::Value clipsArr(rapidjson::kArrayType);
                    for (const auto& clip : comp.clips) {
                        rapidjson::Value clipObj(rapidjson::kObjectType);

                        rapidjson::Value nameVal;
                        nameVal.SetString(clip.name.c_str(), allocator);
                        clipObj.AddMember("name", nameVal, allocator);

                        clipObj.AddMember("start", clip.startFrame, allocator);
                        clipObj.AddMember("count", clip.frameCount, allocator);
                        clipObj.AddMember("fps", clip.fps, allocator);
                        clipObj.AddMember("loop", clip.loop, allocator);

                        clipsArr.PushBack(clipObj, allocator);
                    }
                    compVal.AddMember("clips", clipsArr, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::SPRITE_ANIMATOR_COMPONENT), compVal, allocator);
                }
                // =========================================================
// PrefabInstanceComponent
// =========================================================
                if (gECS.HasComponent<Component::PrefabInstanceComponent>(id)) {
                    const auto& p = *gECS.GetComponent<Component::PrefabInstanceComponent>(id);
                    rapidjson::Value val(rapidjson::kObjectType);
                    val.AddMember("name", rapidjson::Value(p.prefabName.c_str(), allocator), allocator);
                    entityValue.AddMember(rapidjson::StringRef(Keys::PREFAB_COMPONENT), val, allocator);
                }

                // =========================================================
                // Q. ParentComponent
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

                // =========================================================
                // R. KeyframeAnimatorComponent SERIALIZATION
                // =========================================================
                if (gECS.HasComponent<KeyframeAnimatorComponent>(id)) {
                    const KeyframeAnimatorComponent& ka = *gECS.GetComponent<KeyframeAnimatorComponent>(id);
                    rapidjson::Value kaVal(rapidjson::kObjectType);

                    // 1. Basic State
                    rapidjson::Value currentVal;
                    currentVal.SetString(ka.current.c_str(), allocator);
                    kaVal.AddMember("current", currentVal, allocator);
                    kaVal.AddMember("t", ka.t, allocator);
                    kaVal.AddMember("playing", ka.playing, allocator);

                    // 2. Save Clips
                    rapidjson::Value clipsArr(rapidjson::kArrayType);

                    for (const auto& pair : ka.clips) {
                        const AnimationClip& clip = pair.second;
                        rapidjson::Value clipVal(rapidjson::kObjectType);

                        rapidjson::Value cName;
                        cName.SetString(clip.name.c_str(), allocator);
                        clipVal.AddMember("name", cName, allocator);
                        clipVal.AddMember("wrap", static_cast<int>(clip.wrap), allocator);

                        // --- [NEW] Save Texture & Size Data ---
                        if (!clip.textureAsset.empty()) {
                            rapidjson::Value texName;
                            texName.SetString(clip.textureAsset.c_str(), allocator);
                            clipVal.AddMember("textureAsset", texName, allocator);
                        }
                        clipVal.AddMember("spriteCols", clip.spriteCols, allocator);
                        clipVal.AddMember("spriteRows", clip.spriteRows, allocator);
                        clipVal.AddMember("useCustomSize", clip.useCustomSize, allocator);
                        clipVal.AddMember("targetWidth", clip.targetWidth, allocator);
                        clipVal.AddMember("targetHeight", clip.targetHeight, allocator);
                        // --------------------------------------

                        // 3. Save Bindings
                        rapidjson::Value bindArr(rapidjson::kArrayType);
                        for (const auto& binding : clip.bindings) {
                            rapidjson::Value bVal(rapidjson::kObjectType);
                            bVal.AddMember("property", static_cast<int>(binding.property), allocator);

                            // 4. Save Keys
                            rapidjson::Value keyArr(rapidjson::kArrayType);
                            for (const auto& k : binding.curve.keys) {
                                rapidjson::Value kVal(rapidjson::kObjectType);
                                kVal.AddMember("t", k.time, allocator);
                                kVal.AddMember("v", k.value, allocator);
                                kVal.AddMember("i", static_cast<int>(k.interp), allocator);
                                kVal.AddMember("in", k.inTan, allocator);
                                kVal.AddMember("out", k.outTan, allocator);
                                keyArr.PushBack(kVal, allocator);
                            }
                            bVal.AddMember("keys", keyArr, allocator);
                            bindArr.PushBack(bVal, allocator);
                        }
                        clipVal.AddMember("bindings", bindArr, allocator);
                        clipsArr.PushBack(clipVal, allocator);
                    }

                    kaVal.AddMember("clips", clipsArr, allocator);
                    entityValue.AddMember(rapidjson::StringRef(Keys::KEYFRAME_ANIMATOR_COMPONENT), kaVal, allocator);
                }
            
                // =========================================================
                // S. ParticleComponent SERIALIZATION
                // =========================================================
                if (gECS.HasComponent<ParticleComponent>(id)) {
                    const auto& pc = *gECS.GetComponent<ParticleComponent>(id);
                    rapidjson::Value pVal(rapidjson::kObjectType);

                    // 1. General Settings
                    pVal.AddMember("isEmitting", pc.isEmitting, allocator);
                    pVal.AddMember("useWorldSpace", pc.useWorldSpace, allocator);
                    pVal.AddMember("maxParticles", pc.maxParticles, allocator);
                    pVal.AddMember("emitRate", pc.emitRate, allocator);

                    // 2. Emission Shape
                    pVal.AddMember("emissionShape", static_cast<int>(pc.emissionShape), allocator);
                    pVal.AddMember("radiusMin", pc.radiusMin, allocator);
                    pVal.AddMember("radiusMax", pc.radiusMax, allocator);

                    // 3. Lifecycle & Speed
                    pVal.AddMember("startLifeMin", pc.startLifeMin, allocator);
                    pVal.AddMember("startLifeMax", pc.startLifeMax, allocator);
                    pVal.AddMember("startSpeedMin", pc.startSpeedMin, allocator);
                    pVal.AddMember("startSpeedMax", pc.startSpeedMax, allocator);

                    // 4. Size & Rotation
                    pVal.AddMember("uniformSize", pc.uniformSize, allocator);
                    pVal.AddMember("startSizeXMin", pc.startSizeXMin, allocator);
                    pVal.AddMember("startSizeXMax", pc.startSizeXMax, allocator);
                    pVal.AddMember("startSizeYMin", pc.startSizeYMin, allocator);
                    pVal.AddMember("startSizeYMax", pc.startSizeYMax, allocator);

                    pVal.AddMember("sizeOverTime", pc.sizeOverTime, allocator);
                    pVal.AddMember("rotationOverTime", pc.rotationOverTime, allocator);

                    // 5. Colors & Physics
                    rapidjson::Value sCol(rapidjson::kObjectType);
                    sCol.AddMember("x", pc.startcolor.x(), allocator);
                    sCol.AddMember("y", pc.startcolor.y(), allocator);
                    sCol.AddMember("z", pc.startcolor.z(), allocator);
                    sCol.AddMember("w", pc.startcolor.w(), allocator);
                    pVal.AddMember("startColor", sCol, allocator);

                    rapidjson::Value eCol(rapidjson::kObjectType);
                    eCol.AddMember("x", pc.endColor.x(), allocator);
                    eCol.AddMember("y", pc.endColor.y(), allocator);
                    eCol.AddMember("z", pc.endColor.z(), allocator);
                    eCol.AddMember("w", pc.endColor.w(), allocator);
                    pVal.AddMember("endColor", eCol, allocator);

                    rapidjson::Value grav(rapidjson::kObjectType);
                    grav.AddMember("x", pc.gravity.x, allocator);
                    grav.AddMember("y", pc.gravity.y, allocator);
                    grav.AddMember("z", pc.gravity.z, allocator);
                    pVal.AddMember("gravity", grav, allocator);

                    rapidjson::Value sVel(rapidjson::kObjectType);
                    sVel.AddMember("x", pc.startVelocity.x, allocator);
                    sVel.AddMember("y", pc.startVelocity.y, allocator);
                    sVel.AddMember("z", pc.startVelocity.z, allocator);
                    pVal.AddMember("startVelocity", sVel, allocator);

                    // 6. Texture
                    pVal.AddMember("useTexture", pc.useTexture, allocator);
                    rapidjson::Value texName;
                    texName.SetString(pc.textureName.c_str(), allocator);
                    pVal.AddMember("textureName", texName, allocator);

                    rapidjson::Value uv(rapidjson::kObjectType);
                    uv.AddMember("x", pc.textureUV.x, allocator);
                    uv.AddMember("y", pc.textureUV.y, allocator);
                    pVal.AddMember("textureUV", uv, allocator);

                    entityValue.AddMember(rapidjson::StringRef(Keys::PARTICLE_COMPONENT), pVal, allocator);
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

            // QuadTree Save
            rapidjson::Value quadTree(rapidjson::kObjectType);

            Collision::CollisionHandler& colHandler{Collision::CollisionHandler::GetInstance()};
            rapidjson::Value min(rapidjson::kObjectType);
            min.AddMember("x", colHandler.quadTree.boundary.min.x, allocator);
            min.AddMember("y", colHandler.quadTree.boundary.min.y, allocator);

            rapidjson::Value max(rapidjson::kObjectType);
            max.AddMember("x", colHandler.quadTree.boundary.max.x, allocator);
            max.AddMember("y", colHandler.quadTree.boundary.max.y, allocator);

            quadTree.AddMember("min", min, allocator);
            quadTree.AddMember("max", max, allocator);

            doc.AddMember("quadtree", quadTree, allocator);
            
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
         
            LOGW(Core, "About to write %zu bytes to '%s'", content.size(), fullPath.c_str());

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
