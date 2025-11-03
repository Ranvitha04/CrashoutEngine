using System.Numerics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public class RigidBody
    {
        internal uint EntityID { get; set; }

        public void AddForce(Vector3 force)
        {
            InternalAddForce(EntityID, force.X, force.Y, force.Z);
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
