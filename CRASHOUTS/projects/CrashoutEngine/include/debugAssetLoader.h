/*!
@file       debugAssetLoader.h
@author     Mohamad Amir Akmal
@author     Ranvitha
@date       23/11/2025
@brief      Helpers for asset drag-and-drop payloads in Crashout Engine's debug tools.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once
#include <cstdint>

enum class AssetKind : uint8_t {
    Texture,
    SpriteSheet,
    Shader,
    Audio,
    Scene,
    Prefab,
    Unknown
};

struct AssetDragPayload {
    AssetKind kind{ AssetKind::Unknown };
    char name[128]{};     // manifest/manager "name" (stem)
    char relPath[256]{};  // relative path under assets/ (optional)
};

inline const char* ExpectedAssetKindName(AssetKind k) {
    switch (k) {
    case AssetKind::Texture:     return "Texture";
    case AssetKind::SpriteSheet: return "Sprite Sheet";
    case AssetKind::Audio:       return "Audio";
    case AssetKind::Shader:      return "Shader";
    case AssetKind::Scene:       return "Scene";
    case AssetKind::Prefab:      return "Prefab";
    default:                     return "Unknown";
    }
}