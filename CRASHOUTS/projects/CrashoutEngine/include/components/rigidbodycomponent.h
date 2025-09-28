/*!
@file       rigidbodycomponent.h
@author     miratristan.d@digipen.edu
@date       08/09/2025
@brief      Declaration for the class RigidBodyComponent, used for Phyics system

//__________________________________________________________________________*/

#pragma once

namespace Component {
	class RigidBodyComponent {
		private:
			CEVec3 mVelocity;
			CEVec3 mMaximumVel;
			f32 mMass;
			CEVec3 mNetForce;
			f32 mRestitutionCoeff; /*Measure of elasticity*/
		public:

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

			/*Constructor*/
			RigidBodyComponent(CEVec3 maximumVel = CEVec3{}, f32 mass = 0.f, f32 restitutionCoeff = 0.f);

			/*Member Functions*/
			f32 getInverseMass();
			f32 getMass();
			void setMass(f32 newMass);
			void addForce(CEVec3 force);
			void clearForceAccumulator();
			void updatePhysics(double dt);
			CEVec3 getVelocity();
			void setVelocity(CEVec3 const& newVel);
			f32 getCoeff();
	};
}