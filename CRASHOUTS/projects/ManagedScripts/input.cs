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

        public static bool IsKeyTriggered(Key key)
        {
            return InternalIsKeyTriggered(key);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsKeyTriggered(Key key);

        public static bool IsKeyHeld(Key key)
        {
            return InternalIsKeyHeld(key);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsKeyHeld(Key key);

        public static CEVec2 GetMousePosition()
        {
            CEVec2 pos;
            InternalGetMousePosition(out pos);
            return pos;
        }

        public static bool IsMouseButtonDown(int button)
        {
            return InternalIsMouseButtonDown(button);
        }

        public static bool IsMouseButtonReleased(int button)
        {
            return InternalIsMouseButtonReleased(button);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalGetMousePosition(out CEVec2 pos);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonDown(int button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonReleased(int button);
    }
}
