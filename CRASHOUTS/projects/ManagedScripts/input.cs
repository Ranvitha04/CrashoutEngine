using System;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class Input
    {
        public static bool IsKeyPressed(Key key)
        {
            return InternalIsKeyPressed(key);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsKeyPressed(Key key);
    }
}
