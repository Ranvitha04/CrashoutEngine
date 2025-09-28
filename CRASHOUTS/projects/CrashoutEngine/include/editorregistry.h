/*!
@file       editorregistry.h
@author     mohamadamirakmal.b@digipen.edu
@date       25/09/2025

@brief      Lightweight registry hooks for the Debug Menu. Provides:
            - IComponentAdapter + ComponentRegistry: ECS inspectors (add/remove/edit)
            - IGameStateProvider setter/getter: data-driven Game State panel
            - PanelRegistry: register custom ImGui panels at runtime
            - IPhysicsProvider + PhysicsRegistry: physics debug controls

These APIs let the debug UI be data-driven without hardcoding per-project UI.
//__________________________________________________________________________*/

#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace CrashoutsEngine { class ECS; using Entity = unsigned; }

namespace EditorRegistry {

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

    // Game State provider (reflection-lite)
    struct IGameStateProvider {
        virtual ~IGameStateProvider() = default;
        virtual int  current()  const = 0;
        virtual int  previous() const = 0;
        virtual int  next()     const = 0;
        virtual void changeTo(int id) = 0;
        virtual const std::vector<std::pair<const char*, int>>& states() const = 0;
    };

    // Set/get the active provider (game installs it at startup)
    void SetGameStateProvider(IGameStateProvider* p);
    IGameStateProvider* GetGameStateProvider();

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

    // Physics provider
    struct IPhysicsProvider {
        virtual ~IPhysicsProvider() = default;
        virtual const char* name() const = 0;
        virtual void draw(CrashoutsEngine::ECS& ecs) = 0;  // render ImGui for this provider
    };

    class PhysicsRegistry {
    public:
        static PhysicsRegistry& instance();
        void registerProvider(IPhysicsProvider* p);
        const std::vector<IPhysicsProvider*>& all() const;
    private:
        std::vector<IPhysicsProvider*> providers_;
    };

} // namespace EditorRegistry