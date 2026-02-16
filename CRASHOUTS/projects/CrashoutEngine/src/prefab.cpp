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
#include "particleComponent.h"
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

// --- Static Registry ---
namespace {
	std::unordered_map<std::string, Prefab::Factory>& Reg() {
		static std::unordered_map<std::string, Prefab::Factory> R;
		return R;
	}
	Prefab::Fallback& FallbackFn() {
		static Prefab::Fallback FB;
		return FB;
	}
}

// --- Helper Functions ---
static std::unique_ptr<Collision::AABBRect> MakeCenteredAABB3(const CEVec3& center, float width, float height) {
	const float halfwidth = 0.5f * width, halfheight = 0.5f * height;
	CEVec3 minCorner{ center.x - halfwidth, center.y - halfheight, center.z };
	CEVec3 maxCorner{ center.x + halfwidth, center.y + halfheight, center.z };
	return std::make_unique<Collision::AABBRect>(minCorner, maxCorner);
}

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

	bool Register(const std::string& name, Factory f) {
		Reg()[name] = std::move(f);
		return true;
	}

	//Remove prefab
	bool Unregister(const std::string& name) { return Reg().erase(name) > 0; }
	bool Exists(const std::string& name) { return Reg().find(name) != Reg().end(); }

	//Get a list sort them by names
	std::vector<std::string> Names() {
		std::vector<std::string> v;
		v.reserve(Reg().size());
		for (auto& kv : Reg()) v.push_back(kv.first);
		std::sort(v.begin(), v.end());
		return v;
	}

	//Make gameobj
	CrashoutsEngine::Entity Create(CrashoutsEngine::ECS& ecs, const std::string& name) {
		auto it = Reg().find(name);
		if (it != Reg().end()) {
			return it->second(ecs); // Return the Entity ID from the factory
		}

		auto& fb = FallbackFn();
		if (fb) {
			return fb(ecs, name); // Return the Entity ID from the fallback
		}

		return 0; // Return 0 if failed
	}

	void SetFallback(Fallback fb) { FallbackFn() = std::move(fb); }

	void ClearRegistry() {
		Reg().clear();
		std::unordered_map<std::string, Factory>().swap(Reg());
		FallbackFn() = nullptr;
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
	using namespace CaptureData;

	std::ifstream inputFile(path);
	if (!inputFile) return false;

	rapidjson::IStreamWrapper isw(inputFile);
	rapidjson::Document doc;
	doc.ParseStream(isw);

	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("prefabs")) return false;

	const auto& arr = doc["prefabs"].GetArray();
	for (auto& prefabs : arr) {
		if (!prefabs.IsObject()) continue;
		if (!prefabs.HasMember("name") || !prefabs["name"].IsString()) continue;

		const std::string name = prefabs["name"].GetString();

		// -------------------------------------------------------------------------
		// 1. EXTRACT DATA TO LOCAL STRUCTS
		// -------------------------------------------------------------------------

		// Active
		ActiveData actD;
		if (prefabs.HasMember("active")) {
			actD.hasComp = true;
			const auto& a = prefabs["active"];
			if (a.HasMember("isActive")) actD.c.isActive = a["isActive"].GetBool();
		}
		else {
			// If the prefab JSON doesn't mention "active",
			// we assume it should exist and be enabled (Default behavior).
			actD.hasComp = true;
			actD.c.isActive = true;
		}

		// Transform
		TransformData trD;
		if (prefabs.HasMember("transform")) {
			const auto& t = prefabs["transform"];
			if (t.HasMember("position")) trD.pos = ReadVectorFromJson{}(t["position"]);
			if (t.HasMember("rotation")) trD.rot = t["rotation"].GetFloat();
			if (t.HasMember("size")) { trD.size.x = t["size"]["x"].GetFloat(); trD.size.y = t["size"]["y"].GetFloat(); }
		}

		// Tag (formerly Layer)
		TagData tgD;
		if (prefabs.HasMember("tag")) {
			const auto& l = prefabs["tag"];
			if (l.IsObject() && l.HasMember("tagMask")) tgD.tagMask = l["tagMask"].GetUint();
		}
		else if (prefabs.HasMember("layer")) {
			// Legacy check: if it has "currentLayer", it's a Tag
			const auto& l = prefabs["layer"];
			if (l.IsObject() && l.HasMember("currentLayer")) tgD.tagMask = l["currentLayer"].GetUint();
		}

		// Layer (formerly ZOrder)
		LayerData ldD;
		if (prefabs.HasMember("layer")) {
			// Check if it's the NEW Layer (ZOrder replacement)
			const auto& l = prefabs["layer"];
			if (l.IsObject() && l.HasMember("layerName")) {
				ldD.hasComp = true;
				ldD.layerName = l["layerName"].GetString();
				if (l.HasMember("z")) ldD.z = l["z"].GetInt();
			}
		}

		// Legacy ZOrder check
		if (!ldD.hasComp && prefabs.HasMember("zorder")) {
			ldD.hasComp = true;
			const auto& z = prefabs["zorder"];
			int oldEnum = 2;
			if (z.HasMember("layer")) oldEnum = z["layer"].GetInt();
			if (z.HasMember("z")) ldD.z = z["z"].GetInt();

			switch (oldEnum) {
			case 0: ldD.layerName = "StaticBackground"; break;
			case 1: ldD.layerName = "InteractiveBG"; break;
			case 2: ldD.layerName = "WorldGeometry"; break;
			case 3: ldD.layerName = "EffectsAndItems"; break;
			case 4: ldD.layerName = "Players"; break;
			default: ldD.layerName = "Default"; break;
			}
		}

		// RigidBody
		RBData rbD;
		if (prefabs.HasMember("rigidbody")) {
			rbD.hasComp = true;
			const auto& r = prefabs["rigidbody"];
			if (r.HasMember("mass")) rbD.mass = r["mass"].GetFloat();
			if (r.HasMember("restitution")) rbD.rest = r["restitution"].GetFloat();
			if (r.HasMember("restitutionCoeff")) rbD.rest = r["restitutionCoeff"].GetFloat();
			if (r.HasMember("staticFriction")) rbD.sFric = r["staticFriction"].GetFloat();
			if (r.HasMember("dynamicFriction")) rbD.dFric = r["dynamicFriction"].GetFloat();
			if (r.HasMember("inertia")) rbD.inert = r["inertia"].GetFloat();
			if (r.HasMember("invInertia")) rbD.invInert = r["invInertia"].GetFloat();
			if (r.HasMember("isTrigger")) rbD.isTrigger = r["isTrigger"].GetBool();
		}

		// Collider
		ColliderData colD;
		if (prefabs.HasMember("collider")) {
			colD.hasComp = true;
			const auto& c = prefabs["collider"];
			if (c.HasMember("ColliderType")) colD.type = static_cast<Collision::ColliderType>(c["ColliderType"].GetInt());
			if (c.HasMember("isTrigger")) colD.isTrigger = c["isTrigger"].GetBool();
			if (c.HasMember("enabled")) colD.enabled = c["enabled"].GetBool();
			if (c.HasMember("position")) colD.pos = ReadVectorFromJson{}(c["position"]);
			if (c.HasMember("offset")) colD.offset = ReadVectorFromJson{}(c["offset"]);

			// Type Specifics
			if (c.HasMember("radius")) colD.radius = c["radius"].GetFloat();
			if (c.HasMember("min")) colD.min = ReadVectorFromJson{}(c["min"]);
			if (c.HasMember("max")) colD.max = ReadVectorFromJson{}(c["max"]);
			if (c.HasMember("rotation")) colD.rotation = c["rotation"].GetFloat();
			if (c.HasMember("vertices") && c["vertices"].IsArray()) {
				for (auto& v : c["vertices"].GetArray()) colD.verts.push_back(ReadVectorFromJson{}(v));
			}
		}
		// Fallback for Box type without collider block (old prefab support)
		else if (std::string(prefabs["type"].GetString()) == "Box") {
			colD.hasComp = true;
			colD.type = Collision::ColliderType::AABB;
			// Auto-calc done in lambda
		}

		// Shape
		ShapeData shD;
		if (prefabs.HasMember("shape")) {
			shD.hasComp = true;
			const auto& s = prefabs["shape"];
			if (s.HasMember("type")) shD.c.type = static_cast<ShapeType>(s["type"].GetInt());
			if (s.HasMember("color") && s["color"].IsObject()) {
				const auto& cObj = s["color"];

				if (cObj.HasMember("r") && cObj["r"].IsNumber()) shD.c.color.r = cObj["r"].GetFloat();
				if (cObj.HasMember("g") && cObj["g"].IsNumber()) shD.c.color.g = cObj["g"].GetFloat();
				if (cObj.HasMember("b") && cObj["b"].IsNumber()) shD.c.color.b = cObj["b"].GetFloat();
				if (cObj.HasMember("a") && cObj["a"].IsNumber()) shD.c.color.a = cObj["a"].GetFloat();
			}
			if (s.HasMember("enableTexture")) shD.c.enableTexture = s["enableTexture"].GetBool();
			if (s.HasMember("textureName")) shD.c.textureName = s["textureName"].GetString();
			if (s.HasMember("uvSize")) { shD.c.uvSize.x = s["uvSize"]["x"].GetFloat(); shD.c.uvSize.y = s["uvSize"]["y"].GetFloat(); }
			if (s.HasMember("uvOffset")) { shD.c.uvOffset.x = s["uvOffset"]["x"].GetFloat(); shD.c.uvOffset.y = s["uvOffset"]["y"].GetFloat(); }
			if (s.HasMember("thickness")) shD.c.thickness = s["thickness"].GetFloat();
			if (s.HasMember("wrapModeU")) shD.c.wrapModeU = s["wrapModeU"].GetInt();
			if (s.HasMember("wrapModeV")) shD.c.wrapModeV = s["wrapModeV"].GetInt();
			if (s.HasMember("preview")) shD.c.preview = s["preview"].GetBool();
			if (s.HasMember("isWorldSpace")) shD.c.isWorldSpace = s["isWorldSpace"].GetBool();
		}

		// Text
		TextData txD;
		if (prefabs.HasMember("text")) {
			txD.hasComp = true;
			const auto& t = prefabs["text"];
			if (t.HasMember("font")) txD.c.font = t["font"].GetString();
			if (t.HasMember("text")) txD.c.text = t["text"].GetString();
			if (t.HasMember("scale")) txD.c.scale = t["scale"].GetFloat();
			if (t.HasMember("centered")) txD.c.centered = t["centered"].GetBool();
			if (t.HasMember("isworldspace")) txD.c.isWorldSpace = t["isworldspace"].GetBool();
			if (t.HasMember("offset")) { txD.c.offset.x = t["offset"]["x"].GetFloat(); txD.c.offset.y = t["offset"]["y"].GetFloat(); }
			if (t.HasMember("color") && t["color"].IsObject()) {
				const auto& cObj = t["color"];

				if (cObj.HasMember("r") && cObj["r"].IsNumber()) txD.c.color.r = cObj["r"].GetFloat();
				if (cObj.HasMember("g") && cObj["g"].IsNumber()) txD.c.color.g = cObj["g"].GetFloat();
				if (cObj.HasMember("b") && cObj["b"].IsNumber()) txD.c.color.b = cObj["b"].GetFloat();
				if (cObj.HasMember("a") && cObj["a"].IsNumber()) txD.c.color.a = cObj["a"].GetFloat();
			}
		}

		// Script
		ScriptData scD;
		if (prefabs.HasMember("script")) {
			scD.hasComp = true;
			const auto& s = prefabs["script"];
			if (s.HasMember("scripts") && s["scripts"].IsArray()) {
				rapidjson::StringBuffer buffer;
				rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
				s["scripts"].Accept(writer);
				scD.scriptsJson = buffer.GetString();
			}
		}
		// Particle
		ParticleData ptD;
		if (prefabs.HasMember("particle")) {
			ptD.hasComp = true;
			const auto& p = prefabs["particle"];
			auto& pc = ptD.c;

			if (p.HasMember("playOnStop"))   pc.playOnStop = p["playOnStop"].GetBool();
			if (p.HasMember("isEmitting"))   pc.isEmitting = p["isEmitting"].GetBool();
			if (p.HasMember("useWorldSpace")) pc.useWorldSpace = p["useWorldSpace"].GetBool();

			if (p.HasMember("emissionShape") && p["emissionShape"].IsString()) {
				std::string s = p["emissionShape"].GetString();
				if (s == "LINE")   pc.emissionShape = ParticleSystem::ParticleShape::LINE;
				if (s == "CIRCLE") pc.emissionShape = ParticleSystem::ParticleShape::CIRCLE;
				if (s == "SQUARE") pc.emissionShape = ParticleSystem::ParticleShape::SQUARE;
			}

			if (p.HasMember("lengthMin")) pc.lengthMin = p["lengthMin"].GetFloat();
			if (p.HasMember("lengthMax")) pc.lengthMax = p["lengthMax"].GetFloat();
			if (p.HasMember("radiusMin")) pc.radiusMin = p["radiusMin"].GetFloat();
			if (p.HasMember("radiusMax")) pc.radiusMax = p["radiusMax"].GetFloat();

			auto readVec2 = [&](const rapidjson::Value& v, CEVec2& out) {
				if (v.HasMember("x")) out.x = v["x"].GetFloat();
				if (v.HasMember("y")) out.y = v["y"].GetFloat();
				};

			if (p.HasMember("areaMin") && p["areaMin"].IsObject()) readVec2(p["areaMin"], pc.areaMin);
			if (p.HasMember("areaMax") && p["areaMax"].IsObject()) readVec2(p["areaMax"], pc.areaMax);

			if (p.HasMember("maxParticles")) pc.maxParticles = p["maxParticles"].GetInt();
			if (p.HasMember("emitRate"))     pc.emitRate = p["emitRate"].GetFloat();

			if (p.HasMember("startLifeMin")) pc.startLifeMin = p["startLifeMin"].GetFloat();
			if (p.HasMember("startLifeMax")) pc.startLifeMax = p["startLifeMax"].GetFloat();
			if (p.HasMember("startRotationMin")) pc.startRotationMin = p["startRotationMin"].GetFloat();
			if (p.HasMember("startRotationMax")) pc.startRotationMax = p["startRotationMax"].GetFloat();
			if (p.HasMember("startSpeedMin")) pc.startSpeedMin = p["startSpeedMin"].GetFloat();
			if (p.HasMember("startSpeedMax")) pc.startSpeedMax = p["startSpeedMax"].GetFloat();

			if (p.HasMember("uniformSize")) pc.uniformSize = p["uniformSize"].GetBool();
			if (p.HasMember("startSizeXMin")) pc.startSizeXMin = p["startSizeXMin"].GetFloat();
			if (p.HasMember("startSizeXMax")) pc.startSizeXMax = p["startSizeXMax"].GetFloat();
			if (p.HasMember("startSizeYMin")) pc.startSizeYMin = p["startSizeYMin"].GetFloat();
			if (p.HasMember("startSizeYMax")) pc.startSizeYMax = p["startSizeYMax"].GetFloat();

			if (p.HasMember("startColor") && p["startColor"].IsObject()) pc.startcolor = ReadColor(p["startColor"]);
			if (p.HasMember("endColor") && p["endColor"].IsObject())   pc.endColor = ReadColor(p["endColor"]);

			if (p.HasMember("startVelocity")) pc.startVelocity = ReadVectorFromJson{}(p["startVelocity"]);
			if (p.HasMember("endVelocity"))   pc.endVelocity = ReadVectorFromJson{}(p["endVelocity"]);
			if (p.HasMember("gravity"))       pc.gravity = ReadVectorFromJson{}(p["gravity"]);

			if (p.HasMember("rotationOverTime")) pc.rotationOverTime = p["rotationOverTime"].GetFloat();
			if (p.HasMember("sizeOverTime"))     pc.sizeOverTime = p["sizeOverTime"].GetFloat();

			if (p.HasMember("useTexture")) pc.useTexture = p["useTexture"].GetBool();
			if (p.HasMember("textureName")) pc.textureName = p["textureName"].GetString();
			if (p.HasMember("textureUV") && p["textureUV"].IsObject()) readVec2(p["textureUV"], pc.textureUV);
		}

		// Audio
		AudioData auD;
		if (prefabs.HasMember("audio")) {
			auD.hasComp = true;
			const auto& a = prefabs["audio"];

			// NEW FORMAT
			if (a.HasMember("clips") && a["clips"].IsArray()) {
				const auto& clipsArr = a["clips"];
				for (rapidjson::SizeType i = 0; i < clipsArr.Size(); ++i) {
					const auto& cVal = clipsArr[i];
					Component::AudioClip clip;

					if (cVal.HasMember("name")) clip.name = cVal["name"].GetString();
					if (cVal.HasMember("path")) clip.path = cVal["path"].GetString();
					if (cVal.HasMember("volume")) clip.volume = cVal["volume"].GetFloat();
					if (cVal.HasMember("pitch")) clip.pitch = cVal["pitch"].GetFloat();
					if (cVal.HasMember("looping")) clip.looping = cVal["looping"].GetBool();
					if (cVal.HasMember("isMusic")) clip.isMusic = cVal["isMusic"].GetBool();
					if (cVal.HasMember("spatial3D")) clip.spatial3D = cVal["spatial3D"].GetBool();

					auD.c.clips.push_back(clip);
				}
			}
			// LEGACY FORMAT
			else {
				Component::AudioClip clip;
				clip.name = "Main Clip"; // Default name for legacy data

				if (a.HasMember("eventPath")) clip.path = a["eventPath"].GetString();
				if (a.HasMember("volume")) clip.volume = a["volume"].GetFloat();
				if (a.HasMember("pitch")) clip.pitch = a["pitch"].GetFloat();
				if (a.HasMember("looping")) clip.looping = a["looping"].GetBool();
				if (a.HasMember("spatial3D")) clip.spatial3D = a["spatial3D"].GetBool();

				// Only add if there is actually data
				if (!clip.path.empty()) {
					auD.c.clips.push_back(clip);
				}
			}
		}

		// Camera
		CameraData camD;
		if (prefabs.HasMember("camera")) {
			camD.hasComp = true;
			const auto& c = prefabs["camera"];
			if (c.HasMember("zoom")) camD.c.zoom = c["zoom"].GetFloat();
			if (c.HasMember("position")) camD.c.position = ReadVectorFromJson{}(c["position"]);
			if (c.HasMember("xBounds") && c["xBounds"].IsArray() && c["xBounds"].Size() >= 2) {
				camD.c.xBounds.x = c["xBounds"][0].GetFloat();
				camD.c.xBounds.y = c["xBounds"][1].GetFloat();
			}
			if (c.HasMember("yBounds") && c["yBounds"].IsArray() && c["yBounds"].Size() >= 2) {
				camD.c.yBounds.x = c["yBounds"][0].GetFloat();
				camD.c.yBounds.y = c["yBounds"][1].GetFloat();
			}
		}

		// Sprite Animator
		SpriteData spD;
		if (prefabs.HasMember("sprite_animator")) {
			spD.hasComp = true;
			const auto& sa = prefabs["sprite_animator"];
			if (sa.HasMember("currentClip")) spD.c.currentClipName = sa["currentClip"].GetString();
			if (sa.HasMember("playing")) spD.c.playing = sa["playing"].GetBool();
			if (sa.HasMember("flipH")) spD.c.flipH = sa["flipH"].GetBool();
			if (sa.HasMember("clips") && sa["clips"].IsArray()) {
				for (auto& c : sa["clips"].GetArray()) {
					std::string n = c["name"].GetString();
					int start = c["start"].GetInt();
					int cnt = c["count"].GetInt();
					float fps = c["fps"].GetFloat();
					bool loop = c["loop"].GetBool();
					spD.c.AddClip(n, start, cnt, fps, loop);
				}
			}
		}

		// Keyframe Animator (Simplified load logic, full logic matches sceneasset)
		KeyframeData kfD;
		if (prefabs.HasMember("keyframe_animator")) {
			kfD.hasComp = true;
			const auto& kaVal = prefabs["keyframe_animator"];
			if (kaVal.HasMember("current")) kfD.c.current = kaVal["current"].GetString();
			if (kaVal.HasMember("playing")) kfD.c.playing = kaVal["playing"].GetBool();

			if (kaVal.HasMember("clips") && kaVal["clips"].IsArray()) {
				for (const auto& clipVal : kaVal["clips"].GetArray()) {
					AnimationClip newClip;
					if (clipVal.HasMember("name")) newClip.name = clipVal["name"].GetString();
					if (clipVal.HasMember("wrap")) newClip.wrap = static_cast<KFWrap>(clipVal["wrap"].GetInt());

					// --- [NEW] Load Texture & Size Data ---
					if (clipVal.HasMember("textureAsset") && clipVal["textureAsset"].IsString())
						newClip.textureAsset = clipVal["textureAsset"].GetString();

					if (clipVal.HasMember("spriteCols") && clipVal["spriteCols"].IsInt())
						newClip.spriteCols = clipVal["spriteCols"].GetInt();

					if (clipVal.HasMember("spriteRows") && clipVal["spriteRows"].IsInt())
						newClip.spriteRows = clipVal["spriteRows"].GetInt();

					if (clipVal.HasMember("useCustomSize") && clipVal["useCustomSize"].IsBool())
						newClip.useCustomSize = clipVal["useCustomSize"].GetBool();

					if (clipVal.HasMember("targetWidth") && clipVal["targetWidth"].IsFloat())
						newClip.targetWidth = clipVal["targetWidth"].GetFloat();

					if (clipVal.HasMember("targetHeight") && clipVal["targetHeight"].IsFloat())
						newClip.targetHeight = clipVal["targetHeight"].GetFloat();
					// --------------------------------------

					if (clipVal.HasMember("bindings")) {
						for (const auto& bVal : clipVal["bindings"].GetArray()) {
							KFBinding newBinding;
							newBinding.property = static_cast<KFProperty>(bVal["property"].GetInt());

							if (bVal.HasMember("keys") && bVal["keys"].IsArray()) {
								for (const auto& kVal : bVal["keys"].GetArray()) {
									KFKey k;
									if (kVal.HasMember("t")) k.time = kVal["t"].GetFloat();
									if (kVal.HasMember("v")) k.value = kVal["v"].GetFloat();
									if (kVal.HasMember("i")) k.interp = static_cast<KFInterp>(kVal["i"].GetInt());
									if (kVal.HasMember("in")) k.inTan = kVal["in"].GetFloat();
									if (kVal.HasMember("out")) k.outTan = kVal["out"].GetFloat();
									newBinding.curve.Add(k); // Using Add ensures sorting!
								}
							}
							newClip.bindings.push_back(newBinding);
						}
					}
					kfD.c.clips[newClip.name] = newClip;
				}
			}
		}

		// Tilemap
		TileMapData tmD;
		if (prefabs.HasMember("tilemap")) {
			tmD.hasComp = true;
			const auto& t = prefabs["tilemap"];
			if (t.HasMember("tileSize")) tmD.c.tileSize = t["tileSize"].GetInt();
			if (t.HasMember("activeAsset")) tmD.c.activeAsset = t["activeAsset"].GetString();
			if (t.HasMember("tiles") && t["tiles"].IsArray()) {
				for (auto& tile : t["tiles"].GetArray()) {
					tmD.c.gridData[{tile["x"].GetInt(), tile["y"].GetInt()}] = tile["asset"].GetString();
				}
			}
			if (t.HasMember("tileAssetNames") && t["tileAssetNames"].IsArray()) {
				for (auto& n : t["tileAssetNames"].GetArray()) tmD.c.tileAssetNames.push_back(n.GetString());
			}
			tmD.c.isCacheDirty = true;
		}

		// Parent
		ParentData parD;
		if (prefabs.HasMember("parent")) {
			parD.hasComp = true;
			const auto& p = prefabs["parent"];
			if (p.HasMember("localPos")) parD.c.localPos = ReadVectorFromJson{}(p["localPos"]);
			if (p.HasMember("localRot")) parD.c.localRot = p["localRot"].GetFloat();
			if (p.HasMember("localScale")) { parD.c.localScale.x = p["localScale"]["x"].GetFloat(); parD.c.localScale.y = p["localScale"]["y"].GetFloat(); }
			if (p.HasMember("inheritColor")) parD.c.inheritColor = p["inheritColor"].GetBool();
			if (p.HasMember("colorMul")) { auto& m = p["colorMul"]; parD.c.colorMul = { m["x"].GetFloat(), m["y"].GetFloat(), m["z"].GetFloat(), m["w"].GetFloat() }; }
		}

		// GUI / Button / Input / Layout
		GuiData guiD;
		if (prefabs.HasMember("gui")) {
			guiD.hasComp = true;
			if (prefabs["gui"].HasMember("uiType")) guiD.c.uiType = static_cast<UIType>(prefabs["gui"]["uiType"].GetUint());
		}

		ButtonData btnD;
		if (prefabs.HasMember("button")) {
			btnD.hasComp = true;
			const auto& b = prefabs["button"];
			// 1. State
			if (b.HasMember("buttonState"))
				btnD.c.buttonState = static_cast<ButtonState>(b["buttonState"].GetUint());

			// 2. Shape Normal
			if (b.HasMember("shapeNormalColor"))
				btnD.c.shapeNormalColor = ReadColor(b["shapeNormalColor"]);

			// 3. Shape Hover
			if (b.HasMember("shapeHoveredColor"))
				btnD.c.shapeHoveredColor = ReadColor(b["shapeHoveredColor"]);

			// 4. Shape Pressed
			if (b.HasMember("shapePressedColor"))
				btnD.c.shapePressedColor = ReadColor(b["shapePressedColor"]);

			// 5. Text Normal
			if (b.HasMember("textNormalColor"))
				btnD.c.textNormalColor = ReadColor(b["textNormalColor"]);

			// 6. Text Hover
			if (b.HasMember("textHoveredColor"))
				btnD.c.textHoveredColor = ReadColor(b["textHoveredColor"]);

			// 7. Text Pressed
			if (b.HasMember("textPressedColor"))
				btnD.c.textPressedColor = ReadColor(b["textPressedColor"]);

			if (b.HasMember("textureNormal") && b["textureNormal"].IsString())
				btnD.c.textureNormal = b["textureNormal"].GetString();
			if (b.HasMember("textureHovered") && b["textureHovered"].IsString())
				btnD.c.textureHovered = b["textureHovered"].GetString();
			if (b.HasMember("texturePressed") && b["texturePressed"].IsString())
				btnD.c.texturePressed = b["texturePressed"].GetString();
		}

		InputData inpD;
		if (prefabs.HasMember("textinput")) {
			inpD.hasComp = true;
			const auto& t = prefabs["textinput"];
			if (t.HasMember("isFocused")) inpD.c.isFocused = t["isFocused"].GetBool();
			if (t.HasMember("maxCharacters")) inpD.c.maxCharacters = t["maxCharacters"].GetUint();
		}

		LayoutData layD;
		if (prefabs.HasMember("uilayout")) {
			layD.hasComp = true;
			const auto& u = prefabs["uilayout"];
			if (u.HasMember("uiLayout")) layD.c.uiLayout = static_cast<UILayout>(u["uiLayout"].GetUint());
			if (u.HasMember("paddingXY")) { layD.c.paddingXY.x = u["paddingXY"]["x"].GetFloat(); layD.c.paddingXY.y = u["paddingXY"]["y"].GetFloat(); }
		}

		PPData ppD;
		if (prefabs.HasMember("postprocessing")) {
			ppD.hasComp = true;
			const auto& p = prefabs["postprocessing"];
			if (p.HasMember("active")) ppD.c.active = p["active"].GetBool();
			if (p.HasMember("brightness")) ppD.c.brightness = p["brightness"].GetFloat();
			if (p.HasMember("contrast")) ppD.c.contrast = p["contrast"].GetFloat();
			if (p.HasMember("saturation")) ppD.c.saturation = p["saturation"].GetFloat();
		}

		CheckpointData cpD;
		if (prefabs.HasMember("checkpoint")) {
			cpD.hasComp = true;
			const auto& c = prefabs["checkpoint"];
			if (c.HasMember("respawnPosition")) cpD.c.respawnPosition = ReadVectorFromJson{}(c["respawnPosition"]);
			if (c.HasMember("targetRespawnEntity")) cpD.c.targetRespawnEntity = c["targetRespawnEntity"].GetUint();
			if (c.HasMember("targetEntityName")) cpD.c.targetEntityName = c["targetEntityName"].GetString();
			if (c.HasMember("activated")) cpD.c.activated = c["activated"].GetBool();
		}

		TriggerData trgD;
		if (prefabs.HasMember("trigger_metadata")) {
			trgD.hasComp = true;
			const auto& c = prefabs["trigger_metadata"];
			if (c.HasMember("triggerID")) trgD.c.triggerID = c["triggerID"].GetString();
			if (c.HasMember("oneTimeUse")) trgD.c.oneTimeUse = c["oneTimeUse"].GetBool();
			if (c.HasMember("triggered")) trgD.c.triggered = c["triggered"].GetBool();
		}


		// -------------------------------------------------------------------------
		// 2. REGISTER LAMBDA FACTORY
		// -------------------------------------------------------------------------
		Prefab::Register(name, [=](CrashoutsEngine::ECS& ecs) -> uint32_t {
			using Entity = uint32_t;
			Entity e = ecs.CreateEntity();
			ecs.AddComponent<NameComponent>(e, NameComponent{ name });
			ecs.AddComponent<PrefabInstanceComponent>(e, PrefabInstanceComponent{ name });

			// Active 
			if (actD.hasComp) {
				ecs.AddComponent<ActiveComponent>(e, actD.c);
			}

			// Transform
			TransformComponent tc{};
			tc.position = trD.pos;
			tc.rotation = trD.rot;
			tc.size = trD.size;
			ecs.AddComponent<TransformComponent>(e, tc);

			// Tag (formerly Layer)
			auto& tgc = ecs.AddComponent<Component::TagComponent>(e);
			tgc.tagMask = tgD.tagMask;

			// Layer (formerly ZOrder)
			if (ldD.hasComp) {
				auto& l = ecs.AddComponent<Component::LayerComponent>(e);
				l.layerName = ldD.layerName;
				l.z = ldD.z;
			}
			else {
				auto& l = ecs.AddComponent<Component::LayerComponent>(e);
				l.layerName = "Default";
				l.z = 0;
			}

			// Shape
			if (shD.hasComp) ecs.AddComponent<ShapeComponent>(e, shD.c);

			// Rigidbody
			if (rbD.hasComp) {
				auto& rb = ecs.AddComponent<RigidBodyComponent>(e, CEVec3{ 0,0,0 }, rbD.mass, rbD.rest);
				rb.staticFriction = rbD.sFric;
				rb.dynamicFriction = rbD.dFric;
				rb.inertia = rbD.inert;
				rb.invInertia = rbD.invInert;
				//if (!rbD.isTrigger) rb.getFlags().set(Physics::SIMULATE_DYNAMICS); // Set True if NOT a trigger
			}

			// Collider
			if (colD.hasComp) {
				// Reconstruct collider unique_ptr
				std::unique_ptr<Collision::Collider> ptr;
				switch (colD.type) {
				case Collision::ColliderType::CIRCLE:
					ptr = std::make_unique<Collision::Circle>(colD.radius, colD.pos);
					break;
				case Collision::ColliderType::OBB:
					ptr = std::make_unique<Collision::Polygon>(Collision::Polygon::CreateOBB(colD.min, colD.max, colD.rotation, CEVec3{}));
					static_cast<Collision::Polygon*>(ptr.get())->position = colD.pos; // Fix drift
					break;
				case Collision::ColliderType::POLYGON:
				case Collision::ColliderType::TRIANGLE: {
					auto p = std::make_unique<Collision::Polygon>();
					p->vertexCount = std::min((int)colD.verts.size(), Collision::Polygon::MAX_VERTEX);
					for (int i = 0; i < p->vertexCount; ++i) p->vertices[i] = colD.verts[i];
					p->rotation = colD.rotation;
					p->position = colD.pos;
					p->ForceUpdateRotation(colD.rotation);
					ptr = std::move(p);
					break;
				}
				case Collision::ColliderType::AABB:
				default:
					// Fallback logic if min/max are zero (e.g. from simple box prefab)
					if (colD.min.x == 0 && colD.max.x == 0) {
						ptr = MakeCenteredAABB3(trD.pos, trD.size.x, trD.size.y);
					}
					else {
						ptr = std::make_unique<Collision::AABBRect>(colD.min, colD.max);
					}
					break;
				}

				if (ptr) {
					ptr->ColliderType = colD.type;
					ptr->isTrigger = colD.isTrigger;
					ptr->enabled = colD.enabled;
					ptr->offset = colD.offset;
					// If pos wasn't set specifically by type, ensure it matches
					if (ptr->position.x == 0 && ptr->position.y == 0 && colD.type != Collision::ColliderType::CIRCLE) ptr->position = colD.pos;

					ecs.AddComponent<ColliderComponent>(e, ColliderComponent{ std::move(ptr) });
				}
			}

			// Text
			if (txD.hasComp) ecs.AddComponent<TextComponent>(e, txD.c);

			// Scripts
			if (scD.hasComp && !scD.scriptsJson.empty()) {
				auto& sc = ecs.AddComponent<ScriptComponent>(e);
				sc.EntityID = (uint32_t)e;

				rapidjson::Document sDoc;
				sDoc.Parse(scD.scriptsJson.c_str());
				if (!sDoc.HasParseError() && sDoc.IsArray()) {
					for (const auto& item : sDoc.GetArray()) {
						if (item.IsObject() && item.HasMember("className")) {
							sc.AddScript(item["className"].GetString());
						}
					}

					if (MonoManager::GetDomain() && MonoManager::GetAssembly()) {
						sc.Initialize(MonoManager::GetDomain(), MonoManager::GetAssembly());
						CrashoutsEngine::ScriptSystem::DeserializeFields(sDoc, sc);
					}
				}
			}

			// Audio
			if (auD.hasComp) ecs.AddComponent<AudioEmitterComponent>(e, auD.c);

			// Sprite Anim
			if (spD.hasComp) ecs.AddComponent<SpriteAnimatorComponent>(e, spD.c);

			// Keyframe Anim
			if (kfD.hasComp) ecs.AddComponent<KeyframeAnimatorComponent>(e, kfD.c);

			// Tilemap
			if (tmD.hasComp) {
				auto& tm = ecs.AddComponent<TileMapComponent>(e);
				tm.tileSize = tmD.c.tileSize;
				tm.activeAsset = tmD.c.activeAsset;
				tm.gridData = tmD.c.gridData;
				tm.tileAssetNames = tmD.c.tileAssetNames;
				tm.isCacheDirty = true;
			}

			// UI Components
			if (guiD.hasComp) ecs.AddComponent<GuiComponent>(e, guiD.c);
			{
				ecs.AddComponent<ButtonComponent>(e, btnD.c);

				// --- SYNC COLORS (Matches SceneAsset logic) ---
				if (auto* s = ecs.GetComponent<ShapeComponent>(e)) {
					s->color = btnD.c.shapeNormalColor;
				}
				if (auto* t = ecs.GetComponent<TextComponent>(e)) {
					t->color = btnD.c.textNormalColor;
				}
			}
			if (inpD.hasComp) ecs.AddComponent<TextInputComponent>(e, inpD.c);
			if (layD.hasComp) ecs.AddComponent<UILayoutComponent>(e, layD.c);

			// PP
			if (ppD.hasComp) ecs.AddComponent<PPComponent>(e, ppD.c);

			// Checkpoint
			if (cpD.hasComp) {
				auto& cp = ecs.AddComponent<CheckpointComponent>(e);
				cp.respawnPosition = cpD.c.respawnPosition;
				cp.targetRespawnEntity = cpD.c.targetRespawnEntity;
				cp.targetEntityName = cpD.c.targetEntityName;
				cp.activated = cpD.c.activated;
			}

			// Trigger
			if (trgD.hasComp) ecs.AddComponent<TriggerComponent>(e, trgD.c);

			// Parent (Data only)
			if (parD.hasComp) ecs.AddComponent<ParentComponent>(e, parD.c);
			return e;
		});
	}
	return true;
}

namespace CrashoutsEngine {
	using namespace ::Component;

	void InitPrefabs() {
		Prefab::SetFallback([](ECS& e, const std::string& name) -> uint32_t {
			auto entity = e.CreateEntity();
			e.AddComponent<NameComponent>(entity, NameComponent{ name });
			e.AddComponent<TransformComponent>(entity, TransformComponent{});
			return entity; // <--- Return the ID
			});

		namespace fs = std::filesystem;

#if USE_IMGUI_EDITOR
		const fs::path dir = fs::path(CrashoutsEngine::Assets::rootPath) / "prefabs";
#else
		const fs::path dir = fs::path(GetLocalAppData()) / "prefabs";
#endif
		if (fs::exists(dir) && fs::is_directory(dir)) {
			for (auto& entry : fs::directory_iterator(dir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					//std::cout << entry.path().generic_string() << std::endl;
					LoadPrefabsFromJson_RJ(entry.path().generic_string());
				}
			}
		}
	}

	void ReloadPrefabsFromDisk(const std::string& path) {
		// 1. Wipe everything (Factories + Fallback)
		Prefab::ClearRegistry();

		// 2. Re-register the default Fallback 
		Prefab::SetFallback([](ECS& e, const std::string& name) -> uint32_t {
			auto entity = e.CreateEntity();
			e.AddComponent<NameComponent>(entity, NameComponent{ name });
			e.AddComponent<TransformComponent>(entity, TransformComponent{});
			return entity; // <--- Return the ID
			});

		// 3. Load from the SPECIFIC path provided
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
	bool SavePrefabToJson(ECS& ecs, Entity e, const std::string& newName, const std::string& path, const std::string& matchName) {
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
		if (!doc.IsObject()) doc.SetObject();

		auto& allocator = doc.GetAllocator();
		if (!doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
			if (doc.HasMember("prefabs")) doc.RemoveMember("prefabs"); // Remove corrupt/null entry
			doc.AddMember("prefabs", Value(kArrayType), allocator);    // Create fresh array
		}

		Value& prefabs = doc["prefabs"]; // Now guaranteed to be a valid Array

		Value obj(kObjectType);
		obj.AddMember("type", "Box", allocator); // Tag type
		obj.AddMember("name", Value(newName.c_str(), allocator), allocator);

		// Active
		if (auto* ac = ecs.GetComponent<ActiveComponent>(e)) {
			Value aObj(kObjectType);
			aObj.AddMember("isActive", ac->isActive, allocator);
			obj.AddMember("active", aObj, allocator);
		}
		//Audio

		// Transform
		if (auto* tc = ecs.GetComponent<TransformComponent>(e)) {
			Value tObj(kObjectType);
			Value pos(kObjectType); pos.AddMember("x", tc->position.x, allocator).AddMember("y", tc->position.y, allocator).AddMember("z", tc->position.z, allocator);
			Value size(kObjectType); size.AddMember("x", tc->size.x, allocator).AddMember("y", tc->size.y, allocator);
			tObj.AddMember("position", pos, allocator).AddMember("rotation", tc->rotation, allocator).AddMember("size", size, allocator);
			obj.AddMember("transform", tObj, allocator);
		}

		// Tag (formerly Layer)
		if (auto* tc = ecs.GetComponent<Component::TagComponent>(e)) {
			Value tObj(kObjectType);
			tObj.AddMember("tagMask", tc->tagMask, allocator);
			obj.AddMember("tag", tObj, allocator);
		}

		// Layer (formerly ZOrder)
		if (auto* l = ecs.GetComponent<Component::LayerComponent>(e)) {
			Value lObj(kObjectType);
			lObj.AddMember("layerName", Value(l->layerName.c_str(), allocator), allocator);
			lObj.AddMember("z", l->z, allocator);
			obj.AddMember("layer", lObj, allocator);
		}

		// Audio
		if (auto* audio = ecs.GetComponent<AudioEmitterComponent>(e)) {
			rapidjson::Value audioObj(rapidjson::kObjectType);


			rapidjson::Value clipsArr(rapidjson::kArrayType);
			for (const auto& c : audio->clips) {
				rapidjson::Value clipObj(rapidjson::kObjectType);


				clipObj.AddMember("name", rapidjson::Value(c.name.c_str(), allocator), allocator);
				clipObj.AddMember("path", rapidjson::Value(c.path.c_str(), allocator), allocator);

				clipObj.AddMember("volume", c.volume, allocator);
				clipObj.AddMember("pitch", c.pitch, allocator);
				clipObj.AddMember("looping", c.looping, allocator);


				clipObj.AddMember("isMusic", c.isMusic, allocator);
				clipObj.AddMember("spatial3D", c.spatial3D, allocator);

				clipsArr.PushBack(clipObj, allocator);
			}

			audioObj.AddMember("clips", clipsArr, allocator);
			obj.AddMember("audio", audioObj, allocator);
		}

		// Shape
		if (auto* sc = ecs.GetComponent<ShapeComponent>(e)) {
			Value sObj(kObjectType);
			sObj.AddMember("type", (int)sc->type, allocator);
			Value col(kObjectType); col.AddMember("r", sc->color.r, allocator).AddMember("g", sc->color.g, allocator).AddMember("b", sc->color.b, allocator).AddMember("a", sc->color.a, allocator);
			sObj.AddMember("color", col, allocator);
			sObj.AddMember("enableTexture", sc->enableTexture, allocator);
			if (!sc->textureName.empty()) sObj.AddMember("textureName", Value(sc->textureName.c_str(), allocator), allocator);

			Value uvSize(kObjectType); uvSize.AddMember("x", sc->uvSize.x, allocator).AddMember("y", sc->uvSize.y, allocator);
			sObj.AddMember("uvSize", uvSize, allocator);
			Value uvOff(kObjectType); uvOff.AddMember("x", sc->uvOffset.x, allocator).AddMember("y", sc->uvOffset.y, allocator);
			sObj.AddMember("uvOffset", uvOff, allocator);

			sObj.AddMember("thickness", sc->thickness, allocator);
			sObj.AddMember("isWorldSpace", sc->isWorldSpace, allocator);
			sObj.AddMember("wrapModeU", sc->wrapModeU, allocator);
			sObj.AddMember("wrapModeV", sc->wrapModeV, allocator);
			obj.AddMember("shape", sObj, allocator);
		}

		// Rigidbody
		if (auto* rb = ecs.GetComponent<RigidBodyComponent>(e)) {
			Value rbObj(kObjectType);
			rbObj.AddMember("mass", rb->getMass(), allocator);
			rbObj.AddMember("restitution", rb->GetCoeff(), allocator);
			rbObj.AddMember("staticFriction", rb->staticFriction, allocator);
			rbObj.AddMember("dynamicFriction", rb->dynamicFriction, allocator);
			rbObj.AddMember("inertia", rb->inertia, allocator);
			rbObj.AddMember("invInertia", rb->invInertia, allocator);
			//bool isTrigger = !rb->getFlags().test(Physics::SIMULATE_DYNAMICS);
			//rbObj.AddMember("isTrigger", isTrigger, allocator);
			obj.AddMember("rigidbody", rbObj, allocator);
		}

		// Collider
		if (auto* col = ecs.GetComponent<ColliderComponent>(e)) {
			Value colObj(kObjectType);
			colObj.AddMember("ColliderType", (int)col->collider->ColliderType, allocator);
			colObj.AddMember("isTrigger", col->collider->isTrigger, allocator);
			colObj.AddMember("enabled", col->collider->enabled, allocator);

			Value pos(kObjectType); pos.AddMember("x", col->collider->position.x, allocator).AddMember("y", col->collider->position.y, allocator).AddMember("z", col->collider->position.z, allocator);
			colObj.AddMember("position", pos, allocator);
			Value off(kObjectType); off.AddMember("x", col->collider->offset.x, allocator).AddMember("y", col->collider->offset.y, allocator).AddMember("z", col->collider->offset.z, allocator);
			colObj.AddMember("offset", off, allocator);

			// Specifics based on Type
			switch (col->collider->ColliderType) {
			case Collision::ColliderType::CIRCLE: {
				auto* s = static_cast<Collision::Circle*>(col->collider.get());
				colObj.AddMember("radius", s->radius, allocator);
				break;
			}
			case Collision::ColliderType::AABB: {
				auto* a = static_cast<Collision::AABBRect*>(col->collider.get());
				Value min(kObjectType); min.AddMember("x", a->min.x, allocator).AddMember("y", a->min.y, allocator).AddMember("z", a->min.z, allocator);
				colObj.AddMember("min", min, allocator);
				Value max(kObjectType); max.AddMember("x", a->max.x, allocator).AddMember("y", a->max.y, allocator).AddMember("z", a->max.z, allocator);
				colObj.AddMember("max", max, allocator);
				break;
			}
											  // OBB/Poly logic omitted for brevity but should match sceneasset if needed
			}
			obj.AddMember("collider", colObj, allocator);
		}

		// Script
		if (auto* script = ecs.GetComponent<ScriptComponent>(e)) {
			Value sObj(kObjectType);
			Value arr(kArrayType);
			for (const auto& inst : script->Scripts) {
				if (inst.ClassName.empty()) continue;
				Value item(kObjectType);
				item.AddMember("className", Value(inst.ClassName.c_str(), allocator), allocator);

				// Serialize Fields
				if (inst.GCHandle && inst.Class) {
					MonoObject* instance = mono_gchandle_get_target(inst.GCHandle);
					if (instance) {
						Value fields(kObjectType);
						void* iter = nullptr;
						while (MonoClassField* field = mono_class_get_fields(inst.Class, &iter)) {
							const char* fName = mono_field_get_name(field);
							uint32_t flags = mono_field_get_flags(field);
							if (flags & MONO_FIELD_ATTR_STATIC) continue;

							bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
							bool hasAttr = false;
							MonoClass* attrClass = MonoManager::GetSerializeFieldClass();
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

								if (tEnum == MONO_TYPE_I4) { int v; mono_field_get_value(instance, field, &v); fields.AddMember(key, v, allocator); }
								else if (tEnum == MONO_TYPE_R4) { float v; mono_field_get_value(instance, field, &v); fields.AddMember(key, v, allocator); }
								else if (tEnum == MONO_TYPE_BOOLEAN) { bool v; mono_field_get_value(instance, field, &v); fields.AddMember(key, v, allocator); }
								else if (tEnum == MONO_TYPE_STRING) {
									MonoString* ms = nullptr; mono_field_get_value(instance, field, &ms);
									if (ms) { char* s = mono_string_to_utf8(ms); fields.AddMember(key, rapidjson::Value(s, allocator), allocator); mono_free(s); }
								}
							}
						}
						item.AddMember("fields", fields, allocator);
					}
				}
				arr.PushBack(item, allocator);
			}
			sObj.AddMember("scripts", arr, allocator);
			obj.AddMember("script", sObj, allocator);
		}
		// Particle
		if (auto* pc = ecs.GetComponent<Component::ParticleComponent>(e)) {
			using namespace rapidjson;
			Value pObj(kObjectType);

			pObj.AddMember("playOnStop", pc->playOnStop, allocator);
			pObj.AddMember("isEmitting", pc->isEmitting, allocator);
			pObj.AddMember("useWorldSpace", pc->useWorldSpace, allocator);

			const char* shapeStr = "LINE";
			if (pc->emissionShape == ParticleSystem::ParticleShape::CIRCLE) shapeStr = "CIRCLE";
			if (pc->emissionShape == ParticleSystem::ParticleShape::SQUARE) shapeStr = "SQUARE";
			pObj.AddMember("emissionShape", Value(shapeStr, allocator), allocator);

			pObj.AddMember("lengthMin", pc->lengthMin, allocator);
			pObj.AddMember("lengthMax", pc->lengthMax, allocator);
			pObj.AddMember("radiusMin", pc->radiusMin, allocator);
			pObj.AddMember("radiusMax", pc->radiusMax, allocator);

			auto writeVec2 = [&](float x, float y) {
				Value o(kObjectType);
				o.AddMember("x", x, allocator);
				o.AddMember("y", y, allocator);
				return o;
				};
			auto writeVec3 = [&](const CEVec3& v) {
				Value o(kObjectType);
				o.AddMember("x", v.x, allocator);
				o.AddMember("y", v.y, allocator);
				o.AddMember("z", v.z, allocator);
				return o;
				};
			auto writeColor = [&](const CEColor& c) {
				Value o(kObjectType);
				o.AddMember("x", c.x(), allocator);
				o.AddMember("y", c.y(), allocator);
				o.AddMember("z", c.z(), allocator);
				o.AddMember("w", c.w(), allocator);
				return o;
				};

			pObj.AddMember("areaMin", writeVec2(pc->areaMin.x, pc->areaMin.y), allocator);
			pObj.AddMember("areaMax", writeVec2(pc->areaMax.x, pc->areaMax.y), allocator);

			pObj.AddMember("maxParticles", pc->maxParticles, allocator);
			pObj.AddMember("emitRate", pc->emitRate, allocator);

			pObj.AddMember("startLifeMin", pc->startLifeMin, allocator);
			pObj.AddMember("startLifeMax", pc->startLifeMax, allocator);
			pObj.AddMember("startRotationMin", pc->startRotationMin, allocator);
			pObj.AddMember("startRotationMax", pc->startRotationMax, allocator);
			pObj.AddMember("startSpeedMin", pc->startSpeedMin, allocator);
			pObj.AddMember("startSpeedMax", pc->startSpeedMax, allocator);

			pObj.AddMember("uniformSize", pc->uniformSize, allocator);
			pObj.AddMember("startSizeXMin", pc->startSizeXMin, allocator);
			pObj.AddMember("startSizeXMax", pc->startSizeXMax, allocator);
			pObj.AddMember("startSizeYMin", pc->startSizeYMin, allocator);
			pObj.AddMember("startSizeYMax", pc->startSizeYMax, allocator);

			pObj.AddMember("startColor", writeColor(pc->startcolor), allocator);
			pObj.AddMember("endColor", writeColor(pc->endColor), allocator);

			pObj.AddMember("startVelocity", writeVec3(pc->startVelocity), allocator);
			pObj.AddMember("endVelocity", writeVec3(pc->endVelocity), allocator);
			pObj.AddMember("gravity", writeVec3(pc->gravity), allocator);

			pObj.AddMember("rotationOverTime", pc->rotationOverTime, allocator);
			pObj.AddMember("sizeOverTime", pc->sizeOverTime, allocator);

			pObj.AddMember("useTexture", pc->useTexture, allocator);
			if (!pc->textureName.empty())
				pObj.AddMember("textureName", Value(pc->textureName.c_str(), allocator), allocator);
			pObj.AddMember("textureUV", writeVec2(pc->textureUV.x, pc->textureUV.y), allocator);

			obj.AddMember("particle", pObj, allocator);
		}


		// Camera
		if (auto* cam = ecs.GetComponent<Component::CameraComponent>(e)) {
			rapidjson::Value camObj(rapidjson::kObjectType);

			camObj.AddMember("zoom", cam->zoom, allocator);
			camObj.AddMember("x", cam->position.x, allocator);
			camObj.AddMember("y", cam->position.y, allocator);

			rapidjson::Value xb(rapidjson::kArrayType);
			xb.PushBack(cam->xBounds.x, allocator);
			xb.PushBack(cam->xBounds.y, allocator);
			camObj.AddMember("xBounds", xb, allocator);

			rapidjson::Value yb(rapidjson::kArrayType);
			yb.PushBack(cam->yBounds.x, allocator);
			yb.PushBack(cam->yBounds.y, allocator);
			camObj.AddMember("yBounds", yb, allocator);

			obj.AddMember("camera", camObj, allocator);
		}




		// Sprite Animator
		if (auto* sa = ecs.GetComponent<SpriteAnimatorComponent>(e)) {
			Value animObj(kObjectType);
			animObj.AddMember("currentClip", Value(sa->currentClipName.c_str(), allocator), allocator);
			animObj.AddMember("playing", sa->playing, allocator);
			animObj.AddMember("flipH", sa->flipH, allocator);

			Value clips(kArrayType);
			for (auto& c : sa->clips) {
				Value clip(kObjectType);
				clip.AddMember("name", Value(c.name.c_str(), allocator), allocator);
				clip.AddMember("start", c.startFrame, allocator);
				clip.AddMember("count", c.frameCount, allocator);
				clip.AddMember("fps", c.fps, allocator);
				clip.AddMember("loop", c.loop, allocator);
				clips.PushBack(clip, allocator);
			}
			animObj.AddMember("clips", clips, allocator);
			obj.AddMember("sprite_animator", animObj, allocator);
		}

		// Tilemap
		if (auto* tm = ecs.GetComponent<TileMapComponent>(e)) {
			Value tmObj(kObjectType);
			tmObj.AddMember("tileSize", tm->tileSize, allocator);
			tmObj.AddMember("activeAsset", Value(tm->activeAsset.c_str(), allocator), allocator);

			Value tiles(kArrayType);
			for (const auto& [pos, asset] : tm->gridData) {
				Value t(kObjectType);
				t.AddMember("x", pos.first, allocator);
				t.AddMember("y", pos.second, allocator);
				t.AddMember("asset", Value(asset.c_str(), allocator), allocator);
				tiles.PushBack(t, allocator);
			}
			tmObj.AddMember("tiles", tiles, allocator);
			obj.AddMember("tilemap", tmObj, allocator);
		}

		// Parent
		if (auto* pc = ecs.GetComponent<ParentComponent>(e)) {
			Value pObj(kObjectType);
			Value pos(kObjectType); pos.AddMember("x", pc->localPos.x, allocator).AddMember("y", pc->localPos.y, allocator).AddMember("z", pc->localPos.z, allocator);
			pObj.AddMember("localPos", pos, allocator);
			pObj.AddMember("localRot", pc->localRot, allocator);
			Value scl(kObjectType); scl.AddMember("x", pc->localScale.x, allocator).AddMember("y", pc->localScale.y, allocator);
			pObj.AddMember("localScale", scl, allocator);
			obj.AddMember("parent", pObj, allocator);
		}

		// GUI / Button / Input / Layout
		if (auto* gui = ecs.GetComponent<GuiComponent>(e)) {
			Value gObj(kObjectType);
			gObj.AddMember("uiType", static_cast<unsigned>(gui->uiType), allocator);
			obj.AddMember("gui", gObj, allocator);
		}
		if (auto* btn = ecs.GetComponent<ButtonComponent>(e)) {
			Value bObj(kObjectType);
			bObj.AddMember("buttonState", static_cast<unsigned>(btn->buttonState), allocator);

			// Helper lambda to write a color object
			auto WriteColor = [&](const char* key, const CEColor& c) {
				Value col(kObjectType);
				col.AddMember("x", c.x(), allocator).AddMember("y", c.y(), allocator)
					.AddMember("z", c.z(), allocator).AddMember("w", c.w(), allocator);
				bObj.AddMember(Value(key, allocator), col, allocator);
				};

			WriteColor("shapeNormalColor", btn->shapeNormalColor);
			WriteColor("shapeHoveredColor", btn->shapeHoveredColor);
			WriteColor("shapePressedColor", btn->shapePressedColor);
			WriteColor("textNormalColor", btn->textNormalColor);
			WriteColor("textHoveredColor", btn->textHoveredColor);
			WriteColor("textPressedColor", btn->textPressedColor);

			if (!btn->textureNormal.empty()) bObj.AddMember("textureNormal", Value(btn->textureNormal.c_str(), allocator), allocator);
			if (!btn->textureHovered.empty()) bObj.AddMember("textureHovered", Value(btn->textureHovered.c_str(), allocator), allocator);
			if (!btn->texturePressed.empty()) bObj.AddMember("texturePressed", Value(btn->texturePressed.c_str(), allocator), allocator);

			obj.AddMember("button", bObj, allocator);
		}

		// Checkpoint
		if (auto* c = ecs.GetComponent<CheckpointComponent>(e)) {
			Value val(kObjectType);
			Value pos(kObjectType);
			pos.AddMember("x", c->respawnPosition.x, allocator).AddMember("y", c->respawnPosition.y, allocator).AddMember("z", c->respawnPosition.z, allocator);
			val.AddMember("respawnPosition", pos, allocator);
			val.AddMember("targetRespawnEntity", c->targetRespawnEntity, allocator);
			val.AddMember("targetEntityName", Value(c->targetEntityName.c_str(), allocator), allocator);
			val.AddMember("activated", c->activated, allocator);
			obj.AddMember("checkpoint", val, allocator);
		}

		// Trigger
		if (auto* c = ecs.GetComponent<TriggerComponent>(e)) {
			Value val(kObjectType);
			val.AddMember("triggerID", Value(c->triggerID.c_str(), allocator), allocator);
			val.AddMember("oneTimeUse", c->oneTimeUse, allocator);
			val.AddMember("triggered", c->triggered, allocator);
			obj.AddMember("trigger_metadata", val, allocator);
		}

		// Upsert
		bool replaced = false;
		std::string searchKey = matchName.empty() ? newName : matchName;
		if (prefabs.IsArray()) {
			for (auto& p : prefabs.GetArray()) {
				// Safety check for name existence and type
				if (p.HasMember("name") && p["name"].IsString() && std::string(p["name"].GetString()) == searchKey) {
					p.CopyFrom(obj, allocator);
					replaced = true;
					break;
				}
			}
		}
		if (!replaced) prefabs.PushBack(obj, allocator);

		StringBuffer sb;
		PrettyWriter<StringBuffer> writer(sb);
		doc.Accept(writer);

		std::ofstream out(path);
		out << sb.GetString();
		return true;
	}
}