/*!
@file        sceneutils.h
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements the main functions that run in engine.cpp to update
             visuals and elements in the scene.
*/
#pragma once

#include "entitymanager.h"

namespace Scene {
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