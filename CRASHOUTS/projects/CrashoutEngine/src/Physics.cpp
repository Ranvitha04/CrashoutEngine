/*!
@file   Physics.cpp
@author Renfred (RTIS) (72%), Renfred Lee (11%), tristan (17%)
@date   08/09/2025

@brief  Contains Definitions for Physics runSystem function and the RigidBodyComponent

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "physicsmanager.h"
#include "jobsystem.h"
#include "frameratecontroller.h"
#include "performanceSystem.h"
#include "performanceViewer.h"
#include "hierarchycomponent.h"
#include "activecomponent.h"
#include "collidereventmanager.h"
#include "namecomponent.h"
#include "log.h"
#include <cstdio>
#include "CEMath.h"
#include "observer.h"
#include "glapp.h"
#include "camera.h"
#include "sceneasset.h"
#include <windows.h>

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
		//flags.set(Physics::SIMULATE_DYNAMICS);
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

	f32 Cross(CEVec3 const& a, CEVec3 const& b) {
		return a.x * b.y - a.y * b.x;
	}
}

namespace Physics {

	// Setters and Getters
	void PhysicsHandler::SetGravity(const CEVec3& g) { sGravity = g; }
	CEVec3 PhysicsHandler::GetGravity() { return sGravity; }
	void PhysicsHandler::SetLinearDamping(float c) { sLinDamp = c; }
	float PhysicsHandler::GetLinearDamping() { return sLinDamp; }

	// Helper to recursively Update child transforms and colliders after physics step
	void SyncHierarchyPhysics(CrashoutsEngine::Entity parent, CrashoutsEngine::ECS& ecs)
	{
		if (!ecs.HasComponent<Component::ChildrenComponent>(parent)) return;
		if (!ecs.HasComponent<Component::TransformComponent>(parent)) return;

		auto* parentTransform = ecs.GetComponent<Component::TransformComponent>(parent);
		auto* childrenComp = ecs.GetComponent<Component::ChildrenComponent>(parent);

		const float DEG_TO_RAD = 0.0174532925f;
		float parentRad = parentTransform->rotation * DEG_TO_RAD;
		float s = std::sin(parentRad);
		float c = std::cos(parentRad);

	for (CrashoutsEngine::Entity child : childrenComp->children)
	{
		if (ecs.HasComponent<Component::RigidBodyComponent>(child)) {
			continue;
		}
		if (!ecs.HasComponent<Component::TransformComponent>(child)) continue;
		if (!ecs.HasComponent<Component::ParentComponent>(child))    continue;

		auto* childTransform = ecs.GetComponent<Component::TransformComponent>(child);
		auto* rel = ecs.GetComponent<Component::ParentComponent>(child);

		// 1) SCALE (match hierarchy.cpp)
		childTransform->size.x = parentTransform->size.x * rel->localScale.x;
		childTransform->size.y = parentTransform->size.y * rel->localScale.y;

		// 2) ROTATION (degrees, additive)
		childTransform->rotation = parentTransform->rotation + rel->localRot;

		// 3) POSITION (match hierarchy.cpp: scale local offset by parent size, then rotate)
		float scaledOffsetX = rel->localPos.x * parentTransform->size.x;
		float scaledOffsetY = rel->localPos.y * parentTransform->size.y;

		float rotatedX = (c * scaledOffsetX) - (s * scaledOffsetY);
		float rotatedY = (s * scaledOffsetX) + (c * scaledOffsetY);

		childTransform->position.x = parentTransform->position.x + rotatedX;
		childTransform->position.y = parentTransform->position.y + rotatedY;
		childTransform->position.z = parentTransform->position.z + rel->localPos.z;

		// 4) Update child collider too (position + rotation)
		if (ecs.HasComponent<Component::ColliderComponent>(child)) {
			auto* childCol = ecs.GetComponent<Component::ColliderComponent>(child);
			childCol->collider->UpdateCollider(childTransform->position);
			childCol->collider->UpdateRotation(childTransform->rotation);
		}

		// recurse
		SyncHierarchyPhysics(child, ecs);
	}
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
		f32 mu = std::sqrtf(std::powf(staticFric1, 2.f));
		CEVec3 frictionImpulse{};
		if (std::abs(jt) < j * mu) {
			frictionImpulse = jt * tangent;
		}
		else {
			f32 dynamicFric{ std::sqrtf(std::powf(dynamicFric1, 2.f)) };
			frictionImpulse = -j * tangent * dynamicFric;
		}

		if (rigidBody1) rigidBody1->setVelocity(rigidBody1->GetVelocity() - frictionImpulse * invMass1);
		if (rigidBody2) rigidBody2->setVelocity(rigidBody2->GetVelocity() + frictionImpulse * invMass2);

	}

	void ApplyRotationalImpulse(Collision::CollisionEvent const& event, CrashoutsEngine::ECS& globalECS, float dt) {
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

		CEVec3 penetrationTo1{ event.colManifold.contactPoints[0] - collider1->collider->position };
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

		float e{ std::max(coeff1, coeff2) }; /*COEFFICIENT OF RESTITUTION*/

		// Calculate how much velocity gravity added this frame.
		float gravityLen = CEMath::Length(PhysicsHandler::GetGravity()); // Assuming you have a Length() helper
		float threshold = gravityLen * dt * 2.0f; // * 2.0f provides a safety buffer
		// If impact speed is tiny, kill restitution
		if (std::abs(velAlongNormal) < threshold) {
			e = 0.0f;
		}

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

		// FRICTION SOLVER

		// Re-calculate Relative Velocity on top of newly applied Normal Impulse
		vel1 = rigidBody1 ? rigidBody1->GetVelocity() : CEVec3{};
		vel2 = rigidBody2 ? rigidBody2->GetVelocity() : CEVec3{};
		angVel1 = rigidBody1 ? rigidBody1->angularVelocity : 0.0f;
		angVel2 = rigidBody2 ? rigidBody2->angularVelocity : 0.0f;

		CEVec3 angVelP1{ CEVec3{-1.f * angVel1 * penetrationTo1.y, angVel1 * penetrationTo1.x, 0.f} };
		CEVec3 angVelP2{ CEVec3{-1.f * angVel2 * penetrationTo2.y, angVel2 * penetrationTo2.x, 0.f} };

		CEVec3 newRelVel = (vel2 + angVelP2) - (vel1 + angVelP1);

		// Find Tangent Vector (The direction of the slide)
		// Tangent is perpendicular to Normal, in the direction of velocity
		CEVec3 tangent = newRelVel - CEMath::Dot(newRelVel, event.colManifold.normal) * event.colManifold.normal;
		float tangentLen = CEMath::Length(tangent);

		// If barely sliding, skip friction
		if (tangentLen < 0.001f) return;

		// Normalize tangent
		tangent = tangent * (1.0f / tangentLen);

		// Solve for Friction Impulse Magnitude (jt)
		// This is similar to the Normal Impulse (j) math, but using the Tangent vector
		float jtNumerator = -CEMath::Dot(newRelVel, tangent);

		float r1CrossT = Cross(penetrationTo1, tangent);
		float r2CrossT = Cross(penetrationTo2, tangent);

		float frictionCrossSum = (r1CrossT * r1CrossT * invInertia1) + (r2CrossT * r2CrossT * invInertia2);

		float jt = jtNumerator / (sumInvMass + frictionCrossSum);

		// Coulomb's Law (Clamping)
		// Friction cannot be stronger than the Normal Force * Coefficient
		// If it is, we switch from Static to Dynamic friction.

		// Combine friction coefficients (Average is a common approximation)
		float f1 = rigidBody1 ? rigidBody1->getStaticFriction() : 0.5f;
		float f2 = rigidBody2 ? rigidBody2->getStaticFriction() : 0.5f;
		float muStatic = (f1 + f2) * 0.5f;

		float d1 = rigidBody1 ? rigidBody1->getDynamicFriction() : 0.3f;
		float d2 = rigidBody2 ? rigidBody2->getDynamicFriction() : 0.3f;
		float muDynamic = (d1 + d2) * 0.5f;

		CEVec3 frictionImpulseVec;

		// Determine if we hold still (Static) or slide (Dynamic)
		if (std::abs(jt) < j * muStatic) {
			// Static Friction: We can generate enough force to stop the slide completely.
			frictionImpulseVec = jt * tangent;
		}
		else {
			// Dynamic Friction: The slide is too strong. We apply a constant drag force.
			// Force = NormalForce * DynamicCoeff
			frictionImpulseVec = -j * tangent * muDynamic;
		}

		// Apply Friction Impulse 
		if (rigidBody1) {
			// Apply Linear Friction Drag
			rigidBody1->setVelocity(rigidBody1->GetVelocity() - frictionImpulseVec * invMass1);

			// Apply Angular Friction Torque
			// We use Cross Product to convert the linear friction force into torque
			rigidBody1->angularVelocity += -Cross(penetrationTo1, frictionImpulseVec) * invInertia1;
		}

		if (rigidBody2) {
			rigidBody2->setVelocity(rigidBody2->GetVelocity() + frictionImpulseVec * invMass2);
			rigidBody2->angularVelocity += Cross(penetrationTo2, frictionImpulseVec) * invInertia2;
		}
	}
	/*Main Physics Update*/
	void PhysicsHandler::Update(double fixedDT) {

		CrashoutsEngine::ECS& globalECS = *(CrashoutsEngine::ECS::getInstance());

		const std::vector<CrashoutsEngine::Entity>& globalEntities = globalECS.GetComponentArray<Component::RigidBodyComponent>().indexToEntity;

		bool isSimulating = !sPaused;// || sStepOnce;

		//double simDt = isSimulating ? fixedDT : 0.0;

		const int SUB_STEPS = FrameRateController::GetInstance().GetMaxPhysicsSteps();
		double subDt = (isSimulating ? fixedDT : 0.0) / SUB_STEPS;
		double fullDt = isSimulating ? fixedDT : 0.0;

		/*Fixed step, accumulator will reach the target fixed dt and then execute physics simulation*/
		for (int sub{}; sub < SUB_STEPS; ++sub) {

			CrashoutsEngine::JobSystem::Get().ParallelFor((unsigned int)globalEntities.size(), [&](unsigned int start, unsigned int end) {
				for (unsigned int j = start; j < end; ++j) {
					const CrashoutsEngine::Entity& entity = globalEntities[j];
					if (!IsEntityActive(globalECS, entity)) continue;

					Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };
					//if (!rigidBody->getFlags().test(SIMULATE_DYNAMICS) || !rigidBody->getMass()) continue;
					if (!rigidBody || !rigidBody->getMass()) continue;

					bool isCCD = false;
					if (globalECS.HasComponent<Component::ColliderComponent>(entity)) {
						isCCD = globalECS.GetComponent<Component::ColliderComponent>(entity)->collider->isContinuous;
					}

					// If NOT continuous, only do physics on the FIRST sub-step, using the FULL dt!
					if (!isCCD) {
						if (sub > 0) continue; // Skip sub-steps 1, 2, and 3 completely!

						if (isSimulating) {
							ForceManager& fm = rigidBody->getForceManager();
							CEVec3 forces = fm.ResolveForces((f32)fullDt, rigidBody->getMass());
							CEVec3 gravityForce = rigidBody->getMass() * sGravity * rigidBody->gravityScale;
							rigidBody->addForce(forces + gravityForce);
							rigidBody->UpdatePhysics(fullDt); // Uses fullDt!
						}
					}
					else {
						// IT IS A FAST OBJECT! Do physics EVERY sub-step using subDt!
						if (isSimulating) {
							ForceManager& fm = rigidBody->getForceManager();
							CEVec3 forces = fm.ResolveForces((f32)subDt, rigidBody->getMass());
							CEVec3 gravityForce = rigidBody->getMass() * sGravity * rigidBody->gravityScale;
							rigidBody->addForce(forces + gravityForce);
							rigidBody->UpdatePhysics(subDt); // Uses subDt!
						}
					}

					// Compute Inertia
					if (rigidBody->invInertia == 0.0f) {
						if (globalECS.HasComponent<Component::ColliderComponent>(entity)) {
							auto* colComp = globalECS.GetComponent<Component::ColliderComponent>(entity);

							// Safety Check
							if (colComp && colComp->collider) {
								float inertia = colComp->collider->CalculateInertia(rigidBody->getMass());

								rigidBody->setInertia(inertia);
							}
						}
					}

				}
				});

			/*PHYSICS SIM, UPDATE POSITION*/
			CrashoutsEngine::JobSystem::Get().ParallelFor((unsigned int)globalEntities.size(), [&](unsigned int start, unsigned int end) {
				for (unsigned int j = start; j < end; ++j) {
					const CrashoutsEngine::Entity& entity = globalEntities[j];
					if (!IsEntityActive(globalECS, entity)) continue;

					if (!globalECS.HasComponent<Component::TransformComponent>(entity)) continue;
					Component::TransformComponent* transform{ globalECS.GetComponent<Component::TransformComponent>(entity) };
					Component::RigidBodyComponent* rigidBody{ globalECS.GetComponent<Component::RigidBodyComponent>(entity) };

					bool isCCD = false;
					if (globalECS.HasComponent<Component::ColliderComponent>(entity)) {
						isCCD = globalECS.GetComponent<Component::ColliderComponent>(entity)->collider->isContinuous;
					}

					// -------------------------------------------------------
					// DYNAMIC (Physics Drives Transform)
					// -------------------------------------------------------
					if (rigidBody->getMass()) {

						if (!isCCD && sub > 0) continue; // Skip normal objects on extra sub-steps!

						f32 currentDelta = static_cast<float>(!isCCD ? fullDt : subDt);

						if (isSimulating) {
							// 1. Apply Velocity to Position ONCE
							transform->position += rigidBody->GetVelocity() * currentDelta;

							// 2. Rotate Orientation
							if (rigidBody->invInertia > 0.0f) {
								float angularChange = rigidBody->angularVelocity * (180.f / CEMath::PI) * currentDelta;
								rigidBody->orientation += angularChange;
								transform->rotation = rigidBody->orientation;
							}

							// 3. Apply Drag & Clamping
							rigidBody->setVelocity(rigidBody->GetVelocity() * 0.999f);
							rigidBody->angularVelocity *= 0.98f;    // Angular Drag

							if (rigidBody->angularVelocity > 15.0f) rigidBody->angularVelocity = 15.0f;
							if (rigidBody->angularVelocity < -15.0f) rigidBody->angularVelocity = -15.0f;

							if (std::abs(rigidBody->angularVelocity) < 0.1f) {
								rigidBody->angularVelocity = 0.0f;
							}
						}
						else {
							// STOPPED: TRANSFORM (UI) DRIVES PHYSICS
							rigidBody->orientation = transform->rotation;
							rigidBody->angularVelocity = 0.0f;
						}

						// 4. Sync Colliders
						if (globalECS.HasComponent<Component::ColliderComponent>(entity)) {
							Component::ColliderComponent* c{ globalECS.GetComponent<Component::ColliderComponent>(entity) };
							if (c->collider != nullptr) {
								c->collider->UpdateCollider(transform->position);
								c->collider->UpdateRotation(transform->rotation);
							}
						}

						// Sync children
						SyncHierarchyPhysics(entity, globalECS);
					}
				}
				});

			// Detect collisions
			{
				FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Collision");
				// Determine state for collision
				Collision::EngineState physState = isSimulating ? Collision::EngineState::PLAY : Collision::EngineState::STOP;

				auto& colSys = Collision::CollisionHandler::GetInstance();
				colSys.Update(subDt, physState, sub);
			}

			// Get collision data from handler
			const auto& events = Collision::CollisionHandler::GetInstance().GetFrameCollisions();

			{
				FrameProfiler::Scoped s(PerformanceSystem::Get().GetProfiler(), "Impulse");
				/*IMPULSE RESOLUTION*/
				for (Collision::CollisionEvent const& evnt : events) {

					// Check C# logic via the manager
					bool allowCollision = CrashoutsEngine::CollisionEventManager::GetInstance().CheckPreSolve(evnt.entity1, evnt.entity2);

					// Check the other way too
					if (allowCollision) {
						allowCollision = CrashoutsEngine::CollisionEventManager::GetInstance().CheckPreSolve(evnt.entity2, evnt.entity1);
					}

					if (!allowCollision) continue;

					Component::RigidBodyComponent* rigidBody1{ globalECS.GetComponent<Component::RigidBodyComponent>(evnt.entity1) };
					Component::RigidBodyComponent* rigidBody2{ globalECS.GetComponent<Component::RigidBodyComponent>(evnt.entity2) };

					Component::ColliderComponent* col1 = globalECS.GetComponent<Component::ColliderComponent>(evnt.entity1);
					Component::ColliderComponent* col2 = globalECS.GetComponent<Component::ColliderComponent>(evnt.entity2);

					bool isContinuous1 = col1 ? col1->collider->isContinuous : false;
					bool isContinuous2 = col2 ? col2->collider->isContinuous : false;

					// On sub-steps 1, 2, and 3, IGNORE collisions between two normal objects!
					// They already checked for collisions on sub-step 0.
					if (sub > 0 && !isContinuous1 && !isContinuous2) {
						continue;
					}

					if ((col1 && col1->collider->isTrigger) || (col2 && col2->collider->isTrigger)) {
						continue;
					}

					bool isDynamic1 = rigidBody1 && rigidBody1->getMass();
					bool isDynamic2 = rigidBody2 && rigidBody2->getMass();

					if (!isDynamic1 && !isDynamic2) continue;

					if (isSimulating) {
						PositionCorrection(evnt, globalECS);
						ApplyRotationalImpulse(evnt, globalECS, (float)subDt);
					}
				}
			}
		}
	}
}
