/*!
@file       collisionevents.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      collision events for Crashouts Engine

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class CollisionEvents
    {
        public delegate void CollisionCallback(uint otherEntity);

        // --- PreSolve Delegate (Must return bool) ---
        public delegate bool PreSolveCallback(uint otherEntity);

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

        // --- Register function for PreSolve ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RegisterPreSolve(uint entity, PreSolveCallback callback);
    }
}
