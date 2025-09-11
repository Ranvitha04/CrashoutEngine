/*!
@file       Physics.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Contains Physics runSystem function for collision detection and physics resolution (calculate new pos by vel)

//__________________________________________________________________________*/

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
			/*!
			@brief Modify Transform Component, and do Collision checks and resolutions
			@param globalECS ECS object for ECS functions
			@param globalEntities Collection of Entities
			@param deltaTime Time taken for the previous frame
			*/
			static void runSystem(ECS::ECS& globalECS, std::vector<ECS::Entity>& globalEntities, double deltaTime) {
				/*DETECT COLLISION AND RESOLVE COLLISION*/
				for (ECS::Entity &entity1 : globalEntities) {
					if (!globalECS.HasComponent<Component::ColliderComponent>(entity1) || !globalECS.HasComponent<Component::TransformComponent>(entity1)) continue; 
					for (ECS::Entity const& entity2 : globalEntities) {
						if (entity1 == entity2) continue;
						if (!globalECS.HasComponent<Component::ColliderComponent>(entity2) || !globalECS.HasComponent<Component::TransformComponent>(entity2)) continue;
					
						/*DISPATCH COLLISION FUNCT*/
						/*TODO MESSAGING SYSTEM*/
					}
				}

				/*PHYSICS SIM, UPDATE POSITION*/
				for (ECS::Entity& entity : globalEntities) {
					if (!globalECS.HasComponent<Component::RigidBodyComponent>(entity) || !globalECS.HasComponent<Component::TransformComponent>(entity)) continue;
					Component::TransformComponent* transform { globalECS.GetComponent<Component::TransformComponent>(entity) };
					Component::RigidBodyComponent* rigidBody { globalECS.GetComponent<Component::RigidBodyComponent>(entity) };
					
					rigidBody->prevVelocity = CEVec3{ transform->posX, transform->posY, transform->posZ };
					transform->posX += static_cast<float>(deltaTime) * rigidBody->velocity.x;
					transform->posY += static_cast<float>(deltaTime) * rigidBody->velocity.y;
					transform->posZ += static_cast<float>(deltaTime) * rigidBody->velocity.z;
				}
			}
	};
}
