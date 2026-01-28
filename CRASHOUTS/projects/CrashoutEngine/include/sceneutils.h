/*!
@file        sceneutils.h
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements the main functions that run in engine.cpp to Update
             visuals and elements in the scene.

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once

#include "entitymanager.h"

#include "textcomponent.h"

namespace Scene {
    namespace UI {
        void LoadDialogue(Component::TextComponent& tc);
    }


    /**
     * @name    GetMouseInWorld
     * @brief   Gets the mouse coordinates in world
     */
    CEVec3 GetMouseInWorld();

    /**
     * @name    InitScene
     * @brief   Initialize Required Scene data.
     * @params  gECS - ECS to reference.
     */
    void InitScene(CrashoutsEngine::ECS& gECS);

    /**
     * @name    RenderScene
     * @brief   Renders the scene by cycling through all ShapeComponents used.
     * @params  gECS - ECS to reference.
     */
	void RenderScene(CrashoutsEngine::ECS& gECS);
	
    /**
     * @name    UpdateScene
     * @brief   Updates the Scene especially from in game UI interaction.
     * @params  gECS - ECS to reference.
     */
    void UpdateScene(CrashoutsEngine::ECS& gECS);
}