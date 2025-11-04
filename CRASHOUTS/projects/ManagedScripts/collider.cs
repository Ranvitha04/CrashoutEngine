using System.Numerics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public class Collider
    {
        internal uint EntityID { get; set; }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalUpdateRotation(uint entityID, float rot);

        public void UpdateRotation(float rotation)
        {
            InternalUpdateRotation(EntityID, rotation);
        }
    }
}
