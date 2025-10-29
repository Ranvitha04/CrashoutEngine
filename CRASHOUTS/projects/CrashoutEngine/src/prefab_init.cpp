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

// RapidJSON
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace CrashoutsEngine;
using namespace Component;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static std::unique_ptr<Collision::AABBRect>
MakeCenteredAABB3(const CEVec3& center, float w, float h)
{
    const float hw = 0.5f * w, hh = 0.5f * h;
    CEVec3 mn{ center.x - hw, center.y - hh, center.z };
    CEVec3 mx{ center.x + hw, center.y + hh, center.z };
    return std::make_unique<Collision::AABBRect>(mn, mx);
}

// -----------------------------------------------------------------------------
// Forward decls
// -----------------------------------------------------------------------------
static bool LoadPrefabsFromJson_RJ(const std::string& path);

// -----------------------------------------------------------------------------
// JSON → Prefab Registry
// -----------------------------------------------------------------------------
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
    for (auto& p : arr) {
        if (!p.IsObject()) continue;
        if (!p.HasMember("type") || !p["type"].IsString()) continue;
        if (!p.HasMember("name") || !p["name"].IsString()) continue;

        const std::string type = p["type"].GetString();
        const std::string name = p["name"].GetString();

        // ----------------------
        // BOX PREFAB
        // ----------------------
        if (type == "Box") {
            // ---- shape ----
            if (!p.HasMember("shape") || !p["shape"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing shape"); continue; }
            const auto& shj = p["shape"];
            float width = (shj.HasMember("width") && shj["width"].IsNumber()) ? (float)shj["width"].GetDouble() : 1.0f;
            float height = (shj.HasMember("height") && shj["height"].IsNumber()) ? (float)shj["height"].GetDouble() : 1.0f;
            float shapeSize = (shj.HasMember("size") && shj["size"].IsNumber()) ? (float)shj["size"].GetDouble() : 1.0f;
            int   typeInt = (shj.HasMember("type") && shj["type"].IsInt()) ? shj["type"].GetInt() : (int)ShapeType::Rectangle;

            CEVec3 color{ 0,0,0 };
            if (shj.HasMember("color") && shj["color"].IsObject()) {
                const auto& cj = shj["color"];
                color.x = (cj.HasMember("r") && cj["r"].IsNumber()) ? (float)cj["r"].GetDouble() : 0.f;
                color.y = (cj.HasMember("g") && cj["g"].IsNumber()) ? (float)cj["g"].GetDouble() : 0.f;
                color.z = (cj.HasMember("b") && cj["b"].IsNumber()) ? (float)cj["b"].GetDouble() : 0.f;
            }
            // Normalize 255-range inputs to 0..1
            if (color.x > 1.f || color.y > 1.f || color.z > 1.f) {
                color.x *= 1.f / 255.f; color.y *= 1.f / 255.f; color.z *= 1.f / 255.f;
            }

            const bool enableTexture = (shj.HasMember("enableTexture") && shj["enableTexture"].IsBool()) ? shj["enableTexture"].GetBool() : false;
            std::string textureName;
            if (enableTexture && shj.HasMember("textureName") && shj["textureName"].IsString())
                textureName = shj["textureName"].GetString();

            // ---- transform ----
            if (!p.HasMember("transform") || !p["transform"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing transform"); continue; }
            const auto& tj = p["transform"];
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
            if (!p.HasMember("rigidbody") || !p["rigidbody"].IsObject()) { LOGE(Core, std::string("Box '") + name + "' missing rigidbody"); continue; }
            const auto& rbj = p["rigidbody"];
            float mass = (rbj.HasMember("mass") && rbj["mass"].IsNumber()) ? (float)rbj["mass"].GetDouble() : 0.f;
            float rest = (rbj.HasMember("restitution") && rbj["restitution"].IsNumber()) ? (float)rbj["restitution"].GetDouble() : 0.f;

            // ---- collider flags (optional) ----
            bool hasColliderFlags = false; bool isTrigger = false; unsigned category = 0; unsigned mask = 0;
            if (p.HasMember("collider") && p["collider"].IsObject()) {
                const auto& cj = p["collider"];
                hasColliderFlags = true;
                if (cj.HasMember("isTrigger") && cj["isTrigger"].IsBool()) isTrigger = cj["isTrigger"].GetBool();
                if (cj.HasMember("category") && cj["category"].IsUint()) category = cj["category"].GetUint();
                if (cj.HasMember("mask") && cj["mask"].IsUint())     mask = cj["mask"].GetUint();
            }

            // ----------------------
            // Register factory using captured, POD values
            // ----------------------
            Prefab::Register(name, [=](ECS& e) {
                const Entity ent = e.CreateEntity();
                e.AddComponent<NameComponent>(ent, NameComponent{ name });

                // Transform
                TransformComponent t{};
                t.position = CEVec3{ px, py, pz };
                t.rotation = rotation;
                t.size = size;
                e.AddComponent<TransformComponent>(ent, t);

                // Shape
                ShapeComponent sc{};
                sc.type = (ShapeType)typeInt;
                sc.width = width;
                sc.height = height;
                sc.size = shapeSize;
                sc.color = color;
                sc.enableTexture = enableTexture;
                sc.textureName = textureName;
                e.AddComponent<ShapeComponent>(ent, sc);

                // RigidBody (no velocity field on your component)
                e.AddComponent<RigidBodyComponent>(ent, CEVec3{ 0,0,0 }, mass, rest);

                // Collider (AABB for robustness)
                {
                    auto aabb = MakeCenteredAABB3(t.position, width, height);
                    e.AddComponent<ColliderComponent>(ent, ColliderComponent{ std::move(aabb) });
                    // Optional: apply collider flags if your component exposes them
                    // if (hasColliderFlags) { /* set isTrigger/category/mask here */ }
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

// -----------------------------------------------------------------------------
// ECS → JSON (save one Box prefab)
// -----------------------------------------------------------------------------
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

        // Load / init doc
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

        // Build object
        Value obj(kObjectType);
        obj.AddMember("type", "Box", A);

        // name
        {
            Value nm; nm.SetString(newName.c_str(), (SizeType)newName.size(), A);
            obj.AddMember("name", nm, A);
        }

        // transform (position + rotation + size)
        {
            Value pos(kObjectType);
            pos.AddMember("x", t->position.x, A);
            pos.AddMember("y", t->position.y, A);
            pos.AddMember("z", t->position.z, A);

            Value tr(kObjectType);
            tr.AddMember("position", pos, A);
            tr.AddMember("rotation", t->rotation, A);

            Value sizeJ(kObjectType);
            sizeJ.AddMember("x", t->size.x, A);
            sizeJ.AddMember("y", t->size.y, A);
            tr.AddMember("size", sizeJ, A);

            obj.AddMember("transform", tr, A);
        }

        // shape (rich)
        {
            Value shj(kObjectType);
            shj.AddMember("kind", "Rectangle", A);        // legacy-friendly
            shj.AddMember("type", (int)sh->type, A);       // engine enum value
            shj.AddMember("width", sh->width, A);
            shj.AddMember("height", sh->height, A);
            shj.AddMember("size", sh->size, A);

            // color
            Value col(kObjectType);
            col.AddMember("r", sh->color.x, A);
            col.AddMember("g", sh->color.y, A);
            col.AddMember("b", sh->color.z, A);
            shj.AddMember("color", col, A);

            // texture flags
            shj.AddMember("enableTexture", sh->enableTexture, A);
            if (sh->enableTexture && !sh->textureName.empty()) {
                Value tex; tex.SetString(sh->textureName.c_str(), (SizeType)sh->textureName.size(), A);
                shj.AddMember("textureName", tex, A);
            }

            obj.AddMember("shape", shj, A);
        }

        // rigidbody (mass, restitution)
        {
            auto r = rb->reflection();
            Value rbj(kObjectType);
            rbj.AddMember("mass", r.mass, A);
            rbj.AddMember("restitution", r.restitution, A);
            obj.AddMember("rigidbody", rbj, A);
        }

        // Upsert by key (matchName overrides newName for replacement)
        const std::string& key = matchName.empty() ? newName : matchName;
        bool replaced = false;
        for (auto& p : prefabs.GetArray()) {
            if (p.IsObject() && p.HasMember("name") && p["name"].IsString() && key == std::string(p["name"].GetString())) {
                p.CopyFrom(obj, A);
                replaced = true;
                break;
            }
        }
        if (!replaced) prefabs.PushBack(obj, A);

        // Write back
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> wr(sb);
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

    // -----------------------------------------------------------------------------
    // Prefab bootstrap
    // -----------------------------------------------------------------------------
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

} // namespace CrashoutsEngine
