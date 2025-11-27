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
    }
}
