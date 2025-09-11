#pragma once

#include "CEVec3.h"
#include "entitymanager.h"
#include "Collision.h"

namespace Component {
	struct RigidBodyComponent {
		f32 mass;
		f32 force;
		bool bGravity;
		CEVec3 velocity;
		CEVec3 prevVelocity;
	};
}

namespace Physics {
	class PhysicsHandler {
		public:
			static void runSystem(ECS::ECS& globalECS, std::vector<ECS::Entity>& globalEntities, f32 deltaTime) {
				/*DETECT COLLISION AND RESOLVE COLLISION*/
				for (ECS::Entity &entity1 : globalEntities) {
					if (!globalECS.HasComponent<Component::ColliderComponent>(entity1) || !globalECS.HasComponent<Component::TransformComponent>(entity1)) continue; 
					for (ECS::Entity const& entity2 : globalEntities) {
						if (entity1 == entity2) continue;
						if (!globalECS.HasComponent<Component::ColliderComponent>(entity2) || !globalECS.HasComponent<Component::TransformComponent>(entity2)) continue;
					
						/*DISPATCH COLLISION FUNCT*/
					}
				}

				/*PHYSICS SIM*/
				for (ECS::Entity& entity : globalEntities) {
					if (!globalECS.HasComponent<Component::RigidBodyComponent>(entity) || !globalECS.HasComponent<Component::TransformComponent>(entity)) continue;
					Component::TransformComponent* transform { globalECS.GetComponent<Component::TransformComponent>(entity) };
					Component::RigidBodyComponent* rigidBody { globalECS.GetComponent<Component::RigidBodyComponent>(entity) };
					
					rigidBody->prevVelocity = CEVec3{ transform->posX, transform->posY, transform->posZ };
					transform->posX += deltaTime * rigidBody->velocity.x;
					transform->posY += deltaTime * rigidBody->velocity.y;
					transform->posZ += deltaTime * rigidBody->velocity.z;
				}
			}
	};
}
