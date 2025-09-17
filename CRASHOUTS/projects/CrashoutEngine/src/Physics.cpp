/*!
@file       Physics.cpp
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Contains Definitions for Physics runSystem function and the RigidBodyComponent

//__________________________________________________________________________*/

#include "managers/physicsmanager.h"

#include "math/CEMath.h"

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

		clearForceAccumulator(); /*Reset forces acting upon object in an interation*/

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
		component->posX += trans.x;
		component->posY += trans.y;
		component->posZ += trans.z;
	}

	void operator-=(Component::TransformComponent* component, CEVec3 const& trans) {
		component->posX -= trans.x;
		component->posY -= trans.y;
		component->posZ -= trans.z;
	}
}

namespace Physics {

	/*UPDATE BOTH THE RIGIDBODY AND TRANSFORM COMPONENT*/
	void PhysicsHandler::updatePhysics(Component::TransformComponent *transformComp, Component::RigidBodyComponent *rigidBodyComp, double deltaTime) {
		f32 newDelta{ static_cast<float>(deltaTime) };
		rigidBodyComp->updatePhysics(deltaTime); /*UPDATE CURRENT VELOCITY AND RESET NET FORCE*/
		CEVec3 currentVel{ rigidBodyComp->getVelocity() };
		transformComp->posX += currentVel.x * newDelta;
		transformComp->posY += currentVel.y * newDelta;
		transformComp->posZ += currentVel.z * newDelta;

		// Debug stepping
		if (sOverrideDt) deltaTime = sDebugDt;
		if (sPaused && !sStepOnce) return;
		sStepOnce = false;

		collisionEvents.clear();
		// ... your broadphase/narrowphase that fills collisionEvents and resolves ...

		// Optional: summarize events to the logger
		for (auto const& ev : collisionEvents) {
			LOGI(Physics, "Collision pair registered"); // keep it light; UI shows details
		}
	}

	void PhysicsHandler::applyImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS) {
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
	void PhysicsHandler::update(CrashoutsEngine::ECS& globalECS, std::vector<CrashoutsEngine::Entity>& globalEntities, double deltaTime){

		/*UPDATE FORCES*/
		for (CrashoutsEngine::Entity& entity : globalEntities) {
			if (!globalECS.HasComponent<Component::RigidBodyComponent>(entity)) continue;
			Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };

			/*APPLY ANY FORCE GENERATOR HERE, EVERY ENTITY IS AFFECTED*/

			//LOOSE FRICTION IMPLEMENTATION
			if ( CEMath::LengthSq(rigidBody->getVelocity()) > 0.f) {
				rigidBody->addForce(-(rigidBody->getVelocity() * 1.5f * rigidBody->getMass()));
			}

			rigidBody->addForce(CEVec3{ 0.f, -4.f, 0.f }); /*GRAVITY*/

		}

		/*DETECT COLLISION AND UPDATE COLLISION EVENTS MEMBER*/
		for (CrashoutsEngine::Entity& entity1 : globalEntities) {

			if (!globalECS.HasComponent<Component::ColliderComponent>(entity1) || !globalECS.HasComponent<Component::TransformComponent>(entity1) || !globalECS.HasComponent<Component::RigidBodyComponent>(entity1)) continue;
			for (CrashoutsEngine::Entity const& entity2 : globalEntities) {
				if (entity1 == entity2) continue;
				if (!globalECS.HasComponent<Component::ColliderComponent>(entity2) || !globalECS.HasComponent<Component::TransformComponent>(entity2) || !globalECS.HasComponent<Component::RigidBodyComponent>(entity2)) continue;
				
				Component::ColliderComponent *collider1{ globalECS.GetComponent<Component::ColliderComponent>(entity1) };
				Component::TransformComponent* transform1{ globalECS.GetComponent<Component::TransformComponent>(entity1) };
				Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(entity1) };

				Component::ColliderComponent *collider2{ globalECS.GetComponent<Component::ColliderComponent>(entity2) };
				Component::TransformComponent* transform2{ globalECS.GetComponent<Component::TransformComponent>(entity2) };
				Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(entity2) };

				/*CHECK FOR COLLISION and call collision function*/
				Collision::CollisionManifold collisionManifold{};
				int tolerance{ 2 };

				Component::TransformComponent newTransform1{*transform1};
				Component::RigidBodyComponent newRigidBody1{*rigidBody1};	

				newRigidBody1.setVelocity(newRigidBody1.getVelocity() / static_cast<float>(tolerance));

				for (int i{}; i < tolerance; ++i) {
					collisionManifold = collisionHandler.callCollFunct(collider1->collider->type, collider2->collider->type)(
						Collision::CollisionProps{
							collider1->collider,
							CEVec3{},
							CEVec3{newTransform1.posX, newTransform1.posY, newTransform1.posZ}
						},
						Collision::CollisionProps{
							collider2->collider,
							CEVec3{},
							CEVec3{transform2->posX, transform2->posY, transform2->posZ}
					});
					if (collisionManifold.collisionOccured) {
						*transform1 = newTransform1;
						break;
					}
					updatePhysics(&newTransform1, &newRigidBody1, deltaTime);
				}

				/*STORE COLLISION EVENT IN BUFFER*/
				collisionEvents.push_back(Collision::CollisionEvent{
					collisionManifold,
					entity1,
					entity2
				});

			}
		}

		/*RESOLVE COLLISION*/
		for ( Collision::CollisionEvent const &collEvent: collisionEvents ) {
			if (!globalECS.HasComponent<Component::RigidBodyComponent>(collEvent.entity1) || !globalECS.HasComponent<Component::RigidBodyComponent>(collEvent.entity2)) continue;
			applyImpulse(collEvent, globalECS);
		}

		/*PHYSICS SIM, UPDATE POSITION*/
		for (CrashoutsEngine::Entity& entity : globalEntities) {
			if (!globalECS.HasComponent<Component::RigidBodyComponent>(entity) || !globalECS.HasComponent<Component::TransformComponent>(entity)) continue;
			Component::TransformComponent* transform{ globalECS.GetComponent<Component::TransformComponent>(entity) };
			Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };

			updatePhysics(transform, rigidBody, deltaTime);

			//LOGI(Physics, std::to_string(transform->posY));

		}

		/*TODO, FIND A MORE EFFICIENT WAY/REPLACE WITH MESSAGING SYSTEM FOR COLLISION EVENTS*/
		collisionEvents.clear();
	}

}
