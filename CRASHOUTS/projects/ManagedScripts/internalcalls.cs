using System;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    internal static class InternalCalls
    {
        // Transform bindings
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetPosition(ulong entityID, out Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetPosition(ulong entityID, ref Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetRotation(ulong entityID, out float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetRotation(ulong entityID, ref float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetSize(ulong entityID, out Vector2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetSize(ulong entityID, ref Vector2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasTransform(ulong entityID);
    }
}
