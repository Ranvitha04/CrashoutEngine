/*!
@file       aimanager.cpp
@author     miratristan.d@digipen.edu
@date       29/11/2025
@brief      aimanager.cpp file contains the definitions for AI utility functions

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved

//__________________________________________________________________________*/
#include "aimanager.h"
#include "aicomponent.h"
#include "entitymanager.h"
#include "colliders.h"
#include "collidercomponent.h"
#include "collisionmanager.h"
#include "state.h"
#include "CEVec3.h"
#include "CEMath.h"

#include "transformcomponent.h"

namespace AIUtils {
	bool AIUtils::IsPlatformInFront(CrashoutsEngine::Entity entity, float direction, std::string tag) {
		CrashoutsEngine::ECS& globalECS{ *CrashoutsEngine::ECS::getInstance() };
		if (!globalECS.HasComponent<Component::ColliderComponent>(entity)) return false;

		Component::ColliderComponent& targetCollider{ *globalECS.GetComponent<Component::ColliderComponent>(entity) };
		Component::TransformComponent& targetTransform{ *globalECS.GetComponent<Component::TransformComponent>(entity) };

		Collision::AABBRect projectedAABB{ Collision::GetAABB(*targetCollider.collider, targetTransform.position) };
		CEVec3 newExtents{ projectedAABB.extents };

		if (direction > 0.f) {
			newExtents.x *= 2.f;
			newExtents.y *= -2.f;
		}
		else {
			newExtents *= -2.f;
		}

		projectedAABB.updateCollider(targetTransform.position + newExtents);

		std::vector<CrashoutsEngine::Entity> colliderComps{ globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity };


		for (CrashoutsEngine::Entity entityToComp : colliderComps) {
			if (entity == entityToComp) continue;
			Component::Tag& name{ *globalECS.GetComponent<Component::Tag>(entityToComp) };
			if (name.id != tag && name.id != std::string{ "" }) continue;

			Component::ColliderComponent& colliderComp{ *globalECS.GetComponent<Component::ColliderComponent>(entityToComp) };
			Component::TransformComponent& transformComp{ *globalECS.GetComponent<Component::TransformComponent>(entityToComp) };

			Collision::AABBRect aabbCompare{ Collision::GetAABB(*colliderComp.collider, transformComp.position) };

			if (aabbCompare.intersect(projectedAABB)) return true;

		}


		return false;
	}

	bool AIUtils::IsObstacleInFront(CrashoutsEngine::Entity entity, float direction, std::string tag) {

		f32 toleranceNeg{ 0.01f };
		f32 tolerance{ 1.f + toleranceNeg };

		CrashoutsEngine::ECS& globalECS{ *CrashoutsEngine::ECS::getInstance() };
		if (!globalECS.HasComponent<Component::ColliderComponent>(entity)) return false;

		Component::ColliderComponent& targetCollider{ *globalECS.GetComponent<Component::ColliderComponent>(entity) };
		Component::TransformComponent& targetTransform{ *globalECS.GetComponent<Component::TransformComponent>(entity) };

		Collision::AABBRect projectedAABB{ Collision::GetAABB(*targetCollider.collider, targetTransform.position) };

		CEVec3 newPos{ targetTransform.position };
		if (direction < 0.f) {
			newPos.x -= newPos.x * toleranceNeg;
			newPos.y += newPos.y * toleranceNeg;
		}
		else {
			newPos *= tolerance;
		}

		projectedAABB.updateCollider(newPos);

		std::vector<CrashoutsEngine::Entity> colliderComps{ globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity };


		for (CrashoutsEngine::Entity entityToComp : colliderComps) {
			if (entity == entityToComp) continue;
			Component::Tag& name{ *globalECS.GetComponent<Component::Tag>(entityToComp) };
			if (name.id != tag && name.id != std::string{ "" }) continue;

			Component::ColliderComponent& colliderComp{ *globalECS.GetComponent<Component::ColliderComponent>(entityToComp) };
			Component::TransformComponent& transformComp{ *globalECS.GetComponent<Component::TransformComponent>(entityToComp) };

			Collision::AABBRect aabbCompare{ Collision::GetAABB(*colliderComp.collider, transformComp.position) };

			if (aabbCompare.intersect(projectedAABB)) return true;

		}

		return false;
	}

	bool IsObstacleIntersecting(CrashoutsEngine::Entity entity, std::string tag) {
		CrashoutsEngine::ECS& globalECS{ *CrashoutsEngine::ECS::getInstance() };
		if (!globalECS.HasComponent<Component::ColliderComponent>(entity)) return false;

		Component::ColliderComponent& targetCollider{ *globalECS.GetComponent<Component::ColliderComponent>(entity) };
		Component::TransformComponent& targetTransform{ *globalECS.GetComponent<Component::TransformComponent>(entity) };

		Collision::AABBRect projectedAABB{ Collision::GetAABB(*targetCollider.collider, targetTransform.position) };

		std::vector<CrashoutsEngine::Entity> colliderComps{ globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity };

		for (CrashoutsEngine::Entity entityToComp : colliderComps) {
			if (entity == entityToComp) continue;
			Component::Tag& name{ *globalECS.GetComponent<Component::Tag>(entityToComp) };
			if (name.id != tag && name.id != std::string{ "" }) continue;

			Component::ColliderComponent& colliderComp{ *globalECS.GetComponent<Component::ColliderComponent>(entityToComp) };
			Component::TransformComponent& transformComp{ *globalECS.GetComponent<Component::TransformComponent>(entityToComp) };

			Collision::AABBRect aabbCompare{ Collision::GetAABB(*colliderComp.collider, transformComp.position) };

			if (aabbCompare.intersect(projectedAABB)) return true;

		}


		return false;
	}

	bool IsIntersecting(CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2) {
		CrashoutsEngine::ECS& globalECS{ *CrashoutsEngine::ECS::getInstance() };

		Component::ColliderComponent& c1{ *globalECS.GetComponent<Component::ColliderComponent>(entity1) };
		Component::TransformComponent& t1{ *globalECS.GetComponent<Component::TransformComponent>(entity1) };

		Component::ColliderComponent& c2{ *globalECS.GetComponent<Component::ColliderComponent>(entity2) };
		Component::TransformComponent& t2{ *globalECS.GetComponent<Component::TransformComponent>(entity2) };

		return Collision::GetAABB(*c1.collider, t1.position).intersect(Collision::GetAABB(*c2.collider, t2.position));
	}
	bool EntityInRange(CrashoutsEngine::Entity entity1, CrashoutsEngine::Entity entity2, float tolerance) {
		CrashoutsEngine::ECS& globalECS{ *CrashoutsEngine::ECS::getInstance() };
		Component::TransformComponent& t1{ *globalECS.GetComponent<Component::TransformComponent>(entity2) };
		Component::TransformComponent& t2{ *globalECS.GetComponent<Component::TransformComponent>(entity1) };


		CEVec3 vectFromT1T2{ t2.position - t1.position };

		f32 squaredMag{ CEMath::Length(vectFromT1T2) };

		if (tolerance >= squaredMag) return true;

		return false;
	}
}

namespace CrashoutsEngine {
	void AIManager::Update() {
		ECS& aiManagerECS{ *ECS::getInstance() };


		std::vector<CrashoutsEngine::Entity> aiComps{ aiManagerECS.GetComponentArray<Component::AIComponent>().indexToEntity };


		for (CrashoutsEngine::Entity entity : aiComps) {
			Component::AIComponent& ai{ *aiManagerECS.GetComponent<Component::AIComponent>(entity) };

			ai.fsm.Update();
		}
	}
}
