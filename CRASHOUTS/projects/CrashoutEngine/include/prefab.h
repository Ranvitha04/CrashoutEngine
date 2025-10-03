#pragma once
// prefab.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace CrashoutsEngine { class ECS; } // forward-declare; no engine headers here

namespace Prefab {

    // A prefab "factory" mutates ECS (create entity, add comps…)
    using Factory = std::function<void(CrashoutsEngine::ECS&)>;
    using Fallback = std::function<void(CrashoutsEngine::ECS&, const std::string& name)>;

    // Registry API (store by NAME)
    bool                     Register(const std::string& name, Factory f);  // false if already exists
    bool                     Unregister(const std::string& name);
    bool                     Exists(const std::string& name);
    std::vector<std::string> Names();                                       // sorted names (for UI)

    // Create by NAME. If registered → calls its Factory.
    // If not registered but a fallback exists → calls fallback(name).
    // Returns true if something was created (factory or fallback), false otherwise.
    bool                     Create(CrashoutsEngine::ECS& ecs, const std::string& name);

    // Optional: set a fallback used when name is unknown
    void                     SetFallback(Fallback fb);

    // Optional loader: parse a text file via translator
    bool                     LoadFromFile(
        const std::string& path,
        const std::function<bool(
            const std::string& /*type*/,
            const std::vector<std::string>& /*tokens*/,
            Factory& /*out*/)>& translator);

} // namespace Prefab
