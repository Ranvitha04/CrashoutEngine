/*!
@file       visualscriptsystem.cpp
@author     Afiq
@date       13/03/2026
@brief      Runtime evaluation and update logic for the visual scripting system.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

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
/*!
@brief Get a cached visual scripting graph or load it from disk.
@param path Graph asset path.
@return Shared pointer to the graph, or nullptr on failure.
*/
std::shared_ptr<UI::VisualScriptingGraph> GetOrLoadGraph(const std::string& path);

namespace
 {   
    std::unordered_map<std::string, std::shared_ptr<UI::VisualScriptingGraph>> g_visualScriptCache;

    /*!
    @brief Find a node in the graph by its id.
    @param graph Graph to search.
    @param id Node id.
    @return Pointer to the node if found, otherwise nullptr.
    */
    UI::VSNode* FindNodeById(UI::VisualScriptingGraph& graph, int id)
    {
        for (auto& node : graph.nodes) {
            if (node.id == id)
                return &node;
        }
        return nullptr;
    }

    /*!
    @brief Evaluate a float-producing node and output its value.
    @param ecs ECS instance to query/modify.
    @param e Entity being evaluated.
    @param comp Visual script component containing variable state.
    @param graph Graph containing nodes and links.
    @param nodeId Node id to evaluate.
    @param outValue Output float value on success.
    @return True if evaluation succeeded, otherwise false.
    */
    static bool EvaluateFloatNode(CrashoutsEngine::ECS& ecs,
        CrashoutsEngine::Entity e,
        Component::VisualScriptComponent& comp,
        UI::VisualScriptingGraph& graph,
        int nodeId,
        float& outValue)
    {
        UI::VSNode* node = FindNodeById(graph, nodeId);
        if (!node) return false;

        switch (node->type)
        {
        case UI::VSNodeType::FloatConstant:
            outValue = node->floatValue;
            return true;

        case UI::VSNodeType::GetVariable:
        {
            auto it = comp.floatVariables.find(node->variableName);
            if (it == comp.floatVariables.end())
                return false;

            outValue = it->second;
            return true;
        }

        case UI::VSNodeType::Add:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a + b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Subtract:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a - b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Multiply:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a * b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Divide:
        {
            float a = 0.0f, b = 1.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB && b != 0.0f) {
                outValue = a / b;
                return true;
            }
            return false;
        }

        default:
            return false;
        }
    }

    /*!
    @brief Evaluate a bool-producing node and output its value.
    @param ecs ECS instance to query/modify.
    @param e Entity being evaluated.
    @param comp Visual script component containing variable state.
    @param graph Graph containing nodes and links.
    @param nodeId Node id to evaluate.
    @param outValue Output bool value on success.
    @return True if evaluation succeeded, otherwise false.
    */
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

        case UI::VSNodeType::And:
        {
            bool a = false, b = false;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a && b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Or:
        {
            bool a = false, b = false;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a || b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Not:
        {
            bool a = false;
            bool gotA = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, a);
            }

            if (gotA) {
                outValue = !a;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Equal:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = (a == b);
                return true;
            }
            return false;
        }

        case UI::VSNodeType::NotEqual:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = (a != b);
                return true;
            }
            return false;
        }

        case UI::VSNodeType::GreaterThan:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = (a > b);
                return true;
            }
            return false;
        }

        case UI::VSNodeType::LessThan:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = (a < b);
                return true;
            }
            return false;
        }

        case UI::VSNodeType::GreaterEqual:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = (a >= b);
                return true;
            }
            return false;
        }

        case UI::VSNodeType::LessEqual:
        {
            float a = 0.0f, b = 0.0f;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateFloatNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = (a <= b);
                return true;
            }
            return false;
        }

        default:
            return false;
        }
    }
    
    /*!
    @brief Evaluate an int-producing node and output its value.
    @param ecs ECS instance to query/modify.
    @param e Entity being evaluated.
    @param comp Visual script component containing variable state.
    @param graph Graph containing nodes and links.
    @param nodeId Node id to evaluate.
    @param outValue Output int value on success.
    @return True if evaluation succeeded, otherwise false.
    */
    static bool EvaluateIntNode(CrashoutsEngine::ECS& ecs,
        CrashoutsEngine::Entity e,
        Component::VisualScriptComponent& comp,
        UI::VisualScriptingGraph& graph,
        int nodeId,
        int& outValue)
    {
        UI::VSNode* node = FindNodeById(graph, nodeId);
        if (!node) return false;

        switch (node->type)
        {
        case UI::VSNodeType::IntConstant:
            outValue = node->intValue;
            return true;

        case UI::VSNodeType::Add:
        {
            int a = 0, b = 0;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a + b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Subtract:
        {
            int a = 0, b = 0;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a - b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Multiply:
        {
            int a = 0, b = 0;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB) {
                outValue = a * b;
                return true;
            }
            return false;
        }

        case UI::VSNodeType::Divide:
        {
            int a = 0, b = 1;
            bool gotA = false, gotB = false;

            for (const auto& link : graph.links)
            {
                if (link.toNode == node->id && link.toPin == 0)
                    gotA = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, a);
                else if (link.toNode == node->id && link.toPin == 1)
                    gotB = EvaluateIntNode(ecs, e, comp, graph, link.fromNode, b);
            }

            if (gotA && gotB && b != 0) {
                outValue = a / b;
                return true;
            }
            return false;
        }

        default:
            return false;
        }
    }

    /*!
    @brief Find the start node in a visual scripting graph.
    @param graph Graph to search.
    @return Pointer to the start node if found, otherwise nullptr.
    */
    UI::VSNode* FindStartNode(UI::VisualScriptingGraph& graph)
    {
        for (auto& node : graph.nodes) {
            if (node.type == UI::VSNodeType::Start)
                return &node;
        }
        return nullptr;
    }

    /*!
    @brief Find the next node connected from a given node output pin.
    @param graph Graph containing links.
    @param currentNodeId Source node id.
    @param outputPin Output pin index to follow.
    @return Pointer to the next node if a link exists, otherwise nullptr.
    */
    UI::VSNode* FindNextNode(UI::VisualScriptingGraph& graph, int currentNodeId, int outputPin = 0)
    {
        for (auto& link : graph.links) {
            if (link.fromNode == currentNodeId && link.fromPin == outputPin) {
                return FindNodeById(graph, link.toNode);
            }
        }
        return nullptr;
    }

    /*!
    @brief Invoke a no-argument Mono instance method on a script component attached to an entity.
    @param ecs ECS instance to query for script components.
    @param entity Entity that owns the script component.
    @param className Script class name to match.
    @param methodName Method name to invoke.
    @return True if the method was invoked successfully, otherwise false.
    */
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

    /*!
    @brief Read a boolean property from a Mono script instance attached to an entity.
    @param ecs ECS instance to query for script components.
    @param entity Entity that owns the script component.
    @param className Script class name to match.
    @param propertyName Property name to read.
    @param outValue Output bool value on success.
    @return True if the property was read successfully, otherwise false.
    */
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
    /*!
    @brief Reset the per-entity execution state for a visual script graph.
    @param comp Visual script component to reset.
    */
    void ResetGraphExecution(Component::VisualScriptComponent& comp)
    {
        comp.currentNodeId = 0;
        comp.waitTimer = 0.0f;
    }
}

/*!
@brief Invalidate cached visual script graphs for a given asset path.
@param path Graph asset path to invalidate.
*/
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

/*!
@brief Execute a single visual script graph for an entity.
@param ecs ECS instance to query/modify.
@param e Entity to execute the script for.
@param comp Visual script component containing graph state and variables.
@param dt Delta time in seconds.
*/
void ExecuteVisualScript(ECS& ecs, Entity e, Component::VisualScriptComponent& comp, float dt)
{
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
            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::Move:
        {
            if (auto* transform = ecs.GetComponent<Component::TransformComponent>(e)) {
                transform->position.x += 10.0f;
            }

            UI::VSNode* next = FindNextNode(graph, current->id, 0);
            comp.currentNodeId = next ? next->id : 0;
            break;
        }

        case UI::VSNodeType::Attack:
        {
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
                    gotCondition = EvaluateBoolNode(ecs, e, comp, graph, link.fromNode, condition);
                    if (gotCondition) break;
                }
            }

            if (!gotCondition)
            {
                if (!GetMonoBoolProperty(ecs, e, "PlayerMovement", "IsGrounded", condition))
                {
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
            //bool ok = ExecuteMonoInstanceMethod(ecs, e, current->scriptName, current->functionName);

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

/*!
@brief Update all visual scripts for entities that have a VisualScriptComponent.
@param ecs ECS instance to query/modify.
@param dt Delta time in seconds.
*/
void UpdateVisualScripts(ECS& ecs, float dt)
{
    auto& arr = ecs.GetComponentArray<Component::VisualScriptComponent>();

    for (CrashoutsEngine::Entity e : arr.indexToEntity)
    {
        if (!ecs.HasComponent<Component::VisualScriptComponent>(e)) {
            continue;
        }

        auto* comp = ecs.GetComponent<Component::VisualScriptComponent>(e);
        if (!comp) {
            continue;
        }

        if (!comp->enabled) {
            continue;
        }

        if (!comp->graphLoaded && !comp->graphPath.empty())
        {
            comp->graph = GetOrLoadGraph(comp->graphPath);
            if (comp->graph) {
                UI::VSNode* start = FindStartNode(*comp->graph);
                comp->currentNodeId = start ? start->id : 0;
                comp->waitTimer = 0.0f;
                comp->graphLoaded = true;
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

/*!
@brief Get a cached visual scripting graph or load it from disk.
@param path Graph asset path.
@return Shared pointer to the graph, or nullptr on failure.
*/
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
