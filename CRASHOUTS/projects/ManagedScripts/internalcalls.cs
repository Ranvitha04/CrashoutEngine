using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    internal static class InternalCalls
    {
        // Transform bindings
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetPosition(ulong entityID, out CEVec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetPosition(ulong entityID, ref CEVec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetRotation(ulong entityID, out float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetRotation(ulong entityID, ref float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetSize(ulong entityID, out CEVec2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetSize(ulong entityID, ref CEVec2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasTransform(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool EntityHasRigidBody(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsKeyPressed(Key key);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasCollider(uint entityID);
    }
}
