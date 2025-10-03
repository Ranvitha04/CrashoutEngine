/*!
@file        prefab.h
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Store simple prefab by name

*/
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace CrashoutsEngine { class ECS; }

namespace Prefab {

    using Factory = std::function<void(CrashoutsEngine::ECS&)>;
    using Fallback = std::function<void(CrashoutsEngine::ECS&, const std::string& name)>;

    bool Register(const std::string& name, Factory f);  
    bool Unregister(const std::string& name);
    bool Exists(const std::string& name);
    std::vector<std::string> Names();                                       


    bool Create(CrashoutsEngine::ECS& ecs, const std::string& name);

    // Optional: set a fallback used when name is unknown
    void SetFallback(Fallback fb);

    // Optional loader: parse a text file via translator
    bool LoadFromFile(const std::string& path, const std::function<bool(
    const std::string& ,const std::vector<std::string>&, Factory&)>& translator);

} 
