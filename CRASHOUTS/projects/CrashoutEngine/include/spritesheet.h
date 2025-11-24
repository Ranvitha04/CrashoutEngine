#pragma once
namespace CrashoutsEngine { class ECS; using Entity = unsigned; }

namespace UI
{
    void RenderSpriteSheetPanel(CrashoutsEngine::ECS& ecs,
        CrashoutsEngine::Entity selected);
    void SpriteSheet_SetTexturePath(const char* path);
}
