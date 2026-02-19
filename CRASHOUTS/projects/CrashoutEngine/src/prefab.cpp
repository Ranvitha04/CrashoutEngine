/*!
@file        prefab.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Implementation of Prefab Factory and JSON Serialization.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "engine.h"
#include "prefab.h"
#include "entitymanager.h"
#include "collisionmanager.h" 
#include "transformcomponent.h"
#include "shapescomponent.h"
#include "rigidbodycomponent.h"
#include "collidercomponent.h"
#include "namecomponent.h"
#include "textcomponent.h"
#include "camera.h"
#include "cameracomponent.h"
#include "scriptcomponent.h"
#include "ScriptSystem.h"
#include "audiocomponent.h"
#include "guicomponent.h"
#include "ppcomponent.h"
#include "tilemapcomponent.h"
#include "prefabcomponent.h"
#include "hierarchycomponent.h"
#include "spriteAnimation.h"
#include "keyframe.h"
#include "buttoncomponent.h"
#include "textinputcomponent.h"
#include "uilayoutcomponent.h"
#include "layercomponent.h"
#include "activecomponent.h"
#include "particlecomponent.h"
#include "rendering.h"
#include "tagcomponent.h"
#include <mono/metadata/attrdefs.h>

// RapidJSON
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <iostream>

#include "checkpointcomponent.h"
#include "monomanager.h"
#include "triggercomponent.h"

#include "sceneasset.h"
#include "hierarchy.h"

namespace {
	// Store the raw JSON array string representing the prefab's entity tree
	std::unordered_map<std::string, std::string>& Reg() {
		static std::unordered_map<std::string, std::string> R;
		return R;
	}
}

// --- Helper Functions ---
//static std::unique_ptr<Collision::AABBRect> MakeCenteredAABB3(const CEVec3& center, float width, float height) {
//	const float halfwidth = 0.5f * width, halfheight = 0.5f * height;
//	CEVec3 minCorner{ center.x - halfwidth, center.y - halfheight, center.z };
//	CEVec3 maxCorner{ center.x + halfwidth, center.y + halfheight, center.z };
//	return std::make_unique<Collision::AABBRect>(minCorner, maxCorner);
//}

// Utility for parsing Vectors
struct ReadVectorFromJson {
	CEVec3 operator()(const rapidjson::Value& val) {
		CEVec3 newVal{};
		if (val.HasMember("x")) newVal.x = val["x"].GetFloat();
		if (val.HasMember("y")) newVal.y = val["y"].GetFloat();
		if (val.HasMember("z")) newVal.z = val["z"].GetFloat();
		return newVal;
	}
};

// --- Core Prefab Logic ---
namespace Prefab {

	CrashoutsEngine::Entity Create(CrashoutsEngine::ECS& ecs, const std::string& name) {
		auto it = Reg().find(name);
		if (it == Reg().end()) return 0; // Prefab not found

		// 1. Parse the stored JSON array
		rapidjson::Document doc;
		doc.Parse(it->second.c_str());
		if (!doc.IsArray() || doc.Empty()) return 0;

		// 2. Maps to reconstruct hierarchy
		std::unordered_map<uint64_t, CrashoutsEngine::Entity> oldUuidToNewEntity;
		std::vector<CrashoutsEngine::Serialization::ParentLinkInfo> pendingLinks;

		CrashoutsEngine::Entity rootEntity = 0;

		// 3. First Pass: Create Entities
		for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
			const rapidjson::Value& entVal = doc[i];

			// Extract the original UUID from the prefab file
			uint64_t originalUUID = 0;
			if (entVal.HasMember("uuid")) {
				originalUUID = entVal["uuid"].GetUint64();
			}

			// Spawn the entity (This generates a BRAND NEW UUID automatically)
			// Note: Make sure DeserializeEntity takes a parameter to ignore the JSON UUID
			CrashoutsEngine::Entity newEnt = CrashoutsEngine::Serialization::DeserializeEntity(entVal, &pendingLinks, true);

			// Map the old prefab UUID to the actual Live Entity ID
			if (originalUUID != 0) {
				oldUuidToNewEntity[originalUUID] = newEnt;
			}

			// The first entity in the array is always the root
			if (i == 0) rootEntity = newEnt;

			// Mark it as a prefab instance
			ecs.AddComponent<Component::PrefabInstanceComponent>(newEnt, Component::PrefabInstanceComponent{ name });
		}

		// 4. Second Pass: Reconstruct Hierarchy
		for (const auto& link : pendingLinks) {
			auto parentIt = oldUuidToNewEntity.find(link.parentUUID);
			if (parentIt != oldUuidToNewEntity.end()) {
				CrashoutsEngine::Entity liveParent = parentIt->second;
				CrashoutsEngine::Entity liveChild = link.childEntity;

				// Use the standard AttachChild function to set up Parent/Child components
				CrashoutsEngine::AttachChild(ecs, liveParent, liveChild);
			}
		}

		return rootEntity;
	}

	//Get a list sort them by names
	std::vector<std::string> Names() {
		std::vector<std::string> v;
		v.reserve(Reg().size());
		for (auto& kv : Reg()) v.push_back(kv.first);
		std::sort(v.begin(), v.end());
		return v;
	}

	void ClearRegistry() {
		Reg().clear();
	}
}

// --- DATA STRUCTS FOR CAPTURE ---
namespace CaptureData {

	using namespace Component;

	struct TransformData { CEVec3 pos{ 0,0,0 }; float rot{ 0.f }; CEVec2 size{ 1,1 }; };
	struct TagData { unsigned int tagMask{ 0 }; }; // Was LayerData
	struct LayerData { bool hasComp{ false }; std::string layerName{ "Default" }; int z{ 0 }; }; // Changed default to "Default"
	struct RBData { float mass{ 0 }, rest{ 0 }, sFric{ 0 }, dFric{ 0 }, inert{ 0 }, invInert{ 0 }; bool isTrigger{ false }; bool hasComp{ false }; };
	struct ColliderData {
		bool hasComp{ false }; Collision::ColliderType type{ Collision::ColliderType::AABB };
		bool isTrigger{ false }; bool enabled{ true };
		CEVec3 pos{ 0,0,0 }; CEVec3 offset{ 0,0,0 };
		float radius{ 1.f }; CEVec3 min{ 0,0,0 }; CEVec3 max{ 0,0,0 }; float rotation{ 0.f };
		std::vector<CEVec3> verts; // For Polygon
	};
	struct ShapeData { bool hasComp{ false }; ShapeComponent c; };
	struct TextData { bool hasComp{ false }; TextComponent c; };
	struct AudioData { bool hasComp{ false }; AudioEmitterComponent c; };
	struct ScriptData { bool hasComp{ false }; std::string scriptsJson; };
	struct ParticleData { bool hasComp{ false }; ParticleComponent c; };
	struct CameraData { bool hasComp{ false }; CameraComponent c; };
	struct SpriteData { bool hasComp{ false }; SpriteAnimatorComponent c; };
	struct KeyframeData { bool hasComp{ false }; KeyframeAnimatorComponent c; };
	struct TileMapData { bool hasComp{ false }; TileMapComponent c; };
	struct ParentData { bool hasComp{ false }; ParentComponent c; };
	struct GuiData { bool hasComp{ false }; GuiComponent c; };
	struct ButtonData { bool hasComp{ false }; ButtonComponent c; };
	struct InputData { bool hasComp{ false }; TextInputComponent c; };
	struct LayoutData { bool hasComp{ false }; UILayoutComponent c; };
	struct PPData { bool hasComp{ false }; PPComponent c; };
	struct ActiveData { bool hasComp{ false }; ActiveComponent c; };
	struct CheckpointData { bool hasComp{ false }; CheckpointComponent c; };
	struct TriggerData { bool hasComp{ false }; TriggerComponent c; };
}

// Helper to read color from JSON object {x,y,z,w}
auto ReadColor = [](const rapidjson::Value& val) -> CEColor {
	CEColor c{ 0.f, 0.f, 0.f, 1.f };
	// Handle specific X/Y/Z/W format from your snippet
	if (val.HasMember("x")) c.x() = val["x"].GetFloat();
	if (val.HasMember("y")) c.y() = val["y"].GetFloat();
	if (val.HasMember("z")) c.z() = val["z"].GetFloat();
	if (val.HasMember("w")) c.w() = val["w"].GetFloat();
	// Fallback for r/g/b/a if mixed formats exist
	if (val.HasMember("r")) c.x() = val["r"].GetFloat();
	if (val.HasMember("g")) c.y() = val["g"].GetFloat();
	if (val.HasMember("b")) c.z() = val["b"].GetFloat();
	if (val.HasMember("a")) c.w() = val["a"].GetFloat();
	return c;
	};

// --- JSON Loading Logic ---
static bool LoadPrefabsFromJson_RJ(const std::string& path) {
	std::ifstream inputFile(path);
	if (!inputFile) return false;

	rapidjson::IStreamWrapper isw(inputFile);
	rapidjson::Document doc;
	doc.ParseStream(isw);

	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("prefabs")) return false;

	const auto& arr = doc["prefabs"].GetArray();
	for (auto& prefabData : arr) {
		if (!prefabData.IsObject() || !prefabData.HasMember("name")) continue;

		std::string name = prefabData["name"].GetString();

		if (prefabData.HasMember("entities") && prefabData["entities"].IsArray()) {
			// Convert the entities array back to a string and store it in the registry
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			prefabData["entities"].Accept(writer);

			Reg()[name] = buffer.GetString();
		}
	}
	return true;
}

namespace CrashoutsEngine {
	using namespace ::Component;

	void InitPrefabs() {
		namespace fs = std::filesystem;

#if USE_IMGUI_EDITOR
		const fs::path dir = fs::path(CrashoutsEngine::Assets::rootPath) / "prefabs";
#else
		const fs::path dir = fs::path(GetLocalAppData()) / "prefabs";
#endif
		if (fs::exists(dir) && fs::is_directory(dir)) {
			for (auto& entry : fs::directory_iterator(dir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					LoadPrefabsFromJson_RJ(entry.path().generic_string());
				}
			}
		}
	}

	void ReloadPrefabsFromDisk(const std::string& path) {
		// 1. Wipe everything
		Prefab::ClearRegistry();

		// 2. Load from the SPECIFIC path provided
		namespace fs = std::filesystem;
		fs::path dirPath(path);

		// Check if it exists
		if (fs::exists(dirPath)) {
			// Case A: It's a directory (Load all JSONs inside)
			if (fs::is_directory(dirPath)) {
				for (const auto& entry : fs::directory_iterator(dirPath)) {
					if (entry.is_regular_file() && entry.path().extension() == ".json") {
						LoadPrefabsFromJson_RJ(entry.path().generic_string());
					}
				}
			}
			// Case B: It's a single file (Load just that file)
			else if (fs::is_regular_file(dirPath) && dirPath.extension() == ".json") {
				LoadPrefabsFromJson_RJ(dirPath.generic_string());
			}
		}
		else {
			std::cerr << "[Prefab System] Warning: Reload path does not exist: " << path << std::endl;
		}
	}

	void RecreateInstancesForPrefab(ECS& ecs, const std::string& prefabName) {
		auto& arr = ecs.GetComponentArray<PrefabInstanceComponent>();
		std::vector<Entity> toRecreate;

		// 1. Find entities to update
		for (size_t i = 0; i < arr.dense.size(); ++i) {
			if (arr.dense[i]->prefabName == prefabName) {
				toRecreate.push_back(arr.indexToEntity[i]);
			}
		}

		// 2. Recreate them
		for (Entity e : toRecreate) {
			// --- BACKUP STATE ---
			TransformComponent savedTransform;
			bool hasTransform = false;
			if (auto* tc = ecs.GetComponent<TransformComponent>(e)) {
				savedTransform = *tc;
				hasTransform = true;
			}

			// Backup Shape Color
			CEColor savedShapeColor;
			bool hasShape = false;
			if (auto* sc = ecs.GetComponent<ShapeComponent>(e)) {
				savedShapeColor = sc->color;
				hasShape = true;
			}

			// Backup Text Color (if applicable)
			CEColor savedTextColor;
			bool hasText = false;
			if (auto* txt = ecs.GetComponent<TextComponent>(e)) {
				savedTextColor = txt->color;
				hasText = true;
			}

			// --- DESTROY OLD ---
			ecs.DestroyEntity(e);

			// --- CREATE NEW (And capture ID) ---
			// This now works because we updated Create() to return the ID
			Entity newEntity = Prefab::Create(ecs, prefabName);

			// --- RESTORE STATE ---
			if (newEntity != 0) {
				// Restore Transform
				if (hasTransform) {
					if (auto* newTc = ecs.GetComponent<TransformComponent>(newEntity)) {
						*newTc = savedTransform;
					}
				}

				// Restore Shape Color
				if (hasShape) {
					if (auto* newSc = ecs.GetComponent<ShapeComponent>(newEntity)) {
						newSc->color = savedShapeColor;
					}
				}

				// Restore Text Color
				if (hasText) {
					if (auto* newTxt = ecs.GetComponent<TextComponent>(newEntity)) {
						newTxt->color = savedTextColor;
					}
				}
			}
		}
	}

	// --- SAVE LOGIC (Mirror of Load) ---
	bool SavePrefabToJson(ECS& ecs, Entity rootEntity, const std::string& newName, const std::string& path, const std::string& matchName) {
		using namespace rapidjson;
		using namespace Component;

		Document doc;
		{
			std::ifstream in(path, std::ios::binary);
			if (in) {
				IStreamWrapper isw(in);
				doc.ParseStream(isw);
			}
			else {
				doc.SetObject();
			}
		}
		auto& allocator = doc.GetAllocator();

		if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
			if (doc.HasMember("prefabs")) doc.RemoveMember("prefabs");
			doc.AddMember("prefabs", Value(kArrayType), allocator);
		}
		Value& prefabs = doc["prefabs"];

		// 1. Gather the whole tree
		std::vector<Entity> prefabTree;
		GatherDescendants(ecs, rootEntity, prefabTree);

		// 2. Create the Prefab Object
		Value prefabObj(kObjectType);
		prefabObj.AddMember("name", Value(newName.c_str(), allocator), allocator);

		// 3. Serialize all entities into an array
		Value entitiesArray(kArrayType);
		for (Entity e : prefabTree) {
			// Re-use SerializeEntity function!
			Value entVal = Serialization::SerializeEntity(e, allocator);
			entitiesArray.PushBack(entVal, allocator);
		}

		prefabObj.AddMember("entities", entitiesArray, allocator);

		// 4. Upsert
		bool replaced = false;
		std::string searchKey = matchName.empty() ? newName : matchName;
		for (auto& p : prefabs.GetArray()) {
			if (p.HasMember("name") && p["name"].IsString() && std::string(p["name"].GetString()) == searchKey) {
				p.CopyFrom(prefabObj, allocator);
				replaced = true;
				break;
			}
		}
		if (!replaced) prefabs.PushBack(prefabObj, allocator);

		StringBuffer sb;
		PrettyWriter<StringBuffer> writer(sb);
		doc.Accept(writer);

		std::ofstream out(path);
		out << sb.GetString();
		return true;
	}
}