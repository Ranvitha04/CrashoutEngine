/*!
@file       input.cs
@author     Renfred Lee
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

        // Returns a gameplay axis in the range [-1, 1].
        // Example: "MoveX" combines A/D with the left stick X axis.
        public static float GetAxis(string axisName)
        {
            return InternalGetAxis(axisName);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float InternalGetAxis(string axisName);

        public static bool GetAction(string actionName)
        {
            return InternalGetAction(actionName);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalGetAction(string actionName);

        public static bool GetActionTriggered(string actionName)
        {
            return InternalGetActionTriggered(actionName);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalGetActionTriggered(string actionName);

        public static bool GetActionReleased(string actionName)
        {
            return InternalGetActionReleased(actionName);
        }
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalGetActionReleased(string actionName);

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

        // --- MOUSE BUTTONS ---

        public static bool IsMouseButtonDown(int button)
        {
            return InternalIsMouseButtonDown(button);
        }

        public static bool IsMouseButtonTriggered(int button)
        {
            return InternalIsMouseButtonTriggered(button);
        }

        public static bool IsMouseButtonReleased(int button)
        {
            return InternalIsMouseButtonReleased(button);
        }

        // --- INTERNAL MOUSE CALLS ---

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalGetMousePosition(out CEVec2 pos);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalGetMouseWorldPosition(out CEVec2 pos);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonDown(int button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonTriggered(int button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsMouseButtonReleased(int button);
    }
}