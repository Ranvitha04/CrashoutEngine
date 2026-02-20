/*!
@file       input.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      input class for handling keyboard and mouse input

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
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

        public static CEVec2 GetMouseWorldPosition()
        {
            CEVec2 pos;
            InternalGetMouseWorldPosition(out pos);
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
        private static extern void InternalGetMouseWorldPosition(out CEVec2 pos);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonDown(int button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonReleased(int button);
    }
}
