/*!
@file       Physics.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Contains Definitions for Physics runSystem function and the RigidBodyComponent

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "physicsmanager.h"
#include "frameratecontroller.h"
#include "hierarchy_components.h"
#include "namecomponent.h"
#include "log.h"
#include <cstdio>
#include "CEMath.h"
#include "observer.h"
#include "glapp.h"
#include "camera.h"

namespace {

	std::pair<f32, f32> ComputeWorldDimensions() {
		f32 worldWidth{}, worldHeight{};
		const float BASE_SIZE = 100.0f;
		float aspect = static_cast<float>(GLApp::width) / static_cast<float>(GLApp::height);
		if (aspect >= 1.0f) {
			worldHeight = BASE_SIZE;
			worldWidth = worldHeight * aspect;
		}
		else {
			worldWidth = BASE_SIZE;
			worldHeight = worldWidth / aspect;
		}

		return std::pair{ worldWidth, worldHeight };
	}

}


namespace Component {

	f32 RigidBodyComponent::GetInverseMass() {
		if (!mMass) return 0.f; /*Prevent division by 0*/
		return 1.f / mMass;
	}

	void RigidBodyComponent::addForce(CEVec3 force) {
		mNetForce += force;
	}
	void RigidBodyComponent::clearForceAccumulator() {
		mNetForce = CEVec3{};
	}

	void RigidBodyComponent::setMaxVelocity(CEVec3 const& newVel) {
		mMaximumVel = newVel;
	}

	void RigidBodyComponent::setMass(f32 newMass) {
		mMass = newMass;
	}

	void RigidBodyComponent::setRestitution(f32 restitution) {
		mRestitutionCoeff = restitution;
	}

	RigidBodyComponent::RigidBodyComponent(CEVec3 maximumVel, f32 mass, f32 restitutionCoeff)
		: mVelocity{ CEVec3{} }, mMaximumVel{ maximumVel }, mMass{ mass }, mNetForce{ CEVec3{} }, mRestitutionCoeff{ restitutionCoeff } {
		flags.set(Physics::SIMULATE_DYNAMICS);
	}

	/*Euler*/
	void RigidBodyComponent::UpdatePhysics(double dt) {

		/*Prevent further calculation (Treat 0 mass objects as static (Wont be affected by external forces but still have colliders/collision resolution))*/
		if (mMass == 0.f) {
			return;
		}

		/*
			F(Resultant Force) = ma;
			FIND a = F(Resultant Force) / m
		*/
		CEVec3 acceleration = mNetForce * GetInverseMass();

		/*TODO PREVENT VELOCITY FROM GOING FURTHER FROM MAX velocity*/
		mVelocity += static_cast<f32>(dt) * acceleration;

		clearForceAccumulator(); /*Reset forces acting upon object in an iteration*/

	}

	void RigidBodyComponent::UpdatePhysicsWithRotation(double dt) {

		/*Prevent further calculation (Treat 0 mass objects as static (Wont be affected by external forces but still have colliders/collision resolution))*/
		if (mMass == 0.f) {
			return;
		}

		/*
			F(Resultant Force) = ma;
			FIND a = F(Resultant Force) / m
		*/
		CEVec3 acceleration = mNetForce * GetInverseMass();

		f32 newDt{ static_cast<f32>(dt) };

		/*TODO PREVENT VELOCITY FROM GOING FURTHER FROM MAX velocity*/
		mVelocity += newDt * acceleration;

		constexpr float kStiffness = 20.0f;
		constexpr float kDamping = 4.0f;

		float currentAngleRad = orientation * (CEMath::PI / 180.f);
		float targetAngleRad = 0.0f;
		float angleDiff = targetAngleRad - currentAngleRad;


		float angularAccel = (angleDiff * kStiffness) - (angularVelocity * kDamping);

		angularVelocity += angularAccel * newDt;

		orientation += angularVelocity * (180.f / CEMath::PI) * newDt;

		clearForceAccumulator(); /*Reset forces acting upon object in an iteration*/

	}

	CEVec3 RigidBodyComponent::GetVelocity() {
		return mVelocity;
	}

	void RigidBodyComponent::setVelocity(CEVec3 const& newVel) {
		this->mVelocity = newVel;
	}

	f32 RigidBodyComponent::GetCoeff() {
		return mRestitutionCoeff;
	}

	Physics::ForceManager& RigidBodyComponent::getForceManager() {
		return fm;
	}

	CEVec3 RigidBodyComponent::getMaxVelocity() {
		return mMaximumVel;
	}

}

/*TEMP*/
namespace {
	void operator+=(Component::TransformComponent* component, CEVec3 const& trans) {
		component->position.x += trans.x;
		component->position.y += trans.y;
		component->position.z += trans.z;
	}

	void operator-=(Component::TransformComponent* component, CEVec3 const& trans) {
		component->position.x -= trans.x;
		component->position.y -= trans.y;
		component->position.z -= trans.z;
	}

	f32 Cross(CEVec3 const&a, CEVec3 const&b) {
		return a.x * b.y - a.y * b.x;
	}
}

namespace Physics {

	void PhysicsHandler::HandleCollisionMessages(Messaging::Messages& collEventBase, Messaging::Observer& obsBase) {

		Collision::CollisionMessage& collEvent = static_cast<Collision::CollisionMessage&>(collEventBase);
		PhysicsHandler& observer = static_cast<PhysicsHandler&>(obsBase);

		if (observer.GetPaused()) return;

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());
		std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> colPair{ collEvent.GetCollisionPair() };

		// Check for Triggers
		if (globalECS.HasComponent<Component::ColliderComponent>(colPair.first) &&
			globalECS.HasComponent<Component::ColliderComponent>(colPair.second)) {

			Component::ColliderComponent* c1 = globalECS.GetComponent<Component::ColliderComponent>(colPair.first);
			Component::ColliderComponent* c2 = globalECS.GetComponent<Component::ColliderComponent>(colPair.second);

			if (c1->collider->isTrigger || c2->collider->isTrigger) return;
		}

		// Check for RigidBody
		if (!globalECS.HasComponent<Component::RigidBodyComponent>(colPair.first) &&
			!globalECS.HasComponent<Component::RigidBodyComponent>(colPair.second)) {

			return;
		}

		Collision::CollisionEvent evnt{ collEvent.GetCollisionManifold(), colPair.first, colPair.second };
		observer.SetCollisionEvent(evnt); /*TEMP FOR NOW*/
	}

	void PhysicsHandler::SetCollisionEvent(Collision::CollisionEvent const& event) {
		collisionEvents.push_back(event);
	}

	PhysicsHandler::PhysicsHandler() : Messaging::Observer{} {
		using namespace Messaging;

		HANDLER handler = PhysicsHandler::HandleCollisionMessages;

		// Physics occur on stay and on enter
		attachHandler(EventType::CollisionEnter, handler);
		attachHandler(EventType::CollisionStay, handler);
	}


	// Setters and Getters
	void PhysicsHandler::SetGravity(const CEVec3& g) { sGravity = g; }
	CEVec3 PhysicsHandler::GetGravity() { return sGravity; }
	void PhysicsHandler::SetLinearDamping(float c) { sLinDamp = c; }
	float PhysicsHandler::GetLinearDamping() { return sLinDamp; }

	// Helper to recursively Update child transforms and colliders after physics step
	void SyncHierarchyPhysics(CrashoutsEngine::Entity parent, CrashoutsEngine::ECS& ecs) {
		if (!ecs.HasComponent<Component::ChildrenComponent>(parent)) return;

		auto* childrenComp = ecs.GetComponent<Component::ChildrenComponent>(parent);
		auto* parentTransform = ecs.GetComponent<Component::TransformComponent>(parent);

		for (CrashoutsEngine::Entity child : childrenComp->children) {
			if (!ecs.HasComponent<Component::TransformComponent>(child)) continue;
			if (!ecs.HasComponent<Component::ParentComponent>(child)) continue;

			auto* childTransform = ecs.GetComponent<Component::TransformComponent>(child);
			auto* parentRel = ecs.GetComponent<Component::ParentComponent>(child);

			// 1. Manually Re-calculate Child World Position based on NEW Parent Position
			// (Same logic as UpdateHierarchy, but local to this physics step)
			// This ensures the child 'teleports' along with the parent for collision checks

			// Simple version (assuming no rotation for brevity, add rotation math if needed)
			// childTransform->position = parentTransform->position + parentRel->localPos; 

			// FULL MATH (Matching your Hierarchy.cpp):
			const float s = std::sin(parentTransform->rotation);
			const float c = std::cos(parentTransform->rotation);
			const float lx = parentRel->localPos.x;
			const float ly = parentRel->localPos.y;
			const float rotatedX = (c * lx - s * ly);
			const float rotatedY = (s * lx + c * ly);

			childTransform->position.x = parentTransform->position.x + rotatedX;
			childTransform->position.y = parentTransform->position.y + rotatedY;
			childTransform->position.z = parentTransform->position.z + parentRel->localPos.z;

			// 2. Update Child Collider if it exists
			if (ecs.HasComponent<Component::ColliderComponent>(child)) {
				auto* childCol = ecs.GetComponent<Component::ColliderComponent>(child);
				childCol->collider->UpdateCollider(childTransform->position);
				// Also Update rotation if needed
				// childCol->collider->updateRotation(childTransform->rotation);
			}

			// Recurse
			SyncHierarchyPhysics(child, ecs);
		}
	}

	/*UPDATE BOTH THE RIGIDBODY AND TRANSFORM COMPONENT*/
	void PhysicsHandler::UpdatePhysics(Component::TransformComponent* transformComp, Component::RigidBodyComponent* rigidBodyComp, double deltaTime) {

		// Debug Stepping one
		//double dt = sOverrideDt ? sDebugDt : deltaTime;

		f32 newDelta{ static_cast<float>(deltaTime) };
		rigidBodyComp->UpdatePhysics(deltaTime); /*UPDATE CURRENT VELOCITY AND RESET NET FORCE*/
		CEVec3 currentVel{ rigidBodyComp->GetVelocity() };
		transformComp->position += currentVel * newDelta;

		// Debug stepping two
		if (sPaused && sStepOnce) sStepOnce = false;
	}

	void PositionCorrection(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS) {

		Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(event.entity1) };
		Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity1) };

		Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(event.entity2) };
		Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity2) };

		/*SOLVE FOR LINEAR VEL*/
		f32 invMass1 = rigidBody1 ? rigidBody1->GetInverseMass() : 0.0f;
		f32 invMass2 = rigidBody2 ? rigidBody2->GetInverseMass() : 0.0f;

		f32 sumInvMass{ invMass1 + invMass2 };
		//f32 totalMass{ rigidBody1->getMass() + rigidBody2->getMass()};
		if (sumInvMass == 0.f) {
			return;
		}

		/*TOLERANCE VARIABLE (ATTEMPT TO PREVENT JITTER)*/
		const float percent{ 0.7f };

		f32 correction = event.colManifold.depth / (invMass1 + invMass2) * percent;

		CEVec3 vec{ correction * event.colManifold.normal };

		/*MOVE OBJECT ALONG COLLISION NORMAL BEFORE CALCULATING IMPULSE TO CHANGE VELOCITY*/
		if (rigidBody1 && invMass1 > 0.f) transform1->position -= vec * invMass1;
		if (rigidBody2 && invMass2 > 0.f) transform2->position += vec * invMass2;
	}

	void ApplyImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS) {
		Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity1) };
		Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity2) };

		/*SOLVE FOR LINEAR VEL*/
		// 1. SAFE MASS RETRIEVAL
		// If RB exists, use its InverseMass. If nullptr (Wall), InverseMass is 0.0f (Infinite mass).
		f32 invMass1 = (rigidBody1) ? rigidBody1->GetInverseMass() : 0.0f;
		f32 invMass2 = (rigidBody2) ? rigidBody2->GetInverseMass() : 0.0f;

		// 2. SAFE VELOCITY RETRIEVAL
		// Walls have 0 velocity.
		CEVec3 vel1 = (rigidBody1) ? rigidBody1->GetVelocity() : CEVec3{ 0,0,0 };
		CEVec3 vel2 = (rigidBody2) ? rigidBody2->GetVelocity() : CEVec3{ 0,0,0 };

		f32 sumInvMass{ invMass1 + invMass2 };
		//f32 totalMass{ rigidBody1->getMass() + rigidBody2->getMass()};
		if (sumInvMass == 0.f) {
			return;
		}

		/*int impulseSolveAmount{ 5 };

		for (int i{}; i < impulseSolveAmount; ++i) {*/

		/*RELATIVE VELOCITY*/
		CEVec3 relVel{ rigidBody2->GetVelocity() - rigidBody1->GetVelocity() };

		f32 velAlongNormal{ CEMath::Dot(relVel, event.colManifold.normal) };

		//MOVING AWAY
		if (velAlongNormal > 0.f) {
			return;
		}

		float e{ std::min(rigidBody1->GetCoeff(), rigidBody2->GetCoeff()) }; /*COEFFICIENT OF RESTITUTION*/
		float numerator{ -(1.f + e) * velAlongNormal };

		float j = numerator / sumInvMass;

		CEVec3 impulse{ event.colManifold.normal * j };

		rigidBody1->setVelocity(rigidBody1->GetVelocity() -= impulse * invMass1);
		rigidBody2->setVelocity(rigidBody2->GetVelocity() += impulse * invMass2);


		/*FRICTION CALCULATIONS*/
		relVel = rigidBody2->GetVelocity() - rigidBody1->GetVelocity();

		CEVec3 tangent{ relVel - CEMath::Dot(relVel, event.colManifold.normal) * event.colManifold.normal };
		tangent = CEMath::Normalize(tangent);

		f32 jt{ -CEMath::Dot(relVel, tangent) };
		jt /= (invMass1 + invMass2);

		float staticFric1 = rigidBody1 ? rigidBody1->getStaticFriction() : 0.5f;
		float dynamicFric1 = rigidBody1 ? rigidBody1->getDynamicFriction() : 0.3f;

		/*----------------Coulomb's Law,( clamp our friction based on solved j)----------------*/
		f32 mu = std::sqrtf(std::powf(staticFric1,2.f));
		CEVec3 frictionImpulse{};
		if (std::abs(jt) < j * mu) {
			frictionImpulse = jt * tangent;
		}
		else {
			f32 dynamicFric{ std::sqrtf(std::powf(dynamicFric1, 2.f)) };
			frictionImpulse = -j * tangent * dynamicFric;
		}

		if(rigidBody1) rigidBody1->setVelocity(rigidBody1->GetVelocity() - frictionImpulse * invMass1);
		if(rigidBody2) rigidBody2->setVelocity(rigidBody2->GetVelocity() + frictionImpulse * invMass2);

	}

	void ApplyRotationalImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS) {
		Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity1) };
		Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(event.entity1) };

		Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity2) };
		Component::ColliderComponent* collider2{ globalECS.GetComponent<Component::ColliderComponent>(event.entity2) };
		
		f32 angVel1 = rigidBody1 ? rigidBody1->angularVelocity : 0.0f;
		f32 angVel2 = rigidBody2 ? rigidBody2->angularVelocity : 0.0f;
		f32 invMass1 = rigidBody1 ? rigidBody1->GetInverseMass() : 0.0f;
		f32 invMass2 = rigidBody2 ? rigidBody2->GetInverseMass() : 0.0f;
		f32 invInertia1 = rigidBody1 ? rigidBody1->invInertia : 0.0f;
		f32 invInertia2 = rigidBody2 ? rigidBody2->invInertia : 0.0f;
		CEVec3 vel1 = rigidBody1 ? rigidBody1->GetVelocity() : CEVec3{};
		CEVec3 vel2 = rigidBody2 ? rigidBody2->GetVelocity() : CEVec3{};
		float coeff1 = rigidBody1 ? rigidBody1->GetCoeff() : 0.5f;
		float coeff2 = rigidBody2 ? rigidBody2->GetCoeff() : 0.5f;

		CEVec3 penetrationTo1{event.colManifold.contactPoints[0] - collider1->collider->position};
		CEVec3 penetrationTo2{ event.colManifold.contactPoints[0] - collider2->collider->position };

		CEVec3 angularVelPenetration1{ CEVec3{-1.f * angVel1 * penetrationTo1.y, angVel1 * penetrationTo1.x, 0.f} };
		CEVec3 angularVelPenetration2{ CEVec3{-1.f * angVel2 * penetrationTo2.y, angVel2 * penetrationTo2.x, 0.f} };

		CEVec3 relativeVelocity1{ vel1 + angularVelPenetration1 };
		CEVec3 relativeVelocity2{ vel2 + angularVelPenetration2 };

		/*SOLVE FOR LINEAR VEL*/
		f32 sumInvMass{ invMass1 + invMass2 };

		if (sumInvMass == 0.f) {
			return;
		}

		/*RELATIVE VELOCITY*/
		CEVec3 relVel{ relativeVelocity2 - relativeVelocity1 };

		f32 velAlongNormal{ CEMath::Dot(relVel, event.colManifold.normal) };

		//MOVING AWAY
		if (velAlongNormal > 0.f) {
			return;
		}

		float e{ std::min(coeff1, coeff2) }; /*COEFFICIENT OF RESTITUTION*/
		f32 pToPosCrossNormal1{ Cross(penetrationTo1, event.colManifold.normal) };
		f32 pToPosCrossNormal2{ Cross(penetrationTo2, event.colManifold.normal) };


		float numerator{ -(1.f + e) * velAlongNormal };

		f32 crossSum{ pToPosCrossNormal1 * pToPosCrossNormal1 * invInertia1 + pToPosCrossNormal2 * pToPosCrossNormal2 * invInertia2 };

		float j = numerator / (sumInvMass + crossSum);

		CEVec3 impulse{ event.colManifold.normal * j };

		if (rigidBody1) {
			rigidBody1->setVelocity(rigidBody1->GetVelocity() - impulse * invMass1);
			rigidBody1->angularVelocity += -pToPosCrossNormal1 * j * invInertia1;
		}
		if (rigidBody2) {
			rigidBody2->setVelocity(rigidBody2->GetVelocity() + impulse * invMass2);
			rigidBody2->angularVelocity += pToPosCrossNormal2 * j * invInertia2;
		}
	}

	/*Main Physics Update*/
	void PhysicsHandler::Update(double deltaTime) {

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		std::vector<CrashoutsEngine::Entity> globalEntities = globalECS.GetComponentArray<Component::RigidBodyComponent>().indexToEntity;

		if (sPaused) {
			collisionEvents = std::vector<Collision::CollisionEvent>{};
			if (!sStepOnce) return;
			// allow exactly one tick when paused+Step pressed
		}

		/*std::pair<f32, f32> newDimen{ ComputeWorldDimensions() };

		newDimen.first *= 1.2f;
		newDimen.second *= 1.2f;

		CEVec3 newExtents{ newDimen.first * 0.5f, newDimen.second * 0.5f, 0.f };

		CEVec3 pos{ CrashoutsEngine::RenderCamera::gameCamera.GetPosition() };

		Collision::AABBRect bounds{ pos - newExtents, pos + newExtents };*/

		int step{ FrameRateController::GetInstance().StepCount() };

		/*Fixed step, accumulator will reach the target fixed dt and then execute physics simulation*/
		for (int i{}; i < step; ++i) {

			for (CrashoutsEngine::Entity& entity : globalEntities) {
				Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };
				if (!rigidBody->getFlags().test(SIMULATE_DYNAMICS) || !rigidBody->getMass()) continue;

				Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity) };
				Collision::AABBRect region{ Collision::GetAABB(*(collider1->collider), collider1->collider->position) };

				//if (!bounds.contains(region)) continue;


				/*APPLY ANY FORCE GENERATOR HERE, EVERY ENTITY IS AFFECTED*/
				ForceManager& fm{ rigidBody->getForceManager() };
				CEVec3 forces = fm.ResolveForces((f32)deltaTime, rigidBody->getMass());

				// 2. Apply Gravity Manually
				CEVec3 gravityAccel = sGravity; // e.g., (0, -70, 0)

				if (rigidBody->getMass() < 0.f) {
					// Anti-Gravity: Flip the acceleration vector
					gravityAccel = -sGravity; // (0, 70, 0)
				}

				// F = m * a
				CEVec3 gravityForce = rigidBody->getMass() * gravityAccel;

				// 3. Combine
				rigidBody->addForce(forces + gravityForce);


			}

			/*IMPULSE RESOLUTION*/
			for (Collision::CollisionEvent const& evnt : collisionEvents) {

				Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(evnt.entity1) };
				Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(evnt.entity2) };

				bool isDynamic1 = rigidBody1 && rigidBody1->getFlags().test(SIMULATE_DYNAMICS);
				bool isDynamic2 = rigidBody2 && rigidBody2->getFlags().test(SIMULATE_DYNAMICS);

				//Component::ColliderComponent* collider1{ globalECS.GetComponent<Component::ColliderComponent>(evnt.entity1) };
				//Component::ColliderComponent* collider2{ globalECS.GetComponent<Component::ColliderComponent>(evnt.entity2) };

				/*Collision::AABBRect region1{ Collision::GetAABB(*(collider1->collider), collider1->collider->position) };
				Collision::AABBRect region2{ Collision::GetAABB(*(collider2->collider), collider2->collider->position) };*/

				//if (!bounds.contains(region1) && !bounds.contains(region2)) continue;

				if (!isDynamic1 && !isDynamic2) continue;

				PositionCorrection(evnt, globalECS);
				ApplyImpulse(evnt, globalECS);
			}

			/*PHYSICS SIM, UPDATE POSITION*/
			for (CrashoutsEngine::Entity& entity : globalEntities) {
				if (!globalECS.HasComponent<Component::TransformComponent>(entity)) continue;
				Component::TransformComponent* transform{ globalECS.GetComponent<Component::TransformComponent>(entity) };
				Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };

				// -------------------------------------------------------
				// CASE A: DYNAMIC (Physics Drives Transform)
				// -------------------------------------------------------
				if (rigidBody->getFlags().test(SIMULATE_DYNAMICS)) {

					UpdatePhysics(transform, rigidBody, deltaTime);

					rigidBody->setVelocity(rigidBody->GetVelocity() * 0.999f);
					rigidBody->angularVelocity *= 0.999f;

					if (globalECS.HasComponent<Component::ColliderComponent>(entity)) {
						Component::ColliderComponent* c{ globalECS.GetComponent<Component::ColliderComponent>(entity) };

						/*if (rigidBody->getMass() && rigidBody->inertia) {
							c->collider->updateRotation(rigidBody->orientation);
						}*/
						c->collider->UpdateCollider(transform->position);
					}

					// Sync children so they "ride" the dynamic parent
					SyncHierarchyPhysics(entity, globalECS);
				}
				// -------------------------------------------------------
				// CASE B: KINEMATIC / STATIC (Transform Drives Physics)
				// -------------------------------------------------------
				else {
					// The Hierarchy System or Scripts moved the Transform. 
					// We just need to ensure the Collider follows.
					if (globalECS.HasComponent<Component::ColliderComponent>(entity)) {
						auto* c = globalECS.GetComponent<Component::ColliderComponent>(entity);

						// Sync Collider Rotation to Transform Rotation
						// (Your RigidBody orientation might be stale if you rotated Transform via script)
						rigidBody->setRotation(transform->rotation);
						c->collider->updateRotation(transform->rotation);

						// Sync Collider Position
						c->collider->UpdateCollider(transform->position);
					}

					// --- ADD THIS ---
					// Ensure children (like Spikes on a Platform) Update their colliders too!
					SyncHierarchyPhysics(entity, globalECS);
				}
			}
		}

		collisionEvents.clear(); /*Fully consumed Collision Pairs*/

	}

}
