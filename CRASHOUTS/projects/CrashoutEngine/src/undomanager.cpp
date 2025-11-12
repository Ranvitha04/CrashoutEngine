/*!
@file        UndoManager.cpp
@brief       Implementation of UndoManager using std::deque
@date        08/11/2025
*/

#include "undomanager.h"

UndoManager& UndoManager::GetInstance()
{
    static UndoManager instance;
    return instance;
}

void UndoManager::Execute(std::unique_ptr<EditorCommand> command)
{
    if (!command)
        return;

    command->Execute();
    mUndoStack.push_back(std::move(command));
    mRedoStack.clear(); // new command invalidates redo history

    // trim oldest undo if we exceed max history
    if (mUndoStack.size() > mMaxHistory)
        mUndoStack.pop_front();
}

void UndoManager::Undo()
{
    if (mUndoStack.empty())
        return;

    auto cmd = std::move(mUndoStack.back());
    mUndoStack.pop_back();

    cmd->Undo();
    mRedoStack.push_back(std::move(cmd));

    // trim oldest redo if we exceed max history
    if (mRedoStack.size() > mMaxHistory)
        mRedoStack.pop_front();
}

void UndoManager::Redo()
{
    if (mRedoStack.empty())
        return;

    auto cmd = std::move(mRedoStack.back());
    mRedoStack.pop_back();

    cmd->Execute();
    mUndoStack.push_back(std::move(cmd));

    // trim oldest undo if we exceed max history
    if (mUndoStack.size() > mMaxHistory)
        mUndoStack.pop_front();
}

void UndoManager::Clear()
{
    mUndoStack.clear();
    mRedoStack.clear();
}
