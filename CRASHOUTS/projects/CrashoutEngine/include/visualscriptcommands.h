/*!
@file       visualscriptcommands.h
@author     Afiq
@date       13/03/2026
@brief      Editor commands for visual scripting graph modifications (undo/redo).

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

#pragma once

#include "undomanager.h"
#include "debugmenu.h"
#include <vector>

namespace UI
{
    /*!
    @class CreateVSNodeCommand
    @brief Adds a node to a visual scripting graph.
    */
    /** @brief CreateVSNodeCommand */
    class CreateVSNodeCommand : public EditorCommand
    {
    public:
        /*!
        @brief Construct a create-node command.
        @param graph Graph to modify.
        @param node Node to add.
        */
        CreateVSNodeCommand(VisualScriptingGraph& graph, const VSNode& node);

        /*!
        @brief Execute the command by adding the node.
        */
        void Execute() override;

        /*!
        @brief Undo the command by removing the node.
        */
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        VSNode m_node;
    };

    /*!
    @class DeleteVSNodeCommand
    @brief Removes a node from a visual scripting graph, tracking removed links for undo.
    */
    /** @brief DeleteVSNodeCommand */
    class DeleteVSNodeCommand : public EditorCommand
    {
    public:
        /*!
        @brief Construct a delete-node command.
        @param graph Graph to modify.
        @param nodeId Id of the node to remove.
        */
        DeleteVSNodeCommand(VisualScriptingGraph& graph, int nodeId);

        /*!
        @brief Execute the command by removing the node (and any attached links).
        */
        void Execute() override;

        /*!
        @brief Undo the command by restoring the node and previously removed links.
        */
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        VSNode m_node;
        std::vector<VSLink> m_removedLinks;
        bool m_valid = false;
    };

    /*!
    @class CreateVSLinkCommand
    @brief Adds a link/connection to a visual scripting graph.
    */
    /** @brief CreateVSLinkCommand */
    class CreateVSLinkCommand : public EditorCommand
    {
    public:
        /*!
        @brief Construct a create-link command.
        @param graph Graph to modify.
        @param link Link to add.
        */
        CreateVSLinkCommand(VisualScriptingGraph& graph, const VSLink& link);

        /*!
        @brief Execute the command by adding the link.
        */
        void Execute() override;

        /*!
        @brief Undo the command by removing the link.
        */
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        VSLink m_link;
    };

    /*!
    @class MoveVSNodeCommand
    @brief Moves a node to a new position in the visual scripting editor.
    */
    /** @brief MoveVSNodeCommand */
    class MoveVSNodeCommand : public EditorCommand
    {
    public:
        /*!
        @brief Construct a move-node command.
        @param graph Graph to modify.
        @param nodeId Id of the node to move.
        @param oldPos Previous position.
        @param newPos New position.
        */
        MoveVSNodeCommand(VisualScriptingGraph& graph, int nodeId, ImVec2 oldPos, ImVec2 newPos);

        /*!
        @brief Execute the command by applying the new position.
        */
        void Execute() override;

        /*!
        @brief Undo the command by restoring the previous position.
        */
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        int m_nodeId;
        ImVec2 m_oldPos;
        ImVec2 m_newPos;
    };

    /*!
    @class EditVSNodeCommand
    @brief Replaces a node's data, enabling undo/redo for node edits.
    */
    /** @brief EditVSNodeCommand */
    class EditVSNodeCommand : public EditorCommand
    {
    public:
        /*!
        @brief Construct an edit-node command.
        @param graph Graph to modify.
        @param nodeId Id of the node to edit.
        @param oldNode Previous node state.
        @param newNode New node state.
        */
        EditVSNodeCommand(VisualScriptingGraph& graph, int nodeId, const VSNode& oldNode, const VSNode& newNode);

        /*!
        @brief Execute the command by applying the new node state.
        */
        void Execute() override;

        /*!
        @brief Undo the command by restoring the previous node state.
        */
        void Undo() override;

    private:
        VisualScriptingGraph& m_graph;
        int m_nodeId;
        VSNode m_oldNode;
        VSNode m_newNode;
    };
}
