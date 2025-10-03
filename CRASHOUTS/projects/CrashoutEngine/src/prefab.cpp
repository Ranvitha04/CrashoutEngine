// prefab.cpp
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
        static Prefab::Fallback FB; // empty by default
        return FB;
    }
}

namespace Prefab {

    bool Register(const std::string& name, Factory f) {
        auto& r = Reg();
        if (r.find(name) != r.end()) return false;
        r.emplace(name, std::move(f));
        return true;
    }

    bool Unregister(const std::string& name) { return Reg().erase(name) > 0; }
    bool Exists(const std::string& name) { return Reg().find(name) != Reg().end(); }

    std::vector<std::string> Names() {
        std::vector<std::string> v;
        v.reserve(Reg().size());
        for (auto& kv : Reg()) v.push_back(kv.first);
        std::sort(v.begin(), v.end());
        return v;
    }

    bool Create(CrashoutsEngine::ECS& ecs, const std::string& name) {
        auto it = Reg().find(name);
        if (it != Reg().end()) { it->second(ecs); return true; }
        auto& fb = FallbackFn();
        if (fb) { fb(ecs, name); return true; }
        return false;
    }

    void SetFallback(Fallback fb) { FallbackFn() = std::move(fb); }

    // Simple line-based loader via translator hook
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
            if (tokens.empty()) continue; // need at least NAME at tokens[0]
            const std::string& name = tokens[0];

            Factory f;
            if (translator && translator(type, tokens, f)) {
                Register(name, std::move(f));
            }
        }
        return true;
    }

} // namespace Prefab
