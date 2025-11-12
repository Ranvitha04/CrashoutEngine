/*!
@file        UndoManager.h
@author      m.biniswandi@digipen.edu
@brief       Singleton manager for undo/redo command history (deque version)
@date        08/11/2025
*/

#pragma once
#include <deque>
#include <memory>

/*!
@brief Base interface for all editor commands (actions that can be undone/redone)
*/
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
class UndoManager
{
public:
    static UndoManager& GetInstance();

    void Execute(std::unique_ptr<EditorCommand> command); //!< Runs and records command
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
    size_t mMaxHistory = 20; //!< default limit
};
