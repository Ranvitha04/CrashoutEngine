#pragma once
#include "UndoManager.h"
#include "entitymanager.h" // for ECS access
#include "transformcomponent.h"

class MoveEntityCommand : public EditorCommand
{
    CrashoutsEngine::Entity mEntity;
    CEVec3 mOldPos;
    CEVec3 mNewPos;

public:
    MoveEntityCommand(CrashoutsEngine::Entity entity, const CEVec3& oldPos, const CEVec3& newPos)
        : mEntity(entity), mOldPos(oldPos), mNewPos(newPos) {
    }

    void Execute() override
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (ecs->HasComponent<Component::TransformComponent>(mEntity))
            ecs->GetComponent<Component::TransformComponent>(mEntity)->position = mNewPos;
    }

    void Undo() override
    {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        if (ecs->HasComponent<Component::TransformComponent>(mEntity))
            ecs->GetComponent<Component::TransformComponent>(mEntity)->position = mOldPos;
    }
};

class MoveGroupCommand : public EditorCommand {
    std::vector<CrashoutsEngine::Entity> mEntities;
    std::vector<CEVec3> mOldPos, mNewPos;
public:
    MoveGroupCommand(const std::vector<CrashoutsEngine::Entity>& ents,
        const std::vector<CEVec3>& oldP,
        const std::vector<CEVec3>& newP)
        : mEntities(ents), mOldPos(oldP), mNewPos(newP) {
    }

    void Execute() override {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        for (size_t i = 0; i < mEntities.size(); ++i)
            if (ecs->HasComponent<Component::TransformComponent>(mEntities[i]))
                ecs->GetComponent<Component::TransformComponent>(mEntities[i])->position = mNewPos[i];
    }
    void Undo() override {
        auto* ecs = CrashoutsEngine::ECS::getInstance();
        for (size_t i = 0; i < mEntities.size(); ++i)
            if (ecs->HasComponent<Component::TransformComponent>(mEntities[i]))
                ecs->GetComponent<Component::TransformComponent>(mEntities[i])->position = mOldPos[i];
    }
};
