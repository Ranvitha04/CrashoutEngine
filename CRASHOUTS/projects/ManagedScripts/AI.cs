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
