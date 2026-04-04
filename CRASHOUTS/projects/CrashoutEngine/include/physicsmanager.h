/*!
@file   physicsmanager.h
@author Renfred Lee
@date   08/09/2025

@brief  Contains Physics runSystem function for collision detection and physics resolution (calculate new pos by vel)

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "CEVec3.h"
#include "entitymanager.h"
#include "collisionmanager.h"
#include "observer.h"
#include "rigidbodycomponent.h"
#include "transformcomponent.h"
#include <vector>

namespace Physics {

	void ApplyImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS);

	/** @brief PhysicsHandler */
	class PhysicsHandler {
		private:

			// --- Debug state (static so UI can toggle globally) ---
			static inline bool sPaused = true;
			static inline bool sStepOnce = false;
			static inline bool sOverrideDt = false;
			static inline double sDebugDt = 1.0 / 60.0;

			static inline CEVec3 sGravity{ 0.f, -200.f, 0.f };
			static inline float  sLinDamp = .1f;

			PhysicsHandler() {};
		public:

			static PhysicsHandler& GetInstance() {
				static PhysicsHandler instance; // Meyer's Singleton
				return instance;
			}

			// --- Delete Copy/Move Operations ---
			// Prevent copies and moves
			PhysicsHandler(const PhysicsHandler&) = delete;
			PhysicsHandler& operator=(const PhysicsHandler&) = delete;
			PhysicsHandler(PhysicsHandler&&) = delete;
			PhysicsHandler& operator=(PhysicsHandler&&) = delete;

			~PhysicsHandler() = default;

			/*!
			@brief Modify Transform Component. Call this function for the main PHYSICS Update. Every entity with a rigidbody, collider and transform component will get their transforms modified
			@param globalECS ECS object for ECS functions
			@param globalEntities Collection of Entities
			@param deltatime Time taken for the previous frame
			*/
			void Update(double deltatime);

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
