/*!
@file        prefab_init.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Prefab load/save (RapidJSON) matching your scene:
             Transform {position, rotation, size},
             Shape {type, width, height, size, color, enableTexture, textureName},
             RigidBody {mass, restitution},
             and Collider (AABB by default).
*/

#include <memory>
#include <string>
#include <fstream>
#include "prefab_init.h"
#include "prefab.h"
#include "entitymanager.h"
#include "collisionmanager.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "textcomponent.h"
#include "textrenderer.h"  
using namespace CrashoutsEngine;
using namespace Component;

static std::unique_ptr<Collision::AABBRect>
MakeCenteredAABB3(const CEVec3& center, float width, float height)
{
    const float halfwidth = 0.5f * width, halfheight = 0.5f * height;
    CEVec3 minCorner{ center.x - halfwidth, center.y - halfheight, center.z };
    CEVec3 maxCorner{ center.x + halfwidth, center.y + halfheight, center.z };
    return std::make_unique<Collision::AABBRect>(minCorner, maxCorner);
}

static bool LoadPrefabsFromJson_RJ(const std::string& path)
{
    std::ifstream inputFile(path);
    if (!inputFile) {
        LOGE(Core, std::string("Cannot open ") + path);
        return false;
    }

    rapidjson::IStreamWrapper isw(inputFile);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError() || !doc.IsObject()) {
        LOGE(Core, std::string("JSON parse error in ") + path);
        return false;
    }
    if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
        LOGE(Core, std::string("Missing 'prefabs' array in ") + path);
        return false;
    }

    const auto& arr = doc["prefabs"].GetArray();
    for (auto& prefabs : arr) {
        if (!prefabs.IsObject()) continue;
        if (!prefabs.HasMember("type") || !prefabs["type"].IsString()) continue;
        if (!prefabs.HasMember("name") || !prefabs["name"].IsString()) continue;

        const std::string type = prefabs["type"].GetString();
        const std::string name = prefabs["name"].GetString();

        // ----------------------
        // BOX PREFAB
        // ----------------------
        if (type == "Box") {
            // ---- shape ----
            if (!prefabs.HasMember("shape") || !prefabs["shape"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing shape"); continue; }
            const auto& shapeJson = prefabs["shape"];
            float width = (shapeJson.HasMember("width") && shapeJson["width"].IsNumber()) ? (float)shapeJson["width"].GetDouble() : 1.0f;
            float height = (shapeJson.HasMember("height") && shapeJson["height"].IsNumber()) ? (float)shapeJson["height"].GetDouble() : 1.0f;

            int   typeInt = (shapeJson.HasMember("type") && shapeJson["type"].IsInt()) ? shapeJson["type"].GetInt() : (int)ShapeType::Rectangle;

            CEVec3 color{ 0,0,0 };
            if (shapeJson.HasMember("color") && shapeJson["color"].IsObject()) {
                const auto& colorJson = shapeJson["color"];
                color.x = (colorJson.HasMember("r") && colorJson["r"].IsNumber()) ? (float)colorJson["r"].GetDouble() : 0.f;
                color.y = (colorJson.HasMember("g") && colorJson["g"].IsNumber()) ? (float)colorJson["g"].GetDouble() : 0.f;
                color.z = (colorJson.HasMember("b") && colorJson["b"].IsNumber()) ? (float)colorJson["b"].GetDouble() : 0.f;
            }
         
          

            const bool enableTexture = (shapeJson.HasMember("enableTexture") && shapeJson["enableTexture"].IsBool()) ? shapeJson["enableTexture"].GetBool() : false;
            std::string textureName;
            if (enableTexture && shapeJson.HasMember("textureName") && shapeJson["textureName"].IsString())
                textureName = shapeJson["textureName"].GetString();
            if (!prefabs.HasMember("transform") || !prefabs["transform"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing transform"); continue; }
            const auto& transformJson = prefabs["transform"];
            if (!transformJson.HasMember("position") || !transformJson["position"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing position"); continue; }
            const auto& posJ = transformJson["position"];
            float posX = (posJ.HasMember("x") && posJ["x"].IsNumber()) ? (float)posJ["x"].GetDouble() : 0.f;
            float posY = (posJ.HasMember("y") && posJ["y"].IsNumber()) ? (float)posJ["y"].GetDouble() : 0.f;
            float posZ = (posJ.HasMember("z") && posJ["z"].IsNumber()) ? (float)posJ["z"].GetDouble() : 0.f;

            float rotation = (transformJson.HasMember("rotation") && transformJson["rotation"].IsNumber()) ? (float)transformJson["rotation"].GetDouble() : 0.f;
            CEVec2 size{ 1.f,1.f };
            if (transformJson.HasMember("size") && transformJson["size"].IsObject()) {
                const auto& sizeJ = transformJson["size"];
                size.x = (sizeJ.HasMember("x") && sizeJ["x"].IsNumber()) ? (float)sizeJ["x"].GetDouble() : 1.f;
                size.y = (sizeJ.HasMember("y") && sizeJ["y"].IsNumber()) ? (float)sizeJ["y"].GetDouble() : 1.f;
            }

            // ---- rigidbody ----
            if (!prefabs.HasMember("rigidbody") || !prefabs["rigidbody"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing rigidbody"); continue; }
            const auto& rigidbodyJson = prefabs["rigidbody"];
            float mass = (rigidbodyJson.HasMember("mass") && rigidbodyJson["mass"].IsNumber()) ? (float)rigidbodyJson["mass"].GetDouble() : 0.f;
            float restitution = (rigidbodyJson.HasMember("restitution") && rigidbodyJson["restitution"].IsNumber()) ? (float)rigidbodyJson["restitution"].GetDouble() : 0.f;

        
            bool hasColliderFlags = false; bool isTrigger = false; unsigned category = 0; unsigned mask = 0;
            if (prefabs.HasMember("collider") && prefabs["collider"].IsObject()) {
                const auto& coliderJson = prefabs["collider"];
                hasColliderFlags = true;
                if (coliderJson.HasMember("isTrigger") && coliderJson["isTrigger"].IsBool()) isTrigger = coliderJson["isTrigger"].GetBool();
                if (coliderJson.HasMember("category") && coliderJson["category"].IsUint()) category = coliderJson["category"].GetUint();
                if (coliderJson.HasMember("mask") && coliderJson["mask"].IsUint())     mask = coliderJson["mask"].GetUint();
            }
            // ---------- TextComponent values parsed from JSON (outside the lambda) ----------
            bool hasTextComponent = false;

            std::string jsonFont = "Exo2-Regular";
            std::string jsonText = "TEST";
            float jsonScale = 1.0f;
            CEVec2 jsonOffset{ 0.0f, 0.0f };
            CEVec3 jsonColor{ 255.0f, 255.0f, 255.0f };
            bool jsonCentered = true;
            bool jsonIsWorldSpace = true;

            if (prefabs.HasMember("text") && prefabs["text"].IsObject()) {
                const auto& textJson = prefabs["text"];
                hasTextComponent = true;

                if (textJson.HasMember("font") && textJson["font"].IsString())
                    jsonFont = textJson["font"].GetString();

                if (textJson.HasMember("text") && textJson["text"].IsString())
                    jsonText = textJson["text"].GetString();

                if (textJson.HasMember("scale") && textJson["scale"].IsNumber())
                    jsonScale = static_cast<float>(textJson["scale"].GetDouble());

                if (textJson.HasMember("offset") && textJson["offset"].IsObject()) {
                    const auto& off = textJson["offset"];
                    if (off.HasMember("x") && off["x"].IsNumber())
                        jsonOffset.x = static_cast<float>(off["x"].GetDouble());
                    if (off.HasMember("y") && off["y"].IsNumber())
                        jsonOffset.y = static_cast<float>(off["y"].GetDouble());
                }

                if (textJson.HasMember("color") && textJson["color"].IsObject()) {
                    const auto& col = textJson["color"];
                    if (col.HasMember("r") && col["r"].IsNumber())
                        jsonColor.x = static_cast<float>(col["r"].GetDouble());
                    if (col.HasMember("g") && col["g"].IsNumber())
                        jsonColor.y = static_cast<float>(col["g"].GetDouble());
                    if (col.HasMember("b") && col["b"].IsNumber())
                        jsonColor.z = static_cast<float>(col["b"].GetDouble());
                }

                if (textJson.HasMember("centered") && textJson["centered"].IsBool())
                    jsonCentered = textJson["centered"].GetBool();

                if (textJson.HasMember("isWorldSpace") && textJson["isWorldSpace"].IsBool())
                    jsonIsWorldSpace = textJson["isWorldSpace"].GetBool();
            }


            Prefab::Register(name, [=](ECS& e) {
                const Entity entitty = e.CreateEntity();
                e.AddComponent<NameComponent>(entitty, NameComponent{ name });

                // Transform
                TransformComponent transform{};
                transform.position = CEVec3{ posX, posY, posZ };
                transform.rotation = rotation;
                transform.size = size;
                e.AddComponent<TransformComponent>(entitty, transform);

                // Shape
                ShapeComponent shapeComp{};
                shapeComp.type = (ShapeType)typeInt;
                shapeComp.color = color;
                shapeComp.enableTexture = enableTexture;
                shapeComp.textureName = textureName;
                e.AddComponent<ShapeComponent>(entitty, shapeComp);
             
                
                // RigidBody 
                e.AddComponent<RigidBodyComponent>(entitty, CEVec3{ 0,0,0 }, mass, restitution);

                // Collider (AABB for robustness)
                {
                    auto aabb = MakeCenteredAABB3(transform.position, width, height);
                    e.AddComponent<ColliderComponent>(entitty, ColliderComponent{ std::move(aabb) });
                
                }
                // TextComponent
                if (hasTextComponent) {
                    TextComponent textComponent{};
                    textComponent.font = jsonFont;
                    textComponent.text = jsonText;
                    textComponent.scale = jsonScale;
                    textComponent.offset = jsonOffset;
                    textComponent.color = jsonColor;
                    textComponent.centered = jsonCentered;
                    textComponent.isWorldSpace = jsonIsWorldSpace;

                    e.AddComponent<TextComponent>(entitty, textComponent);
                }
                });
          
        
            // Keep placeholders that always compile. 
            std::string textContentFromJson;
            float textPointSizeFromJson = 16.0f;          
            CEVec3 textColorFromJson{ 255.0f, 255.0f, 255.0f };
            std::string textFontNameFromJson;
            bool textIsWorldSpaceFromJson = false;

            if (prefabs.HasMember("text") && prefabs["text"].IsObject()) {
                const auto& textJson = prefabs["text"];
                hasTextComponent = true;

                if (textJson.HasMember("content") && textJson["content"].IsString())
                    textContentFromJson = textJson["content"].GetString();

                if (textJson.HasMember("pointSize") && textJson["pointSize"].IsNumber())
                    textPointSizeFromJson = static_cast<float>(textJson["pointSize"].GetDouble());

                if (textJson.HasMember("color") && textJson["color"].IsObject()) {
                    const auto& colorJson = textJson["color"];
                    if (colorJson.HasMember("r") && colorJson["r"].IsNumber())
                        textColorFromJson.x = static_cast<float>(colorJson["r"].GetDouble());
                    if (colorJson.HasMember("g") && colorJson["g"].IsNumber())
                        textColorFromJson.y = static_cast<float>(colorJson["g"].GetDouble());
                    if (colorJson.HasMember("b") && colorJson["b"].IsNumber())
                        textColorFromJson.z = static_cast<float>(colorJson["b"].GetDouble());
                }

                if (textJson.HasMember("fontName") && textJson["fontName"].IsString())
                    textFontNameFromJson = textJson["fontName"].GetString();

                if (textJson.HasMember("isWorldSpace") && textJson["isWorldSpace"].IsBool())
                    textIsWorldSpaceFromJson = textJson["isWorldSpace"].GetBool();
            }


            LOGI(Core, std::string("Registered prefab: ") + name);
        }
        else {
            LOGW(Core, std::string("Unknown prefab type: ") + type);
        }
    }

    return true;
}

namespace CrashoutsEngine {

    bool SaveBoxPrefabToJson_RJ(ECS& ecs,
        Entity e,
        const std::string& newName,
        const std::string& path,
        const std::string& matchName)
    {
        using namespace rapidjson;

        // Validate required components
        auto* entityTransform = ecs.GetComponent<TransformComponent>(e);
        auto* shape = ecs.GetComponent<ShapeComponent>(e);
        auto* rigidbody = ecs.GetComponent<RigidBodyComponent>(e);
        if (!entityTransform || !shape || !rigidbody) {
            LOGE(Core, std::string("Save failed: entity missing Transform/Shape/RigidBody."));
            return false;
        }
       

        // Load / init doc
   
        rapidjson::Document doc;
        {
            std::ifstream in(path, std::ios::binary);
            if (in) {
                rapidjson::IStreamWrapper isw(in);
                doc.ParseStream(isw);
                if (doc.HasParseError() || !doc.IsObject()) {
                    LOGE(Core, (std::string("Save aborted: JSON parse error in ") + path).c_str());
                    return false; 
                }
            }
            else {
                
                doc.SetObject();
            }
        }


        auto& allocate = doc.GetAllocator();
        if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
            Value arr(kArrayType);
            doc.AddMember("prefabs", arr, allocate);
        }
        Value& prefabs = doc["prefabs"];

        // Build object
        Value obj(kObjectType);
        obj.AddMember("type", "Box", allocate);

        // name
        {
            Value nm; nm.SetString(newName.c_str(), (SizeType)newName.size(), allocate);
            obj.AddMember("name", nm, allocate);
        }

        // transform (position + rotation + size)
        {
            Value pos(kObjectType);
            pos.AddMember("x", entityTransform->position.x, allocate);
            pos.AddMember("y", entityTransform->position.y, allocate);
            pos.AddMember("z", entityTransform->position.z, allocate);

            Value transformObj(kObjectType);
            transformObj.AddMember("position", pos, allocate);
            transformObj.AddMember("rotation", entityTransform->rotation, allocate);

            Value sizeJ(kObjectType);
            sizeJ.AddMember("x", entityTransform->size.x, allocate);
            sizeJ.AddMember("y", entityTransform->size.y, allocate);
            transformObj.AddMember("size", sizeJ, allocate);

            obj.AddMember("transform", transformObj, allocate);
        }

        // shape
        {
            rapidjson::Value shapeJson(rapidjson::kObjectType);

            // always write type as the enum integer
            shapeJson.AddMember("type", static_cast<int>(shape->type), allocate);

            // optional: human-readable kind string
            auto kindFrom = [](ShapeType t) -> const char* {
                switch (t) {
                case ShapeType::Rectangle: return "Rectangle";
                case ShapeType::Circle:    return "Circle";
                case ShapeType::Triangle:  return "Triangle";
                case ShapeType::Line:      return "Line";
                default:                   return "Unknown";
                }
                };
            shapeJson.AddMember("kind", rapidjson::StringRef(kindFrom(shape->type)), allocate);

            // rectangle-only extras (keep your existing behavior)
            if (shape->type == ShapeType::Rectangle) {
                shapeJson.AddMember("width", entityTransform->size.x, allocate);
                shapeJson.AddMember("height", entityTransform->size.y, allocate);
                shapeJson.AddMember("size",
                    std::max(entityTransform->size.x, entityTransform->size.y), allocate);
            }
            // color
            Value color(kObjectType);
            color.AddMember("r", shape->color.x, allocate);
            color.AddMember("g", shape->color.y, allocate);
            color.AddMember("b", shape->color.z, allocate);
            shapeJson.AddMember("color", color, allocate);

            // texture flags
            shapeJson.AddMember("enableTexture", shape->enableTexture, allocate);
            if (shape->enableTexture && !shape->textureName.empty()) {
                Value texture; texture.SetString(shape->textureName.c_str(), (SizeType)shape->textureName.size(), allocate);
                shapeJson.AddMember("textureName", texture, allocate);
            }

            obj.AddMember("shape", shapeJson, allocate);
        }

       
       // rigidbody (mass, restitution)
   
        {
            rapidjson::Value rigidJson(rapidjson::kObjectType);
            rigidJson.AddMember("mass", rigidbody->getMass(), allocate);          
            rigidJson.AddMember("restitution", rigidbody->getCoeff(), allocate);  
            obj.AddMember("rigidbody", rigidJson, allocate);
        }
      



        // ---- helpers for name matching (add these) ----
        auto trim = [](std::string inputString) {
            size_t firstNonWhitespaceIndex = inputString.find_first_not_of(" \t\r\n");
            size_t lastNonWhitespaceIndex = inputString.find_last_not_of(" \t\r\n");
            if (firstNonWhitespaceIndex == std::string::npos) return std::string{};
            return inputString.substr(firstNonWhitespaceIndex, lastNonWhitespaceIndex - firstNonWhitespaceIndex + 1);
            };
        auto lower = [](std::string inputString) {
            std::transform(inputString.begin(), inputString.end(), inputString.begin(), ::tolower);
            return inputString;
            };
      // Upsert by key (matchName overrides newName for replacement)
        const std::string keyRaw = matchName.empty() ? newName : matchName;
        const std::string key = lower(trim(keyRaw));

        bool replaced = false;
        for (auto& prefab : prefabs.GetArray()) {
            if (!prefab.IsObject() || !prefab.HasMember("name") || !prefab["name"].IsString()) continue;
            const std::string pname = lower(trim(prefab["name"].GetString()));
            if (pname == key) {
                prefab.CopyFrom(obj, allocate);
                replaced = true;
                break;
            }
        }
        if (!replaced) prefabs.PushBack(obj, allocate);


        // Write back
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> write(stringBuffer);
        write.SetMaxDecimalPlaces(6);
        doc.Accept(write);

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) {
            LOGE(Core, std::string("Cannot open for writing: ") + path);
            return false;
        }
        out.write(stringBuffer.GetString(), static_cast<std::streamsize>(stringBuffer.GetSize()));

        LOGI(Core, std::string(replaced ? "Updated prefab: " : "Saved new prefab: ") + newName);
        return true;
    }
    void ReloadPrefabsFromDisk(const std::string& path)
    {
        // Clear existing factories
        for (const auto& n : Prefab::Names())
            Prefab::Unregister(n);

        // Re-load from JSON
        LoadPrefabsFromJson_RJ(path);

        // (Optional) log what’s available now
        for (const auto& n : Prefab::Names())
            LOGI(Core, std::string("Prefab available: ") + n);
    }
   
 void InitPrefabs()
{
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto entity = e.CreateEntity();
        e.AddComponent<NameComponent>(entity, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(entity))
            e.AddComponent<TransformComponent>(entity, TransformComponent{});
    });

    const char* JSON_PATH = "assets/prefabs/prefab.json"; 
    LoadPrefabsFromJson_RJ(JSON_PATH);
}

} 
