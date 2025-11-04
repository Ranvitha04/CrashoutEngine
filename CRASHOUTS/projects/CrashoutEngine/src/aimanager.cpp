#include "aimanager.h"
#include "aicomponent.h"
#include "entitymanager.h"
#include "colliders.h"
#include "collidercomponent.h"
#include "collisionmanager.h"
#include "state.h"
#include "CEVec3.h"

namespace AIUtils {
	bool AIUtils::IsPlatformInFront(CrashoutsEngine::Entity entity, float direction) {
		CrashoutsEngine::ECS& globalECS{ *CrashoutsEngine::ECS::getInstance() };
		if (!globalECS.HasComponent<Component::ColliderComponent>(entity)) return false;

		Component::ColliderComponent& targetCollider{ *globalECS.GetComponent<Component::ColliderComponent>(entity) };
		Component::TransformComponent& targetTransform{ *globalECS.GetComponent<Component::TransformComponent>(entity) };

		Collision::AABBRect projectedAABB{Collision::GetAABB(*targetCollider.collider, targetTransform.position)};
		CEVec3 newExtents{ projectedAABB.extents };

		if (direction > 0.f) {
			newExtents.x *= 2.f;
			newExtents.y *= -2.f;
		} else {
			newExtents *= -2.f;
		}

		projectedAABB.updateCollider(targetTransform.position + newExtents);

		std::vector<CrashoutsEngine::Entity> colliderComps{ globalECS.GetComponentArray<Component::ColliderComponent>().indexToEntity };


		for (CrashoutsEngine::Entity entityToComp : colliderComps) {
			if (entity == entityToComp) continue;
			
			Component::ColliderComponent& colliderComp{ *globalECS.GetComponent<Component::ColliderComponent>(entityToComp) };
			Component::TransformComponent& transformComp{ *globalECS.GetComponent<Component::TransformComponent>(entityToComp) };

			Collision::AABBRect aabbCompare{ Collision::GetAABB(*colliderComp.collider, transformComp.position) };

			if (aabbCompare.intersect(projectedAABB)) return true;
			
		}
		

		return false;
	}
}

namespace CrashoutsEngine {
	void AIManager::Update() {
		ECS& aiManagerECS{ *ECS::getInstance() };

		
		std::vector<CrashoutsEngine::Entity> aiComps{ aiManagerECS.GetComponentArray<Component::AIComponent>().indexToEntity };


		for (CrashoutsEngine::Entity entity : aiComps) {
			Component::AIComponent& ai{ *aiManagerECS.GetComponent<Component::AIComponent>(entity)};

			ai.fsm.Update();
		}
	}
}
