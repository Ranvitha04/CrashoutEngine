#pragma once
#include <stack>
#include <memory>


class ICommand
{
public:
    virtual ~ICommand() = default;

    /// Execute the command (redo or first run)
    virtual void Execute() = 0;

    /// Undo the command (revert the state)
    virtual void Undo() = 0;
};

class UndoManager
{
public:
    static UndoManager& GetInstance();

    /// Execute a new command and push it onto the undo stack
    void Execute(std::unique_ptr<ICommand> command);

    /// Undo the last command (Ctrl+Z)
    void Undo();

    /// Redo the last undone command (Ctrl+Y)
    void Redo();

    /// Clear all history (e.g. when loading a new scene)
    void Clear();

    /// Returns true if undo is available
    bool CanUndo() const { return !mUndoStack.empty(); }

    /// Returns true if redo is available
    bool CanRedo() const { return !mRedoStack.empty(); }

private:
    UndoManager() = default;
    ~UndoManager() = default;

    UndoManager(const UndoManager&) = delete;
    UndoManager& operator=(const UndoManager&) = delete;

private:
    std::stack<std::unique_ptr<ICommand>> mUndoStack;
    std::stack<std::unique_ptr<ICommand>> mRedoStack;
};