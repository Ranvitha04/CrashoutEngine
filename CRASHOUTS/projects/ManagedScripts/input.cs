using System;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class Input
    {
        // Maps to your native internal call
        public static bool IsKeyPressed(Key key)
        {
            return InternalIsKeyPressed(key);
        }

        // This gets replaced by Mono internal call binding
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsKeyPressed(Key key);
    }
}
