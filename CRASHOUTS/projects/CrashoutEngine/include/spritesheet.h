/*!
@file       spritesheet.h
@author     Afiq
@author     Shahrin Idzuan
@author     Ranvitha
@date       19/11/2025
@brief      This file contains the declaration of sprite sheet related components

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#pragma once

namespace CrashoutsEngine { class ECS; using Entity = unsigned; }

namespace UI
{
    void RenderSpriteSheetPanel(CrashoutsEngine::ECS& ecs,
        CrashoutsEngine::Entity selected);
    void SpriteSheet_SetTexturePath(const char* path);
}

