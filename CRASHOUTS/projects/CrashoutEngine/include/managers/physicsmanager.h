/*!
@file       Physics.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Contains Physics runSystem function for collision detection and physics resolution (calculate new pos by vel)

//__________________________________________________________________________*/

#pragma once

#include "math/CEVec3.h"
#include "managers/entitymanager.h"
#include "managers/collisionmanager.h"
#include "messaging/observer.h"
#include "components/rigidbodycomponent.h"
#include "log.h"
#include <vector>

namespace Physics {

	void applyImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS);

	class PhysicsHandler : public Messaging::Observer {
		private:
			std::vector<Collision::CollisionEvent> collisionEvents;
			Collision::CollisionHandler collisionHandler;

			// --- Debug state (static so UI can toggle globally) ---
			static inline bool sPaused = false;
			static inline bool sStepOnce = false;
			static inline bool sOverrideDt = false;
			static inline double sDebugDt = 1.0 / 60.0;

			static inline CEVec3 sGravity{ 0.f, -2.f, 0.f };
			static inline float  sLinDamp = 2.0f;
		public:


			PhysicsHandler();

			~PhysicsHandler() = default;

			/*!
			@brief Modify Transform Component, and do Collision checks and resolutions. Call this function for the main PHYSICS update. Every entity with a rigidbody, collider and transform component will get their transforms modified
			@param globalECS ECS object for ECS functions
			@param globalEntities Collection of Entities
			@param deltaTime Time taken for the previous frame
			*/
			void update(double deltatime);

			/*!
			@brief Based on the velocity of the current RigidBodyComponent, modify the transformComponent accordingly
			@param transformComp The transform component to modify
			@param rigidBodyComp The rigidbody component to get the velocity from
			@param deltaTime Time taken for the previous frame
			*/
			void updatePhysics(Component::TransformComponent *transformComp, Component::RigidBodyComponent *rigidBodyComp, double deltaTime);

			//void fixedUpdate();

			// --- Debug API ---
			const std::vector<Collision::CollisionEvent>& collisions() const { return collisionEvents; }
			void clearCollisions() { collisionEvents.clear(); }

			static void SetPaused(bool p) { sPaused = p; }
			static bool GetPaused() { return sPaused; }
			static void StepOnce() { sStepOnce = true; }
			static void SetOverrideDt(bool o) { sOverrideDt = o; }
			static void SetDebugDt(double dt) { sDebugDt = dt; }

			static void SetGravity(const CEVec3& g);
			static CEVec3 GetGravity();
			static void SetLinearDamping(float c);
			static float GetLinearDamping();
	};
}
