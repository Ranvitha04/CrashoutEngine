#include "debugmenu.h"
#include "visualscriptsystem.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <memory>

namespace UI {

    VisualScriptingGraph& GetVisualScriptingGraph()
    {
        UIState* s = GetUIState();
        if (!s) {
            static VisualScriptingGraph empty;
            return empty;
        }
        return s->visualGraph;
    }

    void ClearVisualScriptingGraph()
    {
        UIState* s = GetUIState();
        if (!s) return;
        s->visualGraph.nodes.clear();
        s->visualGraph.links.clear();
        s->visualGraph.nextNodeId = 1;
    }

    bool SaveVisualScriptingGraph(const std::string& path, const VisualScriptingGraph& graph)
    {
        using namespace rapidjson;

        Document d;
        d.SetObject();
        Document::AllocatorType& a = d.GetAllocator();

        Value nodesArr(kArrayType);
        for (const auto& n : graph.nodes)
        {
            Value nodeObj(kObjectType);

            nodeObj.AddMember("id", n.id, a);
            nodeObj.AddMember("type", static_cast<int>(n.type), a);
            nodeObj.AddMember("title", Value(n.title.c_str(), a), a);

            Value posObj(kObjectType);
            posObj.AddMember("x", n.pos.x, a);
            posObj.AddMember("y", n.pos.y, a);
            nodeObj.AddMember("pos", posObj, a);

            Value sizeObj(kObjectType);
            sizeObj.AddMember("x", n.size.x, a);
            sizeObj.AddMember("y", n.size.y, a);
            nodeObj.AddMember("size", sizeObj, a);

            Value inputsArr(kArrayType);
            for (const auto& pin : n.inputs)
            {
                Value pinObj(kObjectType);
                pinObj.AddMember("id", pin.id, a);
                pinObj.AddMember("name", Value(pin.name.c_str(), a), a);
                pinObj.AddMember("kind", static_cast<int>(pin.kind), a);
                pinObj.AddMember("type", static_cast<int>(pin.type), a);
                inputsArr.PushBack(pinObj, a);
            }
            nodeObj.AddMember("inputs", inputsArr, a);

            Value outputsArr(kArrayType);
            for (const auto& pin : n.outputs)
            {
                Value pinObj(kObjectType);
                pinObj.AddMember("id", pin.id, a);
                pinObj.AddMember("name", Value(pin.name.c_str(), a), a);
                pinObj.AddMember("kind", static_cast<int>(pin.kind), a);
                pinObj.AddMember("type", static_cast<int>(pin.type), a);
                outputsArr.PushBack(pinObj, a);
            }
            nodeObj.AddMember("outputs", outputsArr, a);

            nodeObj.AddMember("scriptName", Value(n.scriptName.c_str(), a), a);
            nodeObj.AddMember("functionName", Value(n.functionName.c_str(), a), a);
            nodeObj.AddMember("waitTime", n.waitTime, a);
            nodeObj.AddMember("boolValue", n.boolValue, a);
            nodeObj.AddMember("intValue", n.intValue, a);
            nodeObj.AddMember("floatValue", n.floatValue, a);
            nodeObj.AddMember("variableName", Value(n.variableName.c_str(), a), a);

            nodesArr.PushBack(nodeObj, a);
        }
        d.AddMember("nodes", nodesArr, a);

        Value linksArr(kArrayType);
        for (const auto& l : graph.links)
        {
            Value linkObj(kObjectType);
            linkObj.AddMember("fromNode", l.fromNode, a);
            linkObj.AddMember("fromPin", l.fromPin, a);
            linkObj.AddMember("toNode", l.toNode, a);
            linkObj.AddMember("toPin", l.toPin, a);
            linksArr.PushBack(linkObj, a);
        }
        d.AddMember("links", linksArr, a);

        d.AddMember("nextNodeId", graph.nextNodeId, a);
        d.AddMember("nextPinId", graph.nextPinId, a);

        std::filesystem::path filePath(path);
        if (!filePath.parent_path().empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(filePath.parent_path(), ec);
            if (ec)
            {
                std::cerr << "[VisualScript] Failed to create directory: "
                    << filePath.parent_path() << " (" << ec.message() << ")\n";
                return false;
            }
        }

        std::ofstream ofs(path);
        if (!ofs.is_open())
        {
            std::cerr << "[VisualScript] Failed to open file for saving: " << path << "\n";
            return false;
        }

        OStreamWrapper osw(ofs);
        PrettyWriter<OStreamWrapper> writer(osw);
        d.Accept(writer);
        ofs.close();

        InvalidateVisualScriptCache(path);
        return true;
    }

    bool LoadVisualScriptingGraph(const std::string& path, VisualScriptingGraph& graph)
    {
        using namespace rapidjson;

        std::ifstream ifs(path);
        if (!ifs.is_open())
        {
            std::cerr << "[VisualScript] Failed to open file for loading: " << path << "\n";
            return false;
        }

        IStreamWrapper isw(ifs);
        Document d;
        d.ParseStream(isw);

        if (d.HasParseError() || !d.IsObject())
        {
            std::cerr << "[VisualScript] Invalid JSON in file: " << path << "\n";
            return false;
        }

        graph.nodes.clear();
        graph.links.clear();
        graph.nextNodeId = 1;
        graph.nextPinId = 1;

        if (d.HasMember("nextNodeId") && d["nextNodeId"].IsInt())
            graph.nextNodeId = d["nextNodeId"].GetInt();

        if (d.HasMember("nextPinId") && d["nextPinId"].IsInt())
            graph.nextPinId = d["nextPinId"].GetInt();

        if (d.HasMember("nodes") && d["nodes"].IsArray())
        {
            const auto& nodesArr = d["nodes"];
            for (rapidjson::SizeType i = 0; i < nodesArr.Size(); ++i)
            {
                const auto& nodeObj = nodesArr[i];
                if (!nodeObj.IsObject())
                    continue;

                VSNode node{};

                if (nodeObj.HasMember("id") && nodeObj["id"].IsInt())
                    node.id = nodeObj["id"].GetInt();

                if (nodeObj.HasMember("type") && nodeObj["type"].IsInt())
                    node.type = static_cast<VSNodeType>(nodeObj["type"].GetInt());

                if (nodeObj.HasMember("title") && nodeObj["title"].IsString())
                    node.title = nodeObj["title"].GetString();

                if (nodeObj.HasMember("pos") && nodeObj["pos"].IsObject())
                {
                    const auto& posObj = nodeObj["pos"];
                    if (posObj.HasMember("x") && posObj["x"].IsNumber())
                        node.pos.x = posObj["x"].GetFloat();
                    if (posObj.HasMember("y") && posObj["y"].IsNumber())
                        node.pos.y = posObj["y"].GetFloat();
                }

                if (nodeObj.HasMember("size") && nodeObj["size"].IsObject())
                {
                    const auto& sizeObj = nodeObj["size"];
                    if (sizeObj.HasMember("x") && sizeObj["x"].IsNumber())
                        node.size.x = sizeObj["x"].GetFloat();
                    if (sizeObj.HasMember("y") && sizeObj["y"].IsNumber())
                        node.size.y = sizeObj["y"].GetFloat();
                }

                node.inputs.clear();
                if (nodeObj.HasMember("inputs") && nodeObj["inputs"].IsArray())
                {
                    const auto& inputsArr = nodeObj["inputs"];
                    for (rapidjson::SizeType j = 0; j < inputsArr.Size(); ++j)
                    {
                        const auto& pinObj = inputsArr[j];
                        if (!pinObj.IsObject())
                            continue;

                        VSPin pin{};

                        if (pinObj.HasMember("id") && pinObj["id"].IsInt())
                            pin.id = pinObj["id"].GetInt();

                        if (pinObj.HasMember("name") && pinObj["name"].IsString())
                            pin.name = pinObj["name"].GetString();

                        if (pinObj.HasMember("kind") && pinObj["kind"].IsInt())
                            pin.kind = static_cast<VSPinKind>(pinObj["kind"].GetInt());

                        if (pinObj.HasMember("type") && pinObj["type"].IsInt())
                            pin.type = static_cast<VSDataType>(pinObj["type"].GetInt());

                        node.inputs.push_back(pin);
                    }
                }

                node.outputs.clear();
                if (nodeObj.HasMember("outputs") && nodeObj["outputs"].IsArray())
                {
                    const auto& outputsArr = nodeObj["outputs"];
                    for (rapidjson::SizeType j = 0; j < outputsArr.Size(); ++j)
                    {
                        const auto& pinObj = outputsArr[j];
                        if (!pinObj.IsObject())
                            continue;

                        VSPin pin{};

                        if (pinObj.HasMember("id") && pinObj["id"].IsInt())
                            pin.id = pinObj["id"].GetInt();

                        if (pinObj.HasMember("name") && pinObj["name"].IsString())
                            pin.name = pinObj["name"].GetString();

                        if (pinObj.HasMember("kind") && pinObj["kind"].IsInt())
                            pin.kind = static_cast<VSPinKind>(pinObj["kind"].GetInt());

                        if (pinObj.HasMember("type") && pinObj["type"].IsInt())
                            pin.type = static_cast<VSDataType>(pinObj["type"].GetInt());

                        node.outputs.push_back(pin);
                    }
                }

                if (nodeObj.HasMember("scriptName") && nodeObj["scriptName"].IsString())
                    node.scriptName = nodeObj["scriptName"].GetString();

                if (nodeObj.HasMember("functionName") && nodeObj["functionName"].IsString())
                    node.functionName = nodeObj["functionName"].GetString();

                if (nodeObj.HasMember("waitTime") && nodeObj["waitTime"].IsNumber())
                    node.waitTime = nodeObj["waitTime"].GetFloat();
                if (nodeObj.HasMember("boolValue") && nodeObj["boolValue"].IsBool())
                    node.boolValue = nodeObj["boolValue"].GetBool();

                if (nodeObj.HasMember("intValue") && nodeObj["intValue"].IsInt())
                    node.intValue = nodeObj["intValue"].GetInt();

                if (nodeObj.HasMember("floatValue") && nodeObj["floatValue"].IsNumber())
                    node.floatValue = nodeObj["floatValue"].GetFloat();

                if (nodeObj.HasMember("variableName") && nodeObj["variableName"].IsString())
                    node.variableName = nodeObj["variableName"].GetString();

                graph.nodes.push_back(node);
            }
        }

        if (d.HasMember("links") && d["links"].IsArray())
        {
            const auto& linksArr = d["links"];
            for (rapidjson::SizeType i = 0; i < linksArr.Size(); ++i)
            {
                const auto& linkObj = linksArr[i];
                if (!linkObj.IsObject())
                    continue;

                VSLink link{};

                if (linkObj.HasMember("fromNode") && linkObj["fromNode"].IsInt())
                    link.fromNode = linkObj["fromNode"].GetInt();

                if (linkObj.HasMember("fromPin") && linkObj["fromPin"].IsInt())
                    link.fromPin = linkObj["fromPin"].GetInt();

                if (linkObj.HasMember("toNode") && linkObj["toNode"].IsInt())
                    link.toNode = linkObj["toNode"].GetInt();

                if (linkObj.HasMember("toPin") && linkObj["toPin"].IsInt())
                    link.toPin = linkObj["toPin"].GetInt();

                graph.links.push_back(link);
            }
        }

        if (graph.nextNodeId <= 0)
        {
            int maxNodeId = 0;
            for (const auto& node : graph.nodes)
            {
                if (node.id > maxNodeId)
                    maxNodeId = node.id;
            }
            graph.nextNodeId = maxNodeId + 1;
        }

        if (graph.nextPinId <= 0)
        {
            int maxPinId = 0;
            for (const auto& node : graph.nodes)
            {
                for (const auto& pin : node.inputs)
                {
                    if (pin.id > maxPinId)
                        maxPinId = pin.id;
                }
                for (const auto& pin : node.outputs)
                {
                    if (pin.id > maxPinId)
                        maxPinId = pin.id;
                }
            }
            graph.nextPinId = maxPinId + 1;
        }

        return true;
    }

} // namespace UI