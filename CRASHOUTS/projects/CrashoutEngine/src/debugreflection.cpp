#include "debugreflection.h"

namespace DebugReflection {

    ComponentRegistry& ComponentRegistry::instance() {
        static ComponentRegistry R;
        return R;
    }
    void ComponentRegistry::registerAdapter(IComponentAdapter* adapter) {
        list.push_back(adapter);
    }

    PanelRegistry& PanelRegistry::instance() {
        static PanelRegistry R;
        return R;
    }
    void PanelRegistry::registerPanel(const std::string& name, PanelFn fn, bool* toggleFlag) {
        Panel p{ name, toggleFlag, std::move(fn) };
        list.push_back(std::move(p));
    }

} // namespace DebugReflection