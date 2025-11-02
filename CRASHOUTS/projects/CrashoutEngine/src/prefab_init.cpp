/*!
@file        prefab_init.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       
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
  

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    if (doc.HasParseError() || !doc.IsObject()) {
       
        return false;
    }
    if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
        
        return false;
    }

    const auto& arr = doc["prefabs"];
    for (auto& p : arr.GetArray()) {
        if (!p.IsObject()) continue;

        // type + name
        if (!p.HasMember("type") || !p["type"].IsString()) continue;
        if (!p.HasMember("name") || !p["name"].IsString()) continue;

        std::string type = p["type"].GetString();
        std::string name = p["name"].GetString();

        if (type == "Box") {
            // shape
            if (!p.HasMember("shape") || !p["shape"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing shape"); continue; }
            const auto& shj = p["shape"];
            float w = shj.HasMember("width") && shj["width"].IsNumber() ? (float)shj["width"].GetDouble() : 1.0f;
            float h = shj.HasMember("height") && shj["height"].IsNumber() ? (float)shj["height"].GetDouble() : 1.0f;

            // transform.position
            if (!p.HasMember("transform") || !p["transform"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing transform"); continue; }
            const auto& tj = p["transform"];
            if (!tj.HasMember("position") || !tj["position"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing position"); continue; }
            const auto& pos = tj["position"];
            float px = pos.HasMember("x") && pos["x"].IsNumber() ? (float)pos["x"].GetDouble() : 0.f;
            float py = pos.HasMember("y") && pos["y"].IsNumber() ? (float)pos["y"].GetDouble() : 0.f;
            float pz = pos.HasMember("z") && pos["z"].IsNumber() ? (float)pos["z"].GetDouble() : 0.f;

            // rigidbody
            if (!p.HasMember("rigidbody") || !p["rigidbody"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing rigidbody"); continue; }
            const auto& rbj = p["rigidbody"];
            float mass = rbj.HasMember("mass") && rbj["mass"].IsNumber() ? (float)rbj["mass"].GetDouble() : 0.f;
            float rest = rbj.HasMember("restitution") && rbj["restitution"].IsNumber() ? (float)rbj["restitution"].GetDouble() : 0.f;

            // Register factory
            Prefab::Register(name, [=](ECS& e) {
                auto ent = e.CreateEntity();
                e.AddComponent<NameComponent>(ent, NameComponent{ name });

                TransformComponent t{};
                t.position = CEVec3{ px, py, pz };
                e.AddComponent<TransformComponent>(ent, t);

                ShapeComponent sc{};
                sc.type = ShapeType::Rectangle;
                e.AddComponent<ShapeComponent>(ent, sc);

                e.AddComponent<ColliderComponent>(ent, MakeCenteredAABB3(t.position, w, h));
                e.AddComponent<RigidBodyComponent>(ent, CEVec3{ 0,0,0 }, mass, rest);
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
        const std::string& path ,
        const std::string& matchName )
    {
        using namespace Component;
        using namespace rapidjson;

        // Validate components
        auto* t = ecs.GetComponent<TransformComponent>(e);
        auto* sh = ecs.GetComponent<ShapeComponent>(e);
        auto* rb = ecs.GetComponent<RigidBodyComponent>(e);
        if (!t || !sh || !rb) {
            LOGE(Core, std::string("Save failed: entity missing Transform/Shape/RigidBody."));
            return false;
        }
        if (sh->type != ShapeType::Rectangle) {
            LOGE(Core, std::string("Save failed: only Rectangle (Box) is supported."));
            return false;
        }

        // Load existing doc (or init)
        Document doc;
        {
            std::ifstream in(path, std::ios::binary);
            if (in) {
                IStreamWrapper isw(in);
                doc.ParseStream(isw);
                if (doc.HasParseError() || !doc.IsObject()) doc.SetObject();
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

        // Build prefab object
        Value obj(kObjectType);
        obj.AddMember("type", "Box", A);

        {   
            Value nm;
            nm.SetString(newName.c_str(), (SizeType)newName.size(), A);
            obj.AddMember("name", nm, A);
        }

        // transform.position
        {
            Value pos(kObjectType);
            pos.AddMember("x", t->position.x, A);
            pos.AddMember("y", t->position.y, A);
            pos.AddMember("z", t->position.z, A);

            Value tr(kObjectType);
            tr.AddMember("position", pos, A);
            obj.AddMember("transform", tr, A);
        }

        // shape (Rectangle)
        {
            Value shj(kObjectType);
            shj.AddMember("kind", "Rectangle", A);
            obj.AddMember("shape", shj, A);
        }

        // rigidbody
        {
            auto r = rb->reflection();
            Value rbj(kObjectType);
            rbj.AddMember("mass", r.mass, A);
            rbj.AddMember("restitution", r.restitution, A);
            obj.AddMember("rigidbody", rbj, A);
        }

        // Upsert by key
        const std::string& key = matchName.empty() ? newName : matchName;
        bool replaced = false;
        for (auto& p : prefabs.GetArray()) {
            if (p.IsObject() && p.HasMember("name") && p["name"].IsString()
                && key == std::string(p["name"].GetString())) {
                p.CopyFrom(obj, A);  
                replaced = true;
                break;
            }
        }
        if (!replaced) prefabs.PushBack(obj, A);

        // Write
        StringBuffer sb;
        PrettyWriter<StringBuffer> wr(sb);
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


    void InitPrefabs()
    {
        
        Prefab::SetFallback([](ECS& e, const std::string& name) {
            auto ent = e.CreateEntity();
            e.AddComponent<NameComponent>(ent, NameComponent{ name });
            if (!e.HasComponent<TransformComponent>(ent))
                e.AddComponent<TransformComponent>(ent, TransformComponent{});
            });

        const char* JSON_PATH = "assets/prefabs/prefab.json";
        if (!LoadPrefabsFromJson_RJ(JSON_PATH)) {
            LOGE(Core, std::string("Failed to load ") + JSON_PATH);
        }

        
        for (const auto& n : Prefab::Names())
            LOGI(Core, std::string("Prefab available: ") + n);
    }

}