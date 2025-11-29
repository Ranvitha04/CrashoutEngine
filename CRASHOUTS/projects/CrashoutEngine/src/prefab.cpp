/*!
@file        prefab.cpp
@author      b.mohamadafiq@digipen.edu
@date        28/9/2025
@brief       Factory where we make prefab

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/
#include "prefab.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace {
    std::unordered_map<std::string, Prefab::Factory>& Reg() {
        static std::unordered_map<std::string, Prefab::Factory> R;
        return R;
    }
    Prefab::Fallback& FallbackFn() {
        static Prefab::Fallback FB; 
        return FB;
    }
}

namespace Prefab {

    bool Register(const std::string& name, Factory f) {
        Reg()[name] = std::move(f);   
        return true;
    }


    //Remove prefab
    bool Unregister(const std::string& name) { return Reg().erase(name) > 0; }
    bool Exists(const std::string& name) { return Reg().find(name) != Reg().end(); }
    //Get a list sort them by names
    std::vector<std::string> Names() {
        std::vector<std::string> v;
        v.reserve(Reg().size());
        for (auto& kv : Reg()) v.push_back(kv.first);
        std::sort(v.begin(), v.end());
        return v;
    }
    //Make gameobj
    bool Create(CrashoutsEngine::ECS& ecs, const std::string& name) {
        auto it = Reg().find(name);
        if (it != Reg().end()) { it->second(ecs); return true; }
        auto& fb = FallbackFn();
        if (fb) { fb(ecs, name); return true; }
        return false;
    }

    void SetFallback(Fallback fb) { FallbackFn() = std::move(fb); }
    //Load prefab from file
    bool LoadFromFile(
        const std::string& path,
        const std::function<bool(
            const std::string&,
            const std::vector<std::string>&,
            Factory&)>& translator)
    {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            auto beg = line.find_first_not_of(" \t\r\n");
            if (beg == std::string::npos) continue;
            if (line[beg] == '#') continue;

            std::istringstream iss(line);
            std::string type; iss >> type;
            if (type.empty()) continue;

            std::vector<std::string> tokens;
            for (std::string tok; iss >> tok; ) tokens.push_back(tok);
            if (tokens.empty()) continue; 
            const std::string& name = tokens[0];

            Factory f;
            if (translator && translator(type, tokens, f)) {
                Register(name, std::move(f));
            }
        }
        return true;
    }

} 
