/*!
@file        prefab.h
@author      b.mohamadafiq@digipen.edu
@date        7/11/2025
@brief       Unified Prefab system: Registry, Factory, JSON Load/Save.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#pragma once
#include "entitymanager.h"
#include <string>
#include <vector>
#include <functional>

namespace CrashoutsEngine { class ECS; }

namespace Prefab {

    using Factory = std::function<void(CrashoutsEngine::ECS&)>;
    using Fallback = std::function<void(CrashoutsEngine::ECS&, const std::string& name)>;

    bool Register(const std::string& name, Factory f);  
    bool Unregister(const std::string& name);
    bool Exists(const std::string& name);
    std::vector<std::string> Names();                                       
    void ClearRegistry();

    bool Create(CrashoutsEngine::ECS& ecs, const std::string& name);
    void SetFallback(Fallback fb);    
} 

namespace CrashoutsEngine {
    // High-Level Operations
    void InitPrefabs();
    void ReloadPrefabsFromDisk(const std::string& path);
    void RecreateInstancesForPrefab(ECS& ecs, const std::string& prefabName);

    // Save entity as prefab
    bool SavePrefabToJson(ECS& ecs,
        Entity e,
        const std::string& newName,
        const std::string& path,
        const std::string& matchName = "");
}