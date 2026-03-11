#include "visualscriptsystem.h"
#include "transformcomponent.h"
#include "visualscriptcomponent.h"
#include "scriptcomponent.h"
#include "debugmenu.h"
#include "monomanager.h"

#include <mono/metadata/object.h>
#include <mono/metadata/debug-helpers.h>

#include <iostream>
#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>
using namespace CrashoutsEngine;

// Forward declaration
std::shared_ptr<UI::VisualScriptingGraph> GetOrLoadGraph(const std::string& path);

namespace
 {   
    std::unordered_map<std::string, std::shared_ptr<UI::VisualScriptingGraph>> g_visualScriptCache;

    UI::VSNode* FindNodeById(UI::VisualScriptingGraph& graph, int id)
    {
        for (auto& node : graph.nodes) {
            if (node.id == id)
                return &node;
        }
        return nullptr;
    }

    static bool EvaluateBoolNode(ECS& ecs, Entity e, Component::VisualScriptComponent& comp,
        UI::VisualScriptingGraph& graph, int nodeId, bool& outValue)
    {
        UI::VSNode* node = FindNodeById(graph, nodeId);
        if (!node)
            return false;

        switch (node->type)
        {
        case UI::VSNodeType::BoolConstant:
            outValue = node->boolValue;
            return true;

        case UI::VSNodeType::GetVariable:
        {
            auto it = comp.boolVariables.find(node->variableName);
            if (it == comp.boolVariables.end())
                return false;

            outValue = it->second;
            return true;
        }

        default:
            return false;
        }
    }

    UI::VSNode* FindStartNode(UI::VisualScriptingGraph& graph)
    {
        for (auto& node : graph.nodes) {
            if (node.type == UI::VSNodeType::Start)
                return &node;
        }
        return nullptr;
    }

    UI::VSNode* FindNextNode(UI::VisualScriptingGraph& graph, int currentNodeId, int outputPin = 0)
    {
        for (auto& link : graph.links) {
            if (link.fromNode == currentNodeId && link.fromPin == outputPin) {
                return FindNodeById(graph, link.toNode);
            }
        }
        return nullptr;
    }

    bool ExecuteMonoInstanceMethod(ECS& ecs,
        Entity entity,
        const std::string& className,
        const std::string& methodName)
    {
        auto* scriptComp = ecs.GetComponent<Component::ScriptComponent>(entity);
        if (!scriptComp)
            return false;

        bool foundMatching = false;

        for (auto& inst : scriptComp->Scripts)
        {
            if (inst.ClassName != className)
                continue;

            foundMatching = true;

            if (!inst.Class)
            {
                std::cerr << "[VisualScript] Script instance '" << inst.ClassName << "' has no MonoClass (not initialized yet)\n";
                continue;
            }

            if (!inst.GCHandle)
            {
                std::cerr << "[VisualScript] Script instance '" << inst.ClassName << "' has no GCHandle (not initialized yet)\n";
                continue;
            }

            MonoObject* instance = mono_gchandle_get_target(inst.GCHandle);
            if (!instance)
            {
                std::cerr << "[VisualScript] mono_gchandle_get_target returned null for '" << inst.ClassName << "'\n";
                continue;
            }

            MonoMethod* method = mono_class_get_method_from_name(inst.Class, methodName.c_str(), 0);
            if (!method)
            {
                std::cerr << "[VisualScript] Method '" << methodName << "' not found on class '" << inst.ClassName << "'\n";
                continue;
            }

            MonoObject* exception = nullptr;
            mono_runtime_invoke(method, instance, nullptr, &exception);

            if (exception)
            {
                MonoString* excStr = mono_object_to_string(exception, nullptr);
                char* utf8 = mono_string_to_utf8(excStr);
                std::cerr << "[VisualScript] Mono exception invoking " << className << "::" << methodName << ": "
                    << (utf8 ? utf8 : "unknown") << "\n";
                if (utf8) mono_free(utf8);
                // We attempted invocation, consider this a failure for the node.
                return false;
            }

            return true;
        }

        if (!foundMatching)
        {
            std::cerr << "[VisualScript] No script instance with class name '" << className << "' found on entity " << entity << "\n";
        }
        return false;
    }

    bool GetMonoBoolProperty(ECS& ecs,
        Entity entity,
        const std::string& className,
        const std::string& propertyName,
        bool& outValue)
    {
        auto* scriptComp = ecs.GetComponent<Component::ScriptComponent>(entity);
        if (!scriptComp)
            return false;

        bool foundMatching = false;

        for (auto& inst : scriptComp->Scripts)
        {
            if (inst.ClassName != className)
                continue;

            foundMatching = true;

            if (!inst.Class)
            {
                std::cerr << "[VisualScript] Script instance '" << inst.ClassName << "' has no MonoClass (not initialized yet)\n";
                continue;
            }

            if (!inst.GCHandle)
            {
                std::cerr << "[VisualScript] Script instance '" << inst.ClassName << "' has no GCHandle (not initialized yet)\n";
                continue;
            }

            MonoObject* instance = mono_gchandle_get_target(inst.GCHandle);
            if (!instance)
            {
                std::cerr << "[VisualScript] mono_gchandle_get_target returned null for '" << inst.ClassName << "'\n";
                continue;
            }

            MonoProperty* prop = mono_class_get_property_from_name(inst.Class, propertyName.c_str());
            if (!prop)
            {
                std::cerr << "[VisualScript] Property '" << propertyName << "' not found on class '" << inst.ClassName << "'\n";
                continue;
            }

            MonoMethod* getter = mono_property_get_get_method(prop);
            if (!getter)
            {
                std::cerr << "[VisualScript] Getter for property '" << propertyName << "' not found on class '" << inst.ClassName << "'\n";
                continue;
            }

            MonoObject* exception = nullptr;
            MonoObject* result = mono_runtime_invoke(getter, instance, nullptr, &exception);

            if (exception || !result)
            {
                if (exception) mono_print_unhandled_exception(exception);
                std::cerr << "[VisualScript] Failed to get property '" << propertyName << "' from '" << inst.ClassName << "'\n";
                continue;
            }

            outValue = *(bool*)mono_object_unbox(result);
            return true;
        }

        if (!foundMatching)
        {
            std::cerr << "[VisualScript] No script instance with class name '" << className << "' found on entity " << entity << "\n";
        }
        return false;
    }
    void ResetGraphExecution(Component::VisualScriptComponent& comp)
    {
        comp.currentNodeId = 0;
        comp.waitTimer = 0.0f;
    }
}
void InvalidateVisualScriptCache(const std::string& path)
{
    std::error_code ec;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);

    if (ec)
    {
        normalized = std::filesystem::path(path).lexically_normal();
    }

    g_visualScriptCache.erase(normalized.string());
    g_visualScriptCache.erase(path);
}
void ExecuteVisualScript(ECS& ecs, Entity e, Component::VisualScriptComponent& comp, float dt)
{
    std::cout << "[VisualScript] Current node id = " << comp.currentNodeId << "\n";
    if (!comp.graph)
        return;

    UI::VisualScriptingGraph& graph = *comp.graph;

    if (comp.currentNodeId == 0)
    {
        UI::VSNode* start = FindStartNode(graph);
        if (!start)
            return;

        comp.currentNodeId = start->id;
    }

    int safety = 0;

    while (comp.currentNodeId != 0 && safety < 100)
    {
        ++safety;

        UI::VSNode* current = FindNodeById(graph, comp.currentNodeId);
        if (!current)
        {
            ResetGraphExecution(comp);
            return;
        }

        switch (current->type)
        {
        case UI::VSNodeType::Start:
        {
            std::cout << "[VisualScript] Start node hit\n";

            for (const auto& link : graph.links)
            {
                if (link.fromNode == current->id)
                {
                    std::cout << "[VisualScript] Start outgoing link: fromNode=" << link.fromNode
                        << " fromPin=" << link.fromPin
                        << " toNode=" << link.toNode
                        << " toPin=" << link.toPin << "\n";
                }
            }

            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            std::cout << "[VisualScript] Start next = " << (next ? next->id : 0) << "\n";

            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::Move:
        {
            if (auto* transform = ecs.GetComponent<Component::TransformComponent>(e)) {
                transform->position.x += 10.0f;
            }

            std::cout << "[VisualScript] Move node executed on entity " << e << "\n";

            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::Attack:
        {
            std::cout << "[VisualScript] Attack node executed on entity " << e << "\n";

            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::Wait:
        {
            if (comp.waitTimer <= 0.0f)
            {
                comp.waitTimer = current->waitTime;
            }

            comp.waitTimer -= dt;

            if (comp.waitTimer > 0.0f)
            {
                return;
            }

            comp.waitTimer = 0.0f;

            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::Branch:
        {
            bool condition = false;
            bool gotCondition = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == current->id)
                {
                    std::cout << "[VisualScript] Branch incoming link: fromNode=" << link.fromNode
                        << " fromPin=" << link.fromPin
                        << " toNode=" << link.toNode
                        << " toPin=" << link.toPin << "\n";

                    gotCondition = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, condition);
                    if (gotCondition)
                    {
                        std::cout << "[VisualScript] Branch got condition from node "
                            << link.fromNode << " value=" << (condition ? "true" : "false") << "\n";
                        break;
                    }
                }
            }

            if (!gotCondition)
            {
                std::cout << "[VisualScript] Branch did NOT get connected bool, using fallback IsGrounded\n";

                if (!GetMonoBoolProperty(ecs, e, "PlayerMovement", "IsGrounded", condition))
                {
                    std::cout << "[VisualScript] Failed to read IsGrounded too\n";
                }
            }

            UI::VSNode* next = FindNextNode(graph, current->id, condition ? 1 : 2);
            if (!next)
                next = FindNextNode(graph, current->id, 0);

            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::ScriptCall:
        {
            bool ok = ExecuteMonoInstanceMethod(ecs, e, current->scriptName, current->functionName);

            std::cout << "[VisualScript] ScriptCall on entity " << e
                << " script=" << current->scriptName
                << " function=" << current->functionName
                << " result=" << (ok ? "success" : "failed") << "\n";

            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        default:
            ResetGraphExecution(comp);
            return;
        }

        if (comp.currentNodeId == 0)
        {
            ResetGraphExecution(comp);
            return;
        }
    }
}

void UpdateVisualScripts(ECS& ecs, float dt)
{
    auto& arr = ecs.GetComponentArray<Component::VisualScriptComponent>();

    std::cout << "[VisualScript] UpdateVisualScripts: components=" << arr.indexToEntity.size() << "\n";

    for (CrashoutsEngine::Entity e : arr.indexToEntity)
    {
        std::cout << "[VisualScript] Entity iterate: " << e << "\n";

        if (!ecs.HasComponent<Component::VisualScriptComponent>(e)) {
            std::cout << "[VisualScript]  - no component present (skipped)\n";
            continue;
        }

        auto* comp = ecs.GetComponent<Component::VisualScriptComponent>(e);
        if (!comp) {
            std::cout << "[VisualScript]  - component pointer null (skipped)\n";
            continue;
        }

        std::cout << "[VisualScript]  - enabled=" << comp->enabled << " graphPath=\"" << comp->graphPath
                  << "\" graphLoaded=" << comp->graphLoaded << " currentNodeId=" << comp->currentNodeId << "\n";

        if (!comp->enabled) {
            std::cout << "[VisualScript]  - disabled (skipped)\n";
            continue;
        }

        if (!comp->graphLoaded && !comp->graphPath.empty())
        {
            std::cout << "[VisualScript]  - loading graph: " << comp->graphPath << "\n";
            comp->graph = GetOrLoadGraph(comp->graphPath);
            if (comp->graph) {
                UI::VSNode* start = FindStartNode(*comp->graph);
                comp->currentNodeId = start ? start->id : 0;
                comp->waitTimer = 0.0f;
                comp->graphLoaded = true;
                std::cout << "[VisualScript]  - graph loaded, start node id=" << comp->currentNodeId << "\n";
            }
            else {
                std::cerr << "[VisualScript] Failed to load graph '" << comp->graphPath
                    << "' for entity " << e << "\n";
                comp->graphLoaded = false;
                continue;
            }
        }

        if (comp->graphLoaded && comp->graph)
        {
            ExecuteVisualScript(ecs, e, *comp, dt);
        }
    }
}

std::shared_ptr<UI::VisualScriptingGraph> GetOrLoadGraph(const std::string& path)
{
    std::error_code ec;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);

    if (ec)
    {
        normalized = std::filesystem::path(path).lexically_normal();
    }

    const std::string key = normalized.string();

    auto it = g_visualScriptCache.find(key);
    if (it != g_visualScriptCache.end())
        return it->second;

    auto graphPtr = std::make_shared<UI::VisualScriptingGraph>();
    if (!UI::LoadVisualScriptingGraph(path, *graphPtr))
        return nullptr;

    g_visualScriptCache[key] = graphPtr;
    return graphPtr;
}