/*!
@file       transformentitycommand.h
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      transformentitycommand.h file contains the declaration of TransformEntityCommand and TransformGroupCommand classes for undo/redo functionality in the CrashoutsEngine

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/

#pragma once
#include "UndoManager.h"
#include "entitymanager.h" // for ECS access
#include "transformcomponent.h"

namespace CrashoutsEngine
{
	struct TransformState
	{
		CEVec3 position;
		float  rotation; // in radians
		CEVec2 scale;
	};
	class TransformEntityCommand : public EditorCommand
	{
		Entity mEntity;
		TransformState mOld;
		TransformState mNew;

	public:
		TransformEntityCommand(Entity entity, const TransformState& oldT, const TransformState& newT)
			: mEntity(entity), mOld(oldT), mNew(newT) {
		}

		void Execute() override
		{
			auto* ecs = ECS::getInstance();
			if (auto* t = ecs->GetComponent<Component::TransformComponent>(mEntity))
			{
				t->position = mNew.position;
				t->rotation = mNew.rotation;
				t->size = mNew.scale;
			}
		}

		void Undo() override
		{
			auto* ecs = ECS::getInstance();
			if (auto* t = ecs->GetComponent<Component::TransformComponent>(mEntity))
			{
				t->position = mOld.position;
				t->rotation = mOld.rotation;
				t->size = mOld.scale;
			}
		}
	};

	class TransformGroupCommand : public EditorCommand
	{
		std::vector<Entity> mEntities;
		std::vector<TransformState> mOld;
		std::vector<TransformState> mNew;

	public:
		TransformGroupCommand(const std::vector<Entity>& ents,
			const std::vector<TransformState>& oldT,
			const std::vector<TransformState>& newT)
			: mEntities(ents), mOld(oldT), mNew(newT)
		{
			assert(mEntities.size() == mOld.size() && mEntities.size() == mNew.size());
		}

		void Execute() override
		{
			auto* ecs = ECS::getInstance();
			for (size_t i = 0; i < mEntities.size(); ++i)
			{
				if (auto* t = ecs->GetComponent<Component::TransformComponent>(mEntities[i]))
				{
					t->position = mNew[i].position;
					t->rotation = mNew[i].rotation;
					t->size = mNew[i].scale;
				}
			}
		}

		void Undo() override
		{
			auto* ecs = ECS::getInstance();
			for (size_t i = 0; i < mEntities.size(); ++i)
			{
				if (auto* t = ecs->GetComponent<Component::TransformComponent>(mEntities[i]))
				{
					t->position = mOld[i].position;
					t->rotation = mOld[i].rotation;
					t->size = mOld[i].scale;
				}
			}
		}
	};
}

