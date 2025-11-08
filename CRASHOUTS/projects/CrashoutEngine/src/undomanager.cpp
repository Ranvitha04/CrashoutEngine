#include "UndoManager.h"

UndoManager& UndoManager::GetInstance()
{
    static UndoManager instance;
    return instance;
}

void UndoManager::Execute(std::unique_ptr<ICommand> command)
{
    if (!command)
        return;

    command->Execute();
    mUndoStack.push(std::move(command));

    // Clear redo history since a new command invalidates future redos
    while (!mRedoStack.empty())
        mRedoStack.pop();
}

void UndoManager::Undo()
{
    if (mUndoStack.empty())
        return;

    auto cmd = std::move(mUndoStack.top());
    mUndoStack.pop();

    cmd->Undo();
    mRedoStack.push(std::move(cmd));
}

void UndoManager::Redo()
{
    if (mRedoStack.empty())
        return;

    auto cmd = std::move(mRedoStack.top());
    mRedoStack.pop();

    cmd->Execute();
    mUndoStack.push(std::move(cmd));
}

void UndoManager::Clear()
{
    while (!mUndoStack.empty())
        mUndoStack.pop();
    while (!mRedoStack.empty())
        mRedoStack.pop();
}