#pragma once
#include <string>
#include "debugmenu.h" 
#include "visualscriptcomponent.h"

namespace CrashoutsEngine { class ECS; }

void ExecuteVisualScript(CrashoutsEngine::ECS& ecs,
    CrashoutsEngine::Entity e,
    Component::VisualScriptComponent& comp,
    float dt);
void UpdateVisualScripts(CrashoutsEngine::ECS& ecs, float dt);
void InvalidateVisualScriptCache(const std::string& path);