using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddCollider", "RemoveCollider", "EntityHasCollider")]
    public class ColliderComponent
    {
        public uint EntityID;

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalUpdateRotation(uint entityID, float rot);

        public void UpdateRotation(float rotation)
        {
            InternalUpdateRotation(EntityID, rotation);
        }
    }
}
