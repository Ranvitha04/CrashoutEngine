#pragma once

#include "undomanager.h"
#include "debugmenu.h"
#include <vector>

namespace UI
{
    class CreateVSNodeCommand : public EditorCommand
    {
    public:
        CreateVSNodeCommand(VisualScriptingGraph& graph, const VSNode& node);

        void Execute() override;
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        VSNode m_node;
    };

    class DeleteVSNodeCommand : public EditorCommand
    {
    public:
        DeleteVSNodeCommand(VisualScriptingGraph& graph, int nodeId);

        void Execute() override;
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        VSNode m_node;
        std::vector<VSLink> m_removedLinks;
        bool m_valid = false;
    };

    class CreateVSLinkCommand : public EditorCommand
    {
    public:
        CreateVSLinkCommand(VisualScriptingGraph& graph, const VSLink& link);

        void Execute() override;
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        VSLink m_link;
    };

    class MoveVSNodeCommand : public EditorCommand
    {
    public:
        MoveVSNodeCommand(VisualScriptingGraph& graph, int nodeId, ImVec2 oldPos, ImVec2 newPos);

        void Execute() override;
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        int m_nodeId;
        ImVec2 m_oldPos;
        ImVec2 m_newPos;
    };
    class EditVSNodeCommand : public EditorCommand
    {
    public:
        EditVSNodeCommand(VisualScriptingGraph& graph, int nodeId, const VSNode& oldNode, const VSNode& newNode);

        void Execute() override;
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        int m_nodeId;
        VSNode m_oldNode;
        VSNode m_newNode;
    };
}