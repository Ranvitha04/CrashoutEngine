/*!
@file        UndoManager.h
@author      Shahrin Idzuan
@author      m.biniswandi
@author      Afiq
@brief       Singleton manager for undo/redo command history (deque version)
@date        08/11/2025
All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.
*/

#pragma once
#include <deque>
#include <memory>

/*!
@brief Base interface for all editor commands (actions that can be undone/redone)
*/
/** @brief EditorCommand */
class EditorCommand
{
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;  //!< Applies the command
    virtual void Undo() = 0;     //!< Reverts the command
};

/*!
@brief Singleton Undo/Redo manager using std::deque for efficient trimming
*/
/** @brief UndoManager */
class UndoManager
{
public:
    static UndoManager& GetInstance();

    void AddCommand(std::unique_ptr<EditorCommand>&& command); //!< Runs and records command
    void Undo();  //!< Ctrl+Z
    void Redo();  //!< Ctrl+Y
    void Clear(); //!< Clear both stacks

    bool CanUndo() const { return !mUndoStack.empty(); }
    bool CanRedo() const { return !mRedoStack.empty(); }

    void SetMaxHistory(size_t max) { mMaxHistory = max; }

private:
    UndoManager() = default;
    ~UndoManager() = default;
    UndoManager(const UndoManager&) = delete;
    UndoManager& operator=(const UndoManager&) = delete;

private:
    std::deque<std::unique_ptr<EditorCommand>> mUndoStack;
    std::deque<std::unique_ptr<EditorCommand>> mRedoStack;
    size_t mMaxHistory = 30; //!< default limit
};
