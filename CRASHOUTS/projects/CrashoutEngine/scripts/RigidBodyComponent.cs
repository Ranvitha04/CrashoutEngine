/*!
@file       rigidbodycomponent.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      rigidbody component for physics interactions

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddRigidBody", "RemoveRigidBody", "EntityHasRigidBody")]
    public class RigidBodyComponent
    {
        public uint EntityID;

        public void AddForce(CEVec3 force)
        {
            InternalAddForce(EntityID, force.x, force.y, force.z);
        }

        public void SetDynamic(bool dynamic)
        {
            InternalSetDynamic(EntityID, dynamic);
        }

        public float Mass
        {
            get
            {
                InternalGetMass(EntityID, out float mass);
                return mass;
            }
            set
            {
                InternalSetMass(EntityID, ref value);
            }
        }

        public CEVec3 LinearVelocity
        {
            get
            {
                InternalGetVelocity(EntityID, out CEVec3 vel);
                return vel;
            }
            set
            {
                InternalSetVelocity(EntityID, ref value);
            }
        }

        public float GravityScale
        {
            get
            {
                InternalGetGravityScale(EntityID, out float scale);
                return scale;
            }
            set
            {
                InternalSetGravityScale(EntityID, ref value);
            }
        }

        public float AngularVelocity
        {
            get
            {
                InternalGetAngularVelocity(EntityID, out float angVel);
                return angVel;
            }
            set
            {
                InternalSetAngularVelocity(EntityID, ref value);
            }
        }

        public float Rotation
        {
            set
            {
                InternalSetRotation(EntityID, value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void InternalGetAngularVelocity(uint entityID, out float angularVelocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetAngularVelocity(uint entityID, ref float angularVelocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalGetGravityScale(uint entityID, out float scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetGravityScale(uint entityID, ref float scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalAddForce(uint entityID, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float InternalGetMass(uint entityID, out float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetMass(uint entityID, ref float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void InternalGetVelocity(uint entityID, out CEVec3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetVelocity(uint entityID, ref CEVec3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetDynamic(uint entityID, bool dynamic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetRotation(uint entityID, float rotation);
    }
}
