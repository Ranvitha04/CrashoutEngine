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

        public float GetMass()
        {
            return InternalGetMass(EntityID);
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
        private static extern float InternalGetMass(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void InternalGetVelocity(uint entityID, out CEVec3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetVelocity(uint entityID, ref CEVec3 velocity);
    }
}
