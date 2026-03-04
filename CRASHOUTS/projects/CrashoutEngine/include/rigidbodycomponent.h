/*!
@file       rigidbodycomponent.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Declaration for the class RigidBodyComponent, used for Physics system

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once
#include <CEVec3.h>
#include <CETypes.h>
#include <bitset>
#include "physicsutils.h"

namespace Component {
	class RigidBodyComponent {
	private:
		CEVec3 mVelocity;
		CEVec3 mMaximumVel;
		f32 mMass{};
		CEVec3 mNetForce;
		f32 mRestitutionCoeff{}; /*Measure of elasticity*/
		Physics::ForceManager fm{};
		std::bitset<8> flags{ 0b0000'0000 };

	public:
		f32 staticFriction{ 0.5f };
		f32 dynamicFriction{ 0.5f };
		f32 inertia{ };
		f32 invInertia{ };
		/*Rotational Properties*/
		f32 angularVelocity{};
		f32 orientation{};
		f32 torque{};
		f32 gravityScale = 1.0f;

		struct ReflectionType {
			CEVec3 velocity{};
			CEVec3 max_velocity{};
			f32 mass{ 0.f };
			f32 restitution{ 0.f };
		};

		RigidBodyComponent(const ReflectionType& r)
			: RigidBodyComponent(r.max_velocity, r.mass, r.restitution) {
			setVelocity(r.velocity);
		}

		ReflectionType reflection() const {
			return ReflectionType{
				.velocity = mVelocity,
				.max_velocity = mMaximumVel,
				.mass = mMass,
				.restitution = mRestitutionCoeff
			};
		}

		void UpdatePhysicsWithRotation(double dt);

		/*Constructor*/
		RigidBodyComponent(CEVec3 maximumVel = CEVec3{}, f32 mass = 0.f, f32 restitutionCoeff = 0.f);

		/*Inline functions*/
		inline f32 getStaticFriction() {
			return staticFriction;
		}

		inline f32 getDynamicFriction() {
			return dynamicFriction;
		}

		inline void setRotation(f32 newRotation) {
			orientation = newRotation;
		}

		inline f32 getRotation() {
			return orientation;
		}

		inline void setAngularVel(f32 newAngVel) {
			angularVelocity = newAngVel;
		}

		inline f32 getAngularVel() {
			return angularVelocity;
		}

		inline void setInertia(f32 newInertia) {
			inertia = newInertia;
			if (inertia) {
				invInertia = 1.f / inertia;
			} else {
				invInertia = 0.f;
			}
		}

		inline f32 getMass() {
			return mMass;
		}

		inline std::bitset<8>& getFlags() { return flags; }

		/*Member Functions*/
		f32 GetInverseMass();
		void setMass(f32 newMass);
		void addForce(CEVec3 force);
		void clearForceAccumulator();
		void UpdatePhysics(double dt);
		void setMaxVelocity(CEVec3 const& newVel);
		CEVec3 getMaxVelocity();
		CEVec3 GetVelocity();
		void setRestitution(f32 restitution);
		void setVelocity(CEVec3 const& newVel);
		f32 GetCoeff();
		Physics::ForceManager& getForceManager();
	};
}