/*!
@file        prefab_init.h
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Declares the InitPrefabs() function that registers all the prefab when it starts.

*/
#pragma once
#include "entitymanager.h"
#include <string>
namespace CrashoutsEngine {
    bool SaveBoxPrefabToJson_RJ(ECS& ecs,
        Entity e,
        const std::string& newName,
        const std::string& path = "assets/prefabs/prefab.json",
        const std::string& matchName = "");
    void InitPrefabs();
    void ReloadPrefabsFromDisk(const std::string& path);
    void RecreateInstancesForPrefab(ECS& ecs, const std::string& prefabName);
    void InitPrefabs();
}