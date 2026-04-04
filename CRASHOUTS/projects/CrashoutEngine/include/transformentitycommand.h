/*!
@file   transformentitycommand.h
@author Ranvitha (48%), Afiq (38%), Shahrin Idzuan (14%)
@date   29/11/2025

@brief  transformentitycommand.h file contains the declaration of TransformEntityCommand and TransformGroupCommand classes for undo/redo functionality in the CrashoutsEngine

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once
#include "UndoManager.h"
#include "entitymanager.h" // for ECS access
#include "transformcomponent.h"
#include "hierarchycomponent.h"

namespace CrashoutsEngine
{
	/** @brief TransformState */
	struct TransformState
	{
		CEVec3 position;
		float  rotation; // in radians
		CEVec2 scale;

		bool   hasParent = false;
		Entity parent = 0;
		CEVec3 localPos{ 0.f, 0.f, 0.f };
		float  localRot = 0.f;
		CEVec2 localScale{ 1.f, 1.f };
	};
	static inline void ApplyTransformState(ECS& ecs, Entity e, const TransformState& s)
	{
		auto* t = ecs.GetComponent<Component::TransformComponent>(e);
		if (!t) return;

		if (s.hasParent && ecs.HasComponent<Component::ParentComponent>(e))
		{
			auto* pc = ecs.GetComponent<Component::ParentComponent>(e);
			if (pc)
			{
				pc->parent = s.parent;
				pc->localPos = s.localPos;
				pc->localRot = s.localRot;
				pc->localScale = s.localScale;
			}
		}

		
		t->position = s.position;
		t->rotation = s.rotation;
		t->size = s.scale;
	}
	/** @brief TransformEntityCommand */
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
			ApplyTransformState(*ecs, mEntity, mNew);
		}

		void Undo() override
		{
			auto* ecs = ECS::getInstance();
			ApplyTransformState(*ecs, mEntity, mOld);
		}
	};

	/** @brief TransformGroupCommand */
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
				ApplyTransformState(*ecs, mEntities[i], mNew[i]);
			}
		}

		void Undo() override
		{
			auto* ecs = ECS::getInstance();
			for (size_t i = 0; i < mEntities.size(); ++i)
			{
				ApplyTransformState(*ecs, mEntities[i], mOld[i]);
			}
		}
	};
}

