/*!
@file       Physics.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Contains Definitions for Physics runSystem function and the RigidBodyComponent

//__________________________________________________________________________*/

#include "managers/physicsmanager.h"

#include "math/CEMath.h"
#include "messaging/observer.h"


namespace Component {
	
	f32 RigidBodyComponent::getInverseMass() {
		if (!mMass) return 0.f; /*Prevent division by 0*/
		return 1.f / mMass;
	}

	f32 RigidBodyComponent::getMass() {
		return mMass;
	}

	void RigidBodyComponent::addForce(CEVec3 force) {
		mNetForce += force;
	}
	void RigidBodyComponent::clearForceAccumulator() {
		mNetForce = CEVec3{};
	}
	
	RigidBodyComponent::RigidBodyComponent(CEVec3 maximumVel, f32 mass, f32 restitutionCoeff) 
		: mVelocity{ CEVec3{} }, mMaximumVel{ maximumVel }, mMass{ mass }, mNetForce{ CEVec3{} }, mRestitutionCoeff{restitutionCoeff} { /*EMPTY BY DESIGN*/ }

	void RigidBodyComponent::updatePhysics(double dt) {

		/*Prevent further calculation (Treat 0 mass objects as static (Wont be affected by external forces but still have colliders/collision resolution))*/
		if (mMass == 0.f) {
			return;
		}

		/*	
			F(Resultant Force) = ma; 
			FIND a = F(Resultant Force) / m 
		*/
		CEVec3 acceleration = mNetForce * getInverseMass();

		/*TODO PREVENT VELOCITY FROM GOING FURTHER FROM MAX velocity*/
		mVelocity += static_cast<f32>(dt) * acceleration;

		clearForceAccumulator(); /*Reset forces acting upon object in an iteration*/

	}

	CEVec3 RigidBodyComponent::getVelocity() {
		return mVelocity;
	}

	void RigidBodyComponent::setVelocity(CEVec3 const &newVel) {
		this->mVelocity = newVel;
	}

	f32 RigidBodyComponent::getCoeff() {
		return mRestitutionCoeff;
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
}

namespace Physics {

	void handleCollisionMessages(Collision::CollisionMessage& collEvent) {

		std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> collPair{ collEvent.getCollisionPair() };
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());
		std::pair<CrashoutsEngine::Entity, CrashoutsEngine::Entity> colPair{ collEvent.getCollisionPair() };
		if (!globalECS.HasComponent<Component::RigidBodyComponent>(colPair.first) || !globalECS.HasComponent<Component::RigidBodyComponent>(colPair.second)) return;
		applyImpulse(Collision::CollisionEvent{collEvent.getCollisionManifold(), colPair.first, colPair.second}, globalECS);

	}

	PhysicsHandler::PhysicsHandler() : Messaging::Observer{ "Physics" } {
		attachHandler(Messaging::getCollisionMsgID(), (Messaging::HANDLER)handleCollisionMessages);
	}

	/*UPDATE BOTH THE RIGIDBODY AND TRANSFORM COMPONENT*/
	void PhysicsHandler::updatePhysics(Component::TransformComponent *transformComp, Component::RigidBodyComponent *rigidBodyComp, double deltaTime) {
		f32 newDelta{ static_cast<float>(deltaTime) };
		rigidBodyComp->updatePhysics(deltaTime); /*UPDATE CURRENT VELOCITY AND RESET NET FORCE*/
		CEVec3 currentVel{ rigidBodyComp->getVelocity() };
		transformComp->position.x += currentVel.x * newDelta;
		transformComp->position.y += currentVel.y * newDelta;
		transformComp->position.z += currentVel.z * newDelta;

		// Debug stepping
		if (sOverrideDt) deltaTime = sDebugDt;
		if (sPaused && !sStepOnce) return;
		sStepOnce = false;

		//collisionEvents.clear(); /*Commented out for now as bug with the collision events
		// ... your broadphase/narrowphase that fills collisionEvents and resolves ...

		// Optional: summarize events to the logger
		for (auto const& ev : collisionEvents) {
			//LOGI(Physics, "Collision pair registered"); // keep it light; UI shows details
		}
	}

	void applyImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS) {
		Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(event.entity1) };
		Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity1) };

		Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(event.entity2) };
		Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(event.entity2) };

		/*SOLVE FOR LINEAR VEL*/
		f32 invMass1{ rigidBody1->getInverseMass() };
		f32 invMass2{ rigidBody2->getInverseMass() };
		f32 sumInvMass{ invMass1 + invMass2 };
		f32 totalMass{ rigidBody1->getMass() + rigidBody2->getMass()};
		if (sumInvMass == 0.f) {
			return;
		}

		/*MOVE OBJECT ALONG COLLISION NORMAL BEFORE CALCULATING IMPULSE TO CHANGE VELOCITY*/
		transform1 -= event.colManifold.normal * event.colManifold.depth * (invMass1 / sumInvMass);
		transform2 += event.colManifold.normal * event.colManifold.depth * (invMass2 / sumInvMass);
		

		/*RELATIVE VELOCITY*/
		CEVec3 relVel{ rigidBody2->getVelocity() - rigidBody1->getVelocity() };

		//MOVING AWAY
		if (CEMath::Dot(relVel, event.colManifold.normal) > 0.f) {
			return;
		}

		float e{ std::min(rigidBody1->getCoeff(), rigidBody2->getCoeff())}; /*COEFFICIENT OF RESTITUTION*/
		float numerator{ -(1.f + e) * CEMath::Dot(relVel, event.colManifold.normal) };

		float j = numerator / sumInvMass;

		CEVec3 impulse{ event.colManifold.normal * j };
		
		rigidBody1->setVelocity(rigidBody1->getVelocity() -= impulse * invMass1);
		rigidBody2->setVelocity(rigidBody2->getVelocity() += impulse * invMass2);

	}

	/*Main Physics update*/
	void PhysicsHandler::update(std::vector<CrashoutsEngine::Entity>& globalEntities, double deltaTime){
		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		/*PHYSICS SIM, UPDATE POSITION*/
		for (CrashoutsEngine::Entity& entity : globalEntities) {
			if (!globalECS.HasComponent<Component::RigidBodyComponent>(entity) || !globalECS.HasComponent<Component::TransformComponent>(entity)) continue;
			Component::TransformComponent* transform{ globalECS.GetComponent<Component::TransformComponent>(entity) };
			Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };

			updatePhysics(transform, rigidBody, deltaTime);

		}

	}

}
