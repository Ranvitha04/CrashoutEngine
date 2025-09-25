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

			/*Constructor*/
			RigidBodyComponent(CEVec3 maximumVel = CEVec3{}, f32 mass = 0.f, f32 restitutionCoeff = 0.f);

			/*Member Functions*/
			f32 getInverseMass();
			f32 getMass();
			void addForce(CEVec3 force);
			void clearForceAccumulator();
			void updatePhysics(double dt);
			CEVec3 getVelocity();
			void setVelocity(CEVec3 const& newVel);
			f32 getCoeff();
	};
}