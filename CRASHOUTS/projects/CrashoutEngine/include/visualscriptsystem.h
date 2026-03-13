/*!
@file       visualscriptsystem.h
@author     Afiq
@date       13/03/2026
@brief      Declarations for visual scripting execution and update utilities.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once
#include <string>
#include "debugmenu.h" 
#include "visualscriptcomponent.h"

namespace CrashoutsEngine { class ECS; }

/*!
@brief Execute a single visual script graph for an entity.
@param ecs ECS instance to query/modify.
@param e Entity to execute the script for.
@param comp Visual script component containing graph state and variables.
@param dt Delta time in seconds.
*/
void ExecuteVisualScript(CrashoutsEngine::ECS& ecs,
    CrashoutsEngine::Entity e,
    Component::VisualScriptComponent& comp,
    float dt);

/*!
@brief Update all visual scripts for entities that have a VisualScriptComponent.
@param ecs ECS instance to query/modify.
@param dt Delta time in seconds.
*/
void UpdateVisualScripts(CrashoutsEngine::ECS& ecs, float dt);

/*!
@brief Invalidate cached visual script graphs for a given asset path.
@param path Graph asset path to invalidate.
*/
void InvalidateVisualScriptCache(const std::string& path);
