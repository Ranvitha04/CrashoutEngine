using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public class ContactInfo
    {
        public uint collidedEntity;
        public CollisionManifold manifold = new CollisionManifold();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_Fill(uint entity, ContactInfo info);

        public static ContactInfo GetForEntity(uint entity)
        {
            var ci = new ContactInfo();
            Internal_Fill(entity, ci);
            return ci;
        }
    }
}
