using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public class RigidBody
    {
        internal uint EntityID { get; set; }

        public void AddForce(CEVec3 force)
        {
            InternalAddForce(EntityID, force.x, force.y, force.z);
        }

        public float GetMass()
        {
            return InternalGetMass(EntityID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalAddForce(uint entityID, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float InternalGetMass(uint entityID);
    }

}
