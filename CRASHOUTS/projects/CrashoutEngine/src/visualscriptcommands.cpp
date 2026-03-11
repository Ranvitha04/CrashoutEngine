#include "visualscriptcommands.h"
#include <algorithm>

namespace UI
{
    static VSNode* FindVSNodeById(VisualScriptingGraph& graph, int id)
    {
        for (auto& node : graph.nodes)
        {
            if (node.id == id)
                return &node;
        }
        return nullptr;
    }

    static bool VSLinkEquals(const VSLink& a, const VSLink& b)
    {
        return a.fromNode == b.fromNode &&
            a.fromPin == b.fromPin &&
            a.toNode == b.toNode &&
            a.toPin == b.toPin;
    }

    CreateVSNodeCommand::CreateVSNodeCommand(VisualScriptingGraph& graph, const VSNode& node)
        : m_graph(graph), m_node(node)
    {
    }

    void CreateVSNodeCommand::Execute()
    {
        auto it = std::find_if(m_graph.nodes.begin(), m_graph.nodes.end(),
            [&](const VSNode& n) { return n.id == m_node.id; });

        if (it == m_graph.nodes.end())
            m_graph.nodes.push_back(m_node);

        if (m_graph.nextNodeId <= m_node.id)
            m_graph.nextNodeId = m_node.id + 1;
    }

    void CreateVSNodeCommand::Undo()
    {
        m_graph.links.erase(
            std::remove_if(m_graph.links.begin(), m_graph.links.end(),
                [&](const VSLink& link)
                {
                    return link.fromNode == m_node.id || link.toNode == m_node.id;
                }),
            m_graph.links.end());

        m_graph.nodes.erase(
            std::remove_if(m_graph.nodes.begin(), m_graph.nodes.end(),
                [&](const VSNode& n) { return n.id == m_node.id; }),
            m_graph.nodes.end());
    }

    DeleteVSNodeCommand::DeleteVSNodeCommand(VisualScriptingGraph& graph, int nodeId)
        : m_graph(graph)
    {
        VSNode* node = FindVSNodeById(graph, nodeId);
        if (!node)
            return;

        m_node = *node;
        m_valid = true;

        for (const auto& link : graph.links)
        {
            if (link.fromNode == nodeId || link.toNode == nodeId)
                m_removedLinks.push_back(link);
        }
    }

    void DeleteVSNodeCommand::Execute()
    {
        if (!m_valid)
            return;

        m_graph.links.erase(
            std::remove_if(m_graph.links.begin(), m_graph.links.end(),
                [&](const VSLink& link)
                {
                    return link.fromNode == m_node.id || link.toNode == m_node.id;
                }),
            m_graph.links.end());

        m_graph.nodes.erase(
            std::remove_if(m_graph.nodes.begin(), m_graph.nodes.end(),
                [&](const VSNode& n) { return n.id == m_node.id; }),
            m_graph.nodes.end());
    }

    void DeleteVSNodeCommand::Undo()
    {
        if (!m_valid)
            return;

        auto it = std::find_if(m_graph.nodes.begin(), m_graph.nodes.end(),
            [&](const VSNode& n) { return n.id == m_node.id; });

        if (it == m_graph.nodes.end())
            m_graph.nodes.push_back(m_node);

        for (const auto& link : m_removedLinks)
        {
            auto lit = std::find_if(m_graph.links.begin(), m_graph.links.end(),
                [&](const VSLink& existing) { return VSLinkEquals(existing, link); });

            if (lit == m_graph.links.end())
                m_graph.links.push_back(link);
        }

        if (m_graph.nextNodeId <= m_node.id)
            m_graph.nextNodeId = m_node.id + 1;
    }

    CreateVSLinkCommand::CreateVSLinkCommand(VisualScriptingGraph& graph, const VSLink& link)
        : m_graph(graph), m_link(link)
    {
    }

    void CreateVSLinkCommand::Execute()
    {
        auto it = std::find_if(m_graph.links.begin(), m_graph.links.end(),
            [&](const VSLink& existing) { return VSLinkEquals(existing, m_link); });

        if (it == m_graph.links.end())
            m_graph.links.push_back(m_link);
    }

    void CreateVSLinkCommand::Undo()
    {
        m_graph.links.erase(
            std::remove_if(m_graph.links.begin(), m_graph.links.end(),
                [&](const VSLink& existing) { return VSLinkEquals(existing, m_link); }),
            m_graph.links.end());
    }

    MoveVSNodeCommand::MoveVSNodeCommand(VisualScriptingGraph& graph, int nodeId, ImVec2 oldPos, ImVec2 newPos)
        : m_graph(graph), m_nodeId(nodeId), m_oldPos(oldPos), m_newPos(newPos)
    {
    }

    void MoveVSNodeCommand::Execute()
    {
        VSNode* node = FindVSNodeById(m_graph, m_nodeId);
        if (node)
            node->pos = m_newPos;
    }

    void MoveVSNodeCommand::Undo()
    {
        VSNode* node = FindVSNodeById(m_graph, m_nodeId);
        if (node)
            node->pos = m_oldPos;
    }
    EditVSNodeCommand::EditVSNodeCommand(VisualScriptingGraph& graph, int nodeId, const VSNode& oldNode, const VSNode& newNode)
        : m_graph(graph), m_nodeId(nodeId), m_oldNode(oldNode), m_newNode(newNode)
    {
	}

    void EditVSNodeCommand::Execute()
    {
        VSNode* node = FindVSNodeById(m_graph, m_nodeId);
        if (node)
            *node = m_newNode;
    }

    void EditVSNodeCommand::Undo()
    {
        VSNode* node = FindVSNodeById(m_graph, m_nodeId);
        if (node)
            *node = m_oldNode;
    }
}