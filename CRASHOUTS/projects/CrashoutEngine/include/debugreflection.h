#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace CrashoutsEngine { class ECS; using Entity = unsigned; }

namespace DebugReflection {

    // ---------- Component Adapters ----------
    // Each adapter knows how to: check, add, remove, and draw its inspector UI.
    struct IComponentAdapter {
        virtual ~IComponentAdapter() = default;
        virtual const char* name() const = 0;
        virtual bool has(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) const = 0;
        virtual void add(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) = 0;
        virtual void remove(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) = 0;
        virtual void drawInspector(CrashoutsEngine::ECS& ecs, CrashoutsEngine::Entity e) = 0;
    };

    // Global registry you can populate at app startup.
    class ComponentRegistry {
    public:
        static ComponentRegistry& instance();

        void registerAdapter(IComponentAdapter* adapter);   // call once; keeps ownership
        const std::vector<IComponentAdapter*>& all() const { return list; }

    private:
        std::vector<IComponentAdapter*> list;
    };

    // ---------- Simple debug panel registry -------------
    using PanelFn = std::function<void()>;

    struct Panel {
        std::string name;
        bool* toggleFlag;        // optional: if you want it to show up in the toggle window
        PanelFn render;
    };

    class PanelRegistry {
    public:
        static PanelRegistry& instance();

        // If toggleFlag is non-null, the toggle window will show a checkbox for it.
        void registerPanel(const std::string& name, PanelFn fn, bool* toggleFlag = nullptr);
        const std::vector<Panel>& panels() const { return list; }

    private:
        std::vector<Panel> list;
    };

} // namespace DebugReflection