/*!
@file        prefab_init.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Prefab load/save (RapidJSON) matching your scene:
             Transform {position, rotation, size},
             Shape {type, width, height, size, color, enableTexture, textureName},
             RigidBody {mass, restitution},
             Collider (AABB from width/height)
             Text, Camera, Script, AudioEmitter, Gui, PPComponent
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
#include "scriptcomponent.h"
#include "audiocomponent.h"   
#include "guicomponent.h"
#include "ppcomponent.h"
#include "camera.h"
using namespace CrashoutsEngine;
using namespace Component;
/**
 * @brief Create an AABB (min/max corners) from center and width/height.
 */
 // Compute-and-return AABB unique_ptr based on center+dimensions
static std::unique_ptr<Collision::AABBRect>
MakeCenteredAABB3(const CEVec3& center, float width, float height)
{
    const float halfwidth = 0.5f * width, halfheight = 0.5f * height;
    CEVec3 minCorner{ center.x - halfwidth, center.y - halfheight, center.z };
    CEVec3 maxCorner{ center.x + halfwidth, center.y + halfheight, center.z };
    return std::make_unique<Collision::AABBRect>(minCorner, maxCorner);
}
/**
 * @brief Parse JSON at 'path', loop over "prefabs" array, and register
 *        a Prefab::Factory lambda for each known type (currently "Box").
 * @return true on success, false on I/O or parse error
 */
 // Static linkage: local to this translation unit
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
    // Grab the array view and itterate each prefab JSON object
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
            if (!prefabs.HasMember("shape") || !prefabs["shape"].IsObject()) {
                LOGE(Core, std::string("Box '") + name + "' missing shape");
                continue;
            }
            const auto& shapeJson = prefabs["shape"];
            float width = (shapeJson.HasMember("width") && shapeJson["width"].IsNumber())
                ? static_cast<float>(shapeJson["width"].GetDouble()) : 1.0f;
            float height = (shapeJson.HasMember("height") && shapeJson["height"].IsNumber())
                ? static_cast<float>(shapeJson["height"].GetDouble()) : 1.0f;

            int typeInt = (shapeJson.HasMember("type") && shapeJson["type"].IsInt())
                ? shapeJson["type"].GetInt() : static_cast<int>(ShapeType::Rectangle);

            CEVec3 color{ 0, 0, 0 };
            if (shapeJson.HasMember("color") && shapeJson["color"].IsObject()) {
                const auto& colorJson = shapeJson["color"];
                if (colorJson.HasMember("r") && colorJson["r"].IsNumber())
                    color.x = static_cast<float>(colorJson["r"].GetDouble());
                if (colorJson.HasMember("g") && colorJson["g"].IsNumber())
                    color.y = static_cast<float>(colorJson["g"].GetDouble());
                if (colorJson.HasMember("b") && colorJson["b"].IsNumber())
                    color.z = static_cast<float>(colorJson["b"].GetDouble());
            }

            const bool enableTexture =
                (shapeJson.HasMember("enableTexture") && shapeJson["enableTexture"].IsBool())
                ? shapeJson["enableTexture"].GetBool() : false;

            std::string textureName;
            if (enableTexture &&
                shapeJson.HasMember("textureName") && shapeJson["textureName"].IsString()) {
                textureName = shapeJson["textureName"].GetString();
            }

            // ---- transform ----
            if (!prefabs.HasMember("transform") || !prefabs["transform"].IsObject()) {
                LOGE(Core, std::string("Box '") + name + "' missing transform");
                continue;
            }
            const auto& transformJson = prefabs["transform"];
            if (!transformJson.HasMember("position") || !transformJson["position"].IsObject()) {
                LOGE(Core, std::string("Box '") + name + "' missing position");
                continue;
            }

            const auto& posJ = transformJson["position"];
            float posX = (posJ.HasMember("x") && posJ["x"].IsNumber())
                ? static_cast<float>(posJ["x"].GetDouble()) : 0.f;
            float posY = (posJ.HasMember("y") && posJ["y"].IsNumber())
                ? static_cast<float>(posJ["y"].GetDouble()) : 0.f;
            float posZ = (posJ.HasMember("z") && posJ["z"].IsNumber())
                ? static_cast<float>(posJ["z"].GetDouble()) : 0.f;

            float rotation =
                (transformJson.HasMember("rotation") && transformJson["rotation"].IsNumber())
                ? static_cast<float>(transformJson["rotation"].GetDouble()) : 0.f;

            CEVec2 size{ 1.f, 1.f };
            if (transformJson.HasMember("size") && transformJson["size"].IsObject()) {
                const auto& sizeJ = transformJson["size"];
                if (sizeJ.HasMember("x") && sizeJ["x"].IsNumber())
                    size.x = static_cast<float>(sizeJ["x"].GetDouble());
                if (sizeJ.HasMember("y") && sizeJ["y"].IsNumber())
                    size.y = static_cast<float>(sizeJ["y"].GetDouble());
            }

            // ---- rigidbody ----
            if (!prefabs.HasMember("rigidbody") || !prefabs["rigidbody"].IsObject()) {
                LOGE(Core, std::string("Box '") + name + "' missing rigidbody");
                continue;
            }
            const auto& rigidbodyJson = prefabs["rigidbody"];
            float mass = (rigidbodyJson.HasMember("mass") && rigidbodyJson["mass"].IsNumber())
                ? static_cast<float>(rigidbodyJson["mass"].GetDouble()) : 0.f;
            float restitution =
                (rigidbodyJson.HasMember("restitution") && rigidbodyJson["restitution"].IsNumber())
                ? static_cast<float>(rigidbodyJson["restitution"].GetDouble()) : 0.f;

            // ---- collider flags  ----
            bool hasColliderFlags = false;
            bool isTrigger = false;
            unsigned category = 0;
            unsigned mask = 0;
            if (prefabs.HasMember("collider") && prefabs["collider"].IsObject()) {
                const auto& colliderJson = prefabs["collider"];
                hasColliderFlags = true;
                if (colliderJson.HasMember("isTrigger") && colliderJson["isTrigger"].IsBool())
                    isTrigger = colliderJson["isTrigger"].GetBool();
                if (colliderJson.HasMember("category") && colliderJson["category"].IsUint())
                    category = colliderJson["category"].GetUint();
                if (colliderJson.HasMember("mask") && colliderJson["mask"].IsUint())
                    mask = colliderJson["mask"].GetUint();
            }

            // ---- text ----
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

            // ---- camera ----
            bool hasCameraComponent = false;
            CEVec3 cameraPosition{ 0.f, 0.f, 0.f };
            float cameraZoom = 1.0f;
            CEVec2 cameraXBounds{ 0.f, 0.f };
            CEVec2 cameraYBounds{ 0.f, 0.f };

            if (prefabs.HasMember("camera") && prefabs["camera"].IsObject()) {
                const auto& cameraJson = prefabs["camera"];
                hasCameraComponent = true;

                if (cameraJson.HasMember("position") && cameraJson["position"].IsObject()) {
                    const auto& pos = cameraJson["position"];
                    if (pos.HasMember("x") && pos["x"].IsNumber())
                        cameraPosition.x = static_cast<float>(pos["x"].GetDouble());
                    if (pos.HasMember("y") && pos["y"].IsNumber())
                        cameraPosition.y = static_cast<float>(pos["y"].GetDouble());
                    if (pos.HasMember("z") && pos["z"].IsNumber())
                        cameraPosition.z = static_cast<float>(pos["z"].GetDouble());
                }

                if (cameraJson.HasMember("zoom") && cameraJson["zoom"].IsNumber())
                    cameraZoom = static_cast<float>(cameraJson["zoom"].GetDouble());

                if (cameraJson.HasMember("xBounds") && cameraJson["xBounds"].IsObject()) {
                    const auto& xb = cameraJson["xBounds"];
                    if (xb.HasMember("x") && xb["x"].IsNumber())
                        cameraXBounds.x = static_cast<float>(xb["x"].GetDouble());
                    if (xb.HasMember("y") && xb["y"].IsNumber())
                        cameraXBounds.y = static_cast<float>(xb["y"].GetDouble());
                }

                if (cameraJson.HasMember("yBounds") && cameraJson["yBounds"].IsObject()) {
                    const auto& yb = cameraJson["yBounds"];
                    if (yb.HasMember("x") && yb["x"].IsNumber())
                        cameraYBounds.x = static_cast<float>(yb["x"].GetDouble());
                    if (yb.HasMember("y") && yb["y"].IsNumber())
                        cameraYBounds.y = static_cast<float>(yb["y"].GetDouble());
                }
            }

            // ---- script ----
            bool hasScriptComponent = false;
            std::string scriptClassName;
            uint32_t scriptEntityID = 0; 

            if (prefabs.HasMember("script") && prefabs["script"].IsObject()) {
                const auto& sJ = prefabs["script"];
                if (sJ.HasMember("className") && sJ["className"].IsString())
                    scriptClassName = sJ["className"].GetString();
                if (sJ.HasMember("entityID") && sJ["entityID"].IsUint())
                    scriptEntityID = sJ["entityID"].GetUint();
                hasScriptComponent = !scriptClassName.empty();
            }
            // --- audio  ---
            bool hasAudioEmitter = false;
            std::string audioEventPath;
            float audioVolume = 1.0f;
            float audioPitch = 1.0f;
            bool audioSpatial3D = false;
            bool audioLooping = false;

            if (prefabs.HasMember("audio") && prefabs["audio"].IsObject()) {
                const auto& aJ = prefabs["audio"];
                if (aJ.HasMember("eventPath") && aJ["eventPath"].IsString())
                    audioEventPath = aJ["eventPath"].GetString();
                if (aJ.HasMember("volume") && aJ["volume"].IsNumber())
                    audioVolume = static_cast<float>(aJ["volume"].GetDouble());
                if (aJ.HasMember("pitch") && aJ["pitch"].IsNumber())
                    audioPitch = static_cast<float>(aJ["pitch"].GetDouble());
                if (aJ.HasMember("spatial3D") && aJ["spatial3D"].IsBool())
                    audioSpatial3D = aJ["spatial3D"].GetBool();
                if (aJ.HasMember("looping") && aJ["looping"].IsBool())
                    audioLooping = aJ["looping"].GetBool();

                hasAudioEmitter = !audioEventPath.empty();
            }
            // --- gui ---
            bool hasGuiComponent = false;
            unsigned guiTypeMask = 0;

            
            auto uiTypeFromString = [](const std::string& s) -> unsigned {
                using Component::UIType;
                if (s == "IMAGE")     return static_cast<unsigned>(UIType::IMAGE);
                if (s == "TEXT")      return static_cast<unsigned>(UIType::TEXT);
                if (s == "BUTTON")    return static_cast<unsigned>(UIType::BUTTON);
                if (s == "INPUTTEXT") return static_cast<unsigned>(UIType::INPUTTEXT);
                return 0u;
                };

            if (prefabs.HasMember("gui") && prefabs["gui"].IsObject()) {
                const auto& guiJ = prefabs["gui"];

               
                if (guiJ.HasMember("uiType")) {
                    const auto& t = guiJ["uiType"];
                    if (t.IsString()) {
                        guiTypeMask |= uiTypeFromString(t.GetString());
                    }
                    else if (t.IsArray()) {
                        for (auto& v : t.GetArray()) {
                            if (v.IsString()) guiTypeMask |= uiTypeFromString(v.GetString());
                        }
                    }
                    else if (t.IsUint()) {
                        guiTypeMask = t.GetUint();
                    }
                }

                hasGuiComponent = (guiTypeMask != 0u);
            }
            // --- post-processing ---
            bool hasPP = false;
            bool ppActive = true;
            float ppBrightness = 0.f;
            float ppContrast = 0.f;
            float ppSaturation = 0.f;

            if (prefabs.HasMember("postprocessing") && prefabs["postprocessing"].IsObject()) {
                const auto& ppJ = prefabs["postprocessing"];
                if (ppJ.HasMember("active") && ppJ["active"].IsBool())
                    ppActive = ppJ["active"].GetBool();
                if (ppJ.HasMember("brightness") && ppJ["brightness"].IsNumber())
                    ppBrightness = static_cast<float>(ppJ["brightness"].GetDouble());
                if (ppJ.HasMember("contrast") && ppJ["contrast"].IsNumber())
                    ppContrast = static_cast<float>(ppJ["contrast"].GetDouble());
                if (ppJ.HasMember("saturation") && ppJ["saturation"].IsNumber())
                    ppSaturation = static_cast<float>(ppJ["saturation"].GetDouble());

                hasPP = true;
            }


            // ---- register factory ----
            Prefab::Register(
                name,
                [name,
                // transform
                posX, posY, posZ, rotation, size,
                // shape
                typeInt, color, enableTexture, textureName, width, height,
                // rigidbody
                mass, restitution,
                // text
                hasTextComponent, jsonFont, jsonText, jsonScale, jsonOffset, jsonColor, jsonCentered, jsonIsWorldSpace,
                // camera
                hasCameraComponent, cameraPosition, cameraZoom, cameraXBounds, cameraYBounds,
                // script
                hasScriptComponent, scriptClassName,
                //audio
                hasAudioEmitter, audioEventPath, audioVolume, audioPitch, audioSpatial3D, audioLooping,
                hasGuiComponent, guiTypeMask,
                hasPP, ppActive, ppBrightness, ppContrast, ppSaturation
                ](ECS& e)
            {
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
                shapeComp.type = static_cast<ShapeType>(typeInt);
                shapeComp.color = color;
                shapeComp.enableTexture = enableTexture;
                shapeComp.textureName = textureName;
                e.AddComponent<ShapeComponent>(entitty, shapeComp);

                // RigidBody
                e.AddComponent<RigidBodyComponent>(entitty, CEVec3{ 0,0,0 }, mass, restitution);

                // Collider (AABB)
                {
                    auto aabb = MakeCenteredAABB3(transform.position, width, height);
                    e.AddComponent<ColliderComponent>(entitty, ColliderComponent{ std::move(aabb) });
                }

                // Text
                if (hasTextComponent) {
                    TextComponent tc{};
                    tc.font = jsonFont;
                    tc.text = jsonText;
                    tc.scale = jsonScale;
                    tc.offset = jsonOffset;
                    tc.color = jsonColor;
                    tc.centered = jsonCentered;
                    tc.isWorldSpace = jsonIsWorldSpace;
                    e.AddComponent<TextComponent>(entitty, tc);
                }

                // Camera
                if (hasCameraComponent) {
                    CameraComponent cc{};
                    cc.position = cameraPosition;
                    cc.zoom = cameraZoom;
                    cc.xBounds = cameraXBounds;
                    cc.yBounds = cameraYBounds;
                    e.AddComponent<CameraComponent>(entitty, cc);
                }

                // Script
                if (hasScriptComponent && !scriptClassName.empty()) {
                    ScriptComponent sc(scriptClassName,
                        static_cast<uint32_t>(entitty)); 
                    e.AddComponent<ScriptComponent>(entitty, sc);
                
                }
                // --- AudioEmitterComponent 
                if (hasAudioEmitter && !audioEventPath.empty()) {
                    Component::AudioEmitterComponent a{};
                    a.eventPath = audioEventPath;
                    a.volume = audioVolume;
                    a.pitch = audioPitch;
                    a.spatial3D = audioSpatial3D;
                    a.looping = audioLooping;
            
                    e.AddComponent<Component::AudioEmitterComponent>(entitty, a);
                }
                // GUI
                if (hasGuiComponent) {
                    Component::GuiComponent gui{};
                    gui.uiType = static_cast<Component::UIType>(guiTypeMask);
                    e.AddComponent<Component::GuiComponent>(entitty, gui);
                }
                if (hasPP) {
                    Component::PPComponent pp{};
                    pp.active = ppActive;
                    pp.brightness = ppBrightness;
                    pp.contrast = ppContrast;
                    pp.saturation = ppSaturation;
                    e.AddComponent<Component::PPComponent>(entitty, pp);
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
    /**
 * @brief Save entity 'e' as a Box prefab into file 'path'.
 *        If 'matchName' matches an existing prefab name, it replaces that entry,
 *        otherwise it appends a new prefab with 'newName'.
 * @return true on success
 */
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
        // camera (
        auto* camera = ecs.GetComponent<CameraComponent>(e);
        if (camera) {
            rapidjson::Value cameraJson(rapidjson::kObjectType);

            // Position
            Value camPos(kObjectType);
            camPos.AddMember("x", camera->position.x, allocate);
            camPos.AddMember("y", camera->position.y, allocate);
            camPos.AddMember("z", camera->position.z, allocate);
            cameraJson.AddMember("position", camPos, allocate);

            // Zoom
            cameraJson.AddMember("zoom", camera->zoom, allocate);

            // X Bounds
            Value xBounds(kObjectType);
            xBounds.AddMember("x", camera->xBounds.x, allocate);
            xBounds.AddMember("y", camera->xBounds.y, allocate);
            cameraJson.AddMember("xBounds", xBounds, allocate);

            // Y Bounds
            Value yBounds(kObjectType);
            yBounds.AddMember("x", camera->yBounds.x, allocate);
            yBounds.AddMember("y", camera->yBounds.y, allocate);
            cameraJson.AddMember("yBounds", yBounds, allocate);

            obj.AddMember("camera", cameraJson, allocate);
        }
        // text (optional - only if entity has TextComponent)
        if (auto* txt = ecs.GetComponent<TextComponent>(e)) {
            rapidjson::Value textJson(rapidjson::kObjectType);

            // font
            if (!txt->font.empty()) {
                rapidjson::Value fontV;
                fontV.SetString(txt->font.c_str(),
                    static_cast<rapidjson::SizeType>(txt->font.size()),
                    allocate);
                textJson.AddMember("font", fontV, allocate);
            }

            // text string
            {
                rapidjson::Value textV;
                textV.SetString(txt->text.c_str(),
                    static_cast<rapidjson::SizeType>(txt->text.size()),
                    allocate);
                textJson.AddMember("text", textV, allocate);
            }

            // scale
            textJson.AddMember("scale", static_cast<double>(txt->scale), allocate);

            // offset
            {
                rapidjson::Value off(rapidjson::kObjectType);
                off.AddMember("x", static_cast<double>(txt->offset.x), allocate);
                off.AddMember("y", static_cast<double>(txt->offset.y), allocate);
                textJson.AddMember("offset", off, allocate);
            }

            // color
            {
                rapidjson::Value col(rapidjson::kObjectType);
                col.AddMember("r", static_cast<double>(txt->color.x), allocate);
                col.AddMember("g", static_cast<double>(txt->color.y), allocate);
                col.AddMember("b", static_cast<double>(txt->color.z), allocate);
                textJson.AddMember("color", col, allocate);
            }

            // centered / isWorldSpace
            textJson.AddMember("centered", txt->centered, allocate);
            textJson.AddMember("isWorldSpace", txt->isWorldSpace, allocate);

            obj.AddMember("text", textJson, allocate);
        }

        // script 
        if (auto* script = ecs.GetComponent<ScriptComponent>(e)) {
            rapidjson::Value scriptJson(rapidjson::kObjectType);

            // className
            rapidjson::Value className;
            className.SetString(script->ClassName.c_str(),
                static_cast<rapidjson::SizeType>(script->ClassName.size()),
                allocate);
            scriptJson.AddMember("className", className, allocate);

            // entityID
            scriptJson.AddMember("entityID", static_cast<uint32_t>(script->EntityID), allocate);

            obj.AddMember("script", scriptJson, allocate);
        }
        // audio (optional - only if entity has AudioEmitterComponent)
        if (auto* audio = ecs.GetComponent<Component::AudioEmitterComponent>(e)) {
            rapidjson::Value audioJson(rapidjson::kObjectType);

            // eventPath
            if (!audio->eventPath.empty()) {
                rapidjson::Value ev;
                ev.SetString(audio->eventPath.c_str(),
                    static_cast<rapidjson::SizeType>(audio->eventPath.size()),
                    allocate);
                audioJson.AddMember("eventPath", ev, allocate);
            }

            // numeric + bool fields
            audioJson.AddMember("volume", static_cast<double>(audio->volume), allocate);
            audioJson.AddMember("pitch", static_cast<double>(audio->pitch), allocate);
            audioJson.AddMember("spatial3D", audio->spatial3D, allocate);
            audioJson.AddMember("looping", audio->looping, allocate);

            // Do NOT serialize runtime pointer:
            // audio->inst is runtime-only; skip it.

            obj.AddMember("audio", audioJson, allocate);
        }
        // 
        // (optional - only if entity has GuiComponent)
        if (auto* gui = ecs.GetComponent<Component::GuiComponent>(e)) {
            rapidjson::Value guiJson(rapidjson::kObjectType);

            unsigned mask = static_cast<unsigned>(gui->uiType);
            guiJson.AddMember("uiType", mask, allocate);


            obj.AddMember("gui", guiJson, allocate);
        }
        // postprocessing
        if (auto* pp = ecs.GetComponent<Component::PPComponent>(e)) {
            rapidjson::Value ppJson(rapidjson::kObjectType);
            ppJson.AddMember("active", pp->active, allocate);
            ppJson.AddMember("brightness", static_cast<double>(pp->brightness), allocate);
            ppJson.AddMember("contrast", static_cast<double>(pp->contrast), allocate);
            ppJson.AddMember("saturation", static_cast<double>(pp->saturation), allocate);
            obj.AddMember("postprocessing", ppJson, allocate);
        }

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
      // Upsert by key 
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
// ============================================================================
// Utility: reload all prefabs from disk (clear registry then load)
// ============================================================================
/**
 * @brief Unregister all factories, then re-read the JSON file and re-register.
 */
    void ReloadPrefabsFromDisk(const std::string& path)
    {
        // Clear existing factories
        for (const auto& n : Prefab::Names())
            Prefab::Unregister(n);

        // Re-load from JSON
        LoadPrefabsFromJson_RJ(path);

        //  log what’s available now
        for (const auto& n : Prefab::Names())
            LOGI(Core, std::string("Prefab available: ") + n);
    }
// ============================================================================
 // Bootstrap: set fallback factory and load initial prefab file
 // ============================================================================
 /**
  * @brief Set a fallback (creates minimal entity) and load JSON at startup.
  */
 void InitPrefabs()
{
    Prefab::SetFallback([](ECS& e, const std::string& name) {
        auto entity = e.CreateEntity();
        e.AddComponent<NameComponent>(entity, NameComponent{ name });
        if (!e.HasComponent<TransformComponent>(entity))
            e.AddComponent<TransformComponent>(entity, TransformComponent{});
    });

    const std::string JSON_PATH = CrashoutsEngine::Assets::rootPath + "prefabs/prefab.json";
    LoadPrefabsFromJson_RJ(JSON_PATH);
}

} 
