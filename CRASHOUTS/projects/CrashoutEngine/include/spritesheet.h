#pragma once

/*!
All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

namespace CrashoutsEngine { class ECS; using Entity = unsigned; }

namespace UI
{
    void RenderSpriteSheetPanel(CrashoutsEngine::ECS& ecs,
        CrashoutsEngine::Entity selected);
    void SpriteSheet_SetTexturePath(const char* path);
}
