/*!
@file        sceneutils.h
@author      lee.r@digipen.edu
@date        29/09/2025
@brief       Implements the

This file implements
*/
#pragma once

#include "entitymanager.h"

namespace Scene {
	void RenderScene(CrashoutsEngine::ECS& gECS);
	void UpdateScene(CrashoutsEngine::ECS& gECS);
}