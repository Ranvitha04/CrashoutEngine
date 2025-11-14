using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class CollisionEvents
    {
        public delegate void CollisionCallback(uint otherEntity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterCollideEnter(uint entity, CollisionCallback callback);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterCollideStay(uint entity, CollisionCallback callback);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterCollideExit(uint entity, CollisionCallback callback);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterTriggerEnter(uint entity, CollisionCallback callback);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterTriggerStay(uint entity, CollisionCallback callback);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterTriggerExit(uint entity, CollisionCallback callback);
    }
}
