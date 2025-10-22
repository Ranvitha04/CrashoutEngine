/*!
@file        prefab_init.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Sets up prefabs: fallback, a built-in example, and de-serializes
             external prefabs from projects/Descent/assets/prefabs/prefabs.txt.
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
    if (!ifs) { LOGE(Core, "JSON open failed: %s", path); return false; }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    if (doc.HasParseError() || !doc.IsObject()) {
        LOGE(Core, "JSON parse error in %s", path);
        return false;
    }
    if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
        LOGE(Core, "JSON missing 'prefabs' array in %s", path);
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
            if (!p.HasMember("shape") || !p["shape"].IsObject()) { LOGE(Core, "Box '%s' missing shape", name.c_str()); continue; }
            const auto& shj = p["shape"];
            float w = shj.HasMember("width") && shj["width"].IsNumber() ? (float)shj["width"].GetDouble() : 1.0f;
            float h = shj.HasMember("height") && shj["height"].IsNumber() ? (float)shj["height"].GetDouble() : 1.0f;

            // transform.position
            if (!p.HasMember("transform") || !p["transform"].IsObject()) { LOGE(Core, "Box '%s' missing transform", name.c_str()); continue; }
            const auto& tj = p["transform"];
            if (!tj.HasMember("position") || !tj["position"].IsObject()) { LOGE(Core, "Box '%s' missing position", name.c_str()); continue; }
            const auto& pos = tj["position"];
            float px = pos.HasMember("x") && pos["x"].IsNumber() ? (float)pos["x"].GetDouble() : 0.f;
            float py = pos.HasMember("y") && pos["y"].IsNumber() ? (float)pos["y"].GetDouble() : 0.f;
            float pz = pos.HasMember("z") && pos["z"].IsNumber() ? (float)pos["z"].GetDouble() : 0.f;

            // rigidbody
            if (!p.HasMember("rigidbody") || !p["rigidbody"].IsObject()) { LOGE(Core, "Box '%s' missing rigidbody", name.c_str()); continue; }
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
                sc.width = w; sc.height = h;
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
static rapidjson::Value MakeVec3(rapidjson::Document::AllocatorType& A, const CEVec3& v) {
    using namespace rapidjson;
    Value obj(kObjectType);
    obj.AddMember("x", v.x, A);
    obj.AddMember("y", v.y, A);
    obj.AddMember("z", v.z, A);
    return obj;
}
bool SaveBoxPrefabToJson_RJ(CrashoutsEngine::ECS& ecs,
    Entity e,
    const std::string& name,
    const std::string& path 
{
    using namespace rapidjson;
    using namespace Component;


    auto* t = ecs.GetComponent<TransformComponent>(e);
    auto* sh = ecs.GetComponent<ShapeComponent>(e);
    auto* rb = ecs.GetComponent<RigidBodyComponent>(e);
    if (!t || !sh || !rb) {
        LOGE(Core, "Save failed: entity missing Transform/Shape/RigidBody.");
        return false;
    }
    if (sh->type != ShapeType::Rectangle) {
        LOGE(Core, "Save failed: only Rectangle (Box) is supported.");
        return false;
    }

    Document doc;
    {
        std::ifstream ifs(path);
        if (ifs) {
            IStreamWrapper isw(ifs);
            doc.ParseStream(isw);
        }
        else {
            doc.SetObject();
        }
    }
    if (!doc.IsObject()) doc.SetObject();

    auto& A = doc.GetAllocator();
    if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
        Value arr(kArrayType);
        doc.AddMember("prefabs", arr, A);
    }
    Value& prefabs = doc["prefabs"];

    Value obj(kObjectType);

    obj.AddMember("type", "Box", A);
    {
        Value nm;
        nm.SetString(name.c_str(), (SizeType)name.size(), A);
        obj.AddMember("name", nm, A);
    }

    {
        Value pos(kObjectType);
        pos.AddMember("x", t->position.x, A);
        pos.AddMember("y", t->position.y, A);
        pos.AddMember("z", t->position.z, A);

        Value tr(kObjectType);
        tr.AddMember("position", pos, A);
        obj.AddMember("transform", tr, A);
    }

    // shape (Rectangle: width/height)
    {
        Value shj(kObjectType);
        shj.AddMember("kind", "Rectangle", A);
        shj.AddMember("width", sh->width, A);
        shj.AddMember("height", sh->height, A);
        obj.AddMember("shape", shj, A);
    }

    {
        auto r = rb->reflection();  /
        Value rbj(kObjectType);
        rbj.AddMember("mass", r.mass, A);
        rbj.AddMember("restitution", r.restitution, A);
        obj.AddMember("rigidbody", rbj, A);
    }

    bool replaced = false;
    for (auto& p : prefabs.GetArray()) {
        if (p.IsObject() && p.HasMember("name") && p["name"].IsString()
            && name == std::string(p["name"].GetString())) {
            p = obj;          
            replaced = true;
            break;
        }
    }
    if (!replaced) prefabs.PushBack(obj, A);

    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    doc.Accept(writer);

    std::ofstream out(path, std::ios::binary);
    if (!out) { LOGE(Core, "Cannot open %s for writing", path.c_str()); return false; }
    out.write(sb.GetString(), static_cast<std::streamsize>(sb.GetSize()));
    return true;
}

void InitPrefabs()
{
    // Fallback so unknown names don't crash
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto ent = e.CreateEntity();
        e.AddComponent<NameComponent>(ent, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(ent))
            e.AddComponent<TransformComponent>(ent, TransformComponent{});
        });

    const char* JSON_PATH = "assets/prefabs/prefab.json";
    if (!LoadPrefabsFromJson_RJ(JSON_PATH)) {
        LOGE(Core, "Failed to load %s", JSON_PATH);
    }

    // Optional: list all names for your UI/debug
    for (const auto& n : Prefab::Names())
        LOGI(Core, std::string("Prefab available: ") + n);
}

