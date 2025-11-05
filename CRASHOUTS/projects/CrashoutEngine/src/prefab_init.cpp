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

#include "prefab_init.h"
#include "prefab.h"
#include "entitymanager.h"
#include "collisionmanager.h"
#include <memory>
#include <string>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace CrashoutsEngine;
using namespace Component;

static std::unique_ptr<Collision::AABBRect>
MakeCenteredAABB3(const CEVec3& center, float w, float h)
{
    const float hw = 0.5f * w, hh = 0.5f * h;
    CEVec3 mn{ center.x - hw, center.y - hh, center.z };
    CEVec3 mx{ center.x + hw, center.y + hh, center.z };
    return std::make_unique<Collision::AABBRect>(mn, mx);
}

static bool LoadPrefabsFromJson_RJ(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) {
        LOGE(Core, std::string("Cannot open ") + path);
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);
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
    for (auto& prefab : arr) {
        if (!prefab.IsObject()) continue;
        if (!prefab.HasMember("type") || !prefab["type"].IsString()) continue;
        if (!prefab.HasMember("name") || !prefab["name"].IsString()) continue;

        const std::string type = prefab["type"].GetString();
        const std::string name = prefab["name"].GetString();

        // ----------------------
        // BOX PREFAB
        // ----------------------
        if (type == "Box") {
            
            if (!prefab.HasMember("shape") || !prefab["shape"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing shape"); continue; }
            const auto& shapeJson = prefab["shape"];
            float width = (shapeJson.HasMember("width") && shapeJson["width"].IsNumber()) ? (float)shapeJson["width"].GetDouble() : 1.0f;
            float height = (shapeJson.HasMember("height") && shapeJson["height"].IsNumber()) ? (float)shapeJson["height"].GetDouble() : 1.0f;
            float shapeSize = (shapeJson.HasMember("size") && shapeJson["size"].IsNumber()) ? (float)shapeJson["size"].GetDouble() : 1.0f;
            int   typeInt = (shapeJson.HasMember("type") && shapeJson["type"].IsInt()) ? shapeJson["type"].GetInt() : (int)ShapeType::Rectangle;

            CEVec3 color{ 0,0,0 };
            if (shapeJson.HasMember("color") && shapeJson["color"].IsObject()) {
                const auto& cj = shapeJson["color"];
                color.x = (cj.HasMember("r") && cj["r"].IsNumber()) ? (float)cj["r"].GetDouble() : 0.f;
                color.y = (cj.HasMember("g") && cj["g"].IsNumber()) ? (float)cj["g"].GetDouble() : 0.f;
                color.z = (cj.HasMember("b") && cj["b"].IsNumber()) ? (float)cj["b"].GetDouble() : 0.f;
            }
         
          

            const bool enableTexture = (shapeJson.HasMember("enableTexture") && shapeJson["enableTexture"].IsBool()) ? shapeJson["enableTexture"].GetBool() : false;
            std::string textureName;
            if (enableTexture && shapeJson.HasMember("textureName") && shapeJson["textureName"].IsString())
                textureName = shapeJson["textureName"].GetString();
            if (!prefab.HasMember("transform") || !prefab["transform"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing transform"); continue; }
            const auto& tj = prefab["transform"];
            if (!tj.HasMember("position") || !tj["position"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing position"); continue; }
            const auto& pos = tj["position"];
            float px = (pos.HasMember("x") && pos["x"].IsNumber()) ? (float)pos["x"].GetDouble() : 0.f;
            float py = (pos.HasMember("y") && pos["y"].IsNumber()) ? (float)pos["y"].GetDouble() : 0.f;
            float pz = (pos.HasMember("z") && pos["z"].IsNumber()) ? (float)pos["z"].GetDouble() : 0.f;

            float rotation = (tj.HasMember("rotation") && tj["rotation"].IsNumber()) ? (float)tj["rotation"].GetDouble() : 0.f;
            CEVec2 size{ 1.f,1.f };
            if (tj.HasMember("size") && tj["size"].IsObject()) {
                const auto& sj = tj["size"];
                size.x = (sj.HasMember("x") && sj["x"].IsNumber()) ? (float)sj["x"].GetDouble() : 1.f;
                size.y = (sj.HasMember("y") && sj["y"].IsNumber()) ? (float)sj["y"].GetDouble() : 1.f;
            }

            // ---- rigidbody ----
            if (!prefab.HasMember("rigidbody") || !prefab["rigidbody"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing rigidbody"); continue; }
            const auto& rigidbodyJson = prefab["rigidbody"];
            float mass = (rigidbodyJson.HasMember("mass") && rigidbodyJson["mass"].IsNumber()) ? (float)rigidbodyJson["mass"].GetDouble() : 0.f;
            float rest = (rigidbodyJson.HasMember("restitution") && rigidbodyJson["restitution"].IsNumber()) ? (float)rigidbodyJson["restitution"].GetDouble() : 0.f;

        
            bool hasColliderFlags = false; bool isTrigger = false; unsigned category = 0; unsigned mask = 0;
            if (prefab.HasMember("collider") && prefab["collider"].IsObject()) {
                const auto& cj = prefab["collider"];
                hasColliderFlags = true;
                if (cj.HasMember("isTrigger") && cj["isTrigger"].IsBool()) isTrigger = cj["isTrigger"].GetBool();
                if (cj.HasMember("category") && cj["category"].IsUint()) category = cj["category"].GetUint();
                if (cj.HasMember("mask") && cj["mask"].IsUint())     mask = cj["mask"].GetUint();
            }


            Prefab::Register(name, [=](ECS& e) {
                const Entity entitity = e.CreateEntity();
                e.AddComponent<NameComponent>(entitity, NameComponent{ name });

                // Transform
                TransformComponent transform{};
                transform.position = CEVec3{ px, py, pz };
                transform.rotation = rotation;
                transform.size = size;
                e.AddComponent<TransformComponent>(entitity, transform);

                // Shape
                ShapeComponent shapecomponent{};
                shapecomponent.type = (ShapeType)typeInt;
                shapecomponent.color = color;
                shapecomponent.enableTexture = enableTexture;
                shapecomponent.textureName = textureName;
                e.AddComponent<ShapeComponent>(entitity, shapecomponent);
                const float cw = (width > 0.f ? width : size.x);
                const float ch = (height > 0.f ? height : size.y);
                
                // RigidBody (no velocity field on your component)
                e.AddComponent<RigidBodyComponent>(entitity, CEVec3{ 0,0,0 }, mass, rest);

                // Collider (AABB for robustness)
                {
                    auto aabb = MakeCenteredAABB3(transform.position, width, height);
                    e.AddComponent<ColliderComponent>(entitity, ColliderComponent{ std::move(aabb) });
                
                }
                });

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
        auto* t = ecs.GetComponent<TransformComponent>(e);
        auto* shape = ecs.GetComponent<ShapeComponent>(e);
        auto* rb = ecs.GetComponent<RigidBodyComponent>(e);
        if (!t || !shape || !rb) {
            LOGE(Core, std::string("Save failed: entity missing Transform/Shape/RigidBody."));
            return false;
        }
        if (shape->type != ShapeType::Rectangle) {
            LOGE(Core, std::string("Save failed: only Rectangle (Box) is supported."));
            return false;
        }
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


        auto& A = doc.GetAllocator();
        if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
            Value arr(kArrayType);
            doc.AddMember("prefabs", arr, A);
        }
        Value& prefabs = doc["prefabs"];

        // Build object
        Value object(kObjectType);
        object.AddMember("type", "Box", A);

        // name
        {
            Value nm; nm.SetString(newName.c_str(), (SizeType)newName.size(), A);
            object.AddMember("name", nm, A);
        }

        // transform (position + rotation + size)
        {
            Value position(kObjectType);
            position.AddMember("x", t->position.x, A);
            position.AddMember("y", t->position.y, A);
            position.AddMember("z", t->position.z, A);

            Value transform(kObjectType);
            transform.AddMember("position", position, A);
            transform.AddMember("rotation", t->rotation, A);

            Value sizeJ(kObjectType);
            sizeJ.AddMember("x", t->size.x, A);
            sizeJ.AddMember("y", t->size.y, A);
            transform.AddMember("size", sizeJ, A);

            object.AddMember("transform", transform, A);
        }

        // shape
        {
            Value shapeJson(kObjectType);
            shapeJson.AddMember("kind", "Rectangle", A);      
            shapeJson.AddMember("type", (int)shape->type, A);       
            shapeJson.AddMember("width", t->size.x, A);
            shapeJson.AddMember("height", t->size.y, A);
            shapeJson.AddMember("size", std::max(t->size.x, t->size.y), A);  

            // color
            Value color(kObjectType);
            color.AddMember("r", shape->color.x, A);
            color.AddMember("g", shape->color.y, A);
            color.AddMember("b", shape->color.z, A);
            shapeJson.AddMember("color", color, A);

            // texture flags
            shapeJson.AddMember("enableTexture", shape->enableTexture, A);
            if (shape->enableTexture && !shape->textureName.empty()) {
                Value tex; tex.SetString(shape->textureName.c_str(), (SizeType)shape->textureName.size(), A);
                shapeJson.AddMember("textureName", tex, A);
            }

            object.AddMember("shape", shapeJson, A);
        }

       
       // rigidbody (mass, restitution)
   
        {
            rapidjson::Value rigidbody(rapidjson::kObjectType);
            rigidbody.AddMember("mass", rb->getMass(), A);         
            rigidbody.AddMember("restitution", rb->getCoeff(), A);
            object.AddMember("rigidbody", rigidbody, A);
        }


        // ---- helpers for name matching (add these) ----
        auto trim = [](std::string s) {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) return std::string{};
            return s.substr(a, b - a + 1);
            };
        auto lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
            };
      // Upsert by key (matchName overrides newName for replacement)
        const std::string keyRaw = matchName.empty() ? newName : matchName;
        const std::string key = lower(trim(keyRaw));

        bool replaced = false;
        for (auto& p : prefabs.GetArray()) {
            if (!p.IsObject() || !p.HasMember("name") || !p["name"].IsString()) continue;
            const std::string pname = lower(trim(p["name"].GetString()));
            if (pname == key) {
                p.CopyFrom(object, A);
                replaced = true;
                break;
            }
        }
        if (!replaced) prefabs.PushBack(object, A);


        // Write back
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> wr(sb);
        wr.SetMaxDecimalPlaces(6);
        doc.Accept(wr);

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) {
            LOGE(Core, std::string("Cannot open for writing: ") + path);
            return false;
        }
        out.write(sb.GetString(), static_cast<std::streamsize>(sb.GetSize()));

        LOGI(Core, std::string(replaced ? "Updated prefab: " : "Saved new prefab: ") + newName);
        return true;
    }
    void ReloadPrefabsFromDisk(const std::string& path)
    {
        // Clear existing factories
        for (const auto& n : Prefab::Names())
            Prefab::Unregister(n);

        LoadPrefabsFromJson_RJ(path);

      
        for (const auto& n : Prefab::Names())
            LOGI(Core, std::string("Prefab available: ") + n);
    }
   
 void InitPrefabs()
{
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(ent))
            e.AddComponent<TransformComponent>(ent, TransformComponent{});
    });

    const char* JSON_PATH = "assets/prefabs/prefab.json"; 
    LoadPrefabsFromJson_RJ(JSON_PATH);
}

} 
