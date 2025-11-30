/*!
@file       ai.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      ai system for C#

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System;
using System.Runtime.CompilerServices;
namespace CrashoutsEngine
{
    public static class AI
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void Update();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SetState(uint entity, int stateID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static int GetState(uint entity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SetEnter(uint entity, int stateID, Action fn);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SetUpdate(uint entity, int stateID, Action fn);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void SetExit(uint entity, int stateID, Action fn);
    }
}
