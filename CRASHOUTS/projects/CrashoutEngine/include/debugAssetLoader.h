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
