using System;

namespace CrashoutsEngine
{
    [NativeComponent("AddAnimator", "RemoveAnimator", "EntityHasAnimator")]
    public class Animator
    {
        public uint EntityID;

        internal Animator() { }

        public void Play(string state)
        {
            InternalPlay(EntityID, state);
        }

        public void SetBool(string key, bool value)
        {
            InternalSetBool(EntityID, key, value);
        }

        public void SetFloat(string key, float value)
        {
            InternalSetFloat(EntityID, key, value);
        }

        public void SetTrigger(string key)
        {
            InternalSetTrigger(EntityID, key);
        }

        public void SetFlip(bool flip)
        {
            InternalSetFlip(EntityID, flip);
        }

        public string CurrentState
        {
            get => InternalGetCurrentState(EntityID);
        }

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private extern static void InternalPlay(uint entityID, string state);

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private extern static void InternalSetBool(uint entityID, string key, bool value);

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private extern static void InternalSetFloat(uint entityID, string key, float value);

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private extern static void InternalSetTrigger(uint entityID, string key);

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private extern static void InternalSetFlip(uint entityID, bool flip);

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        private extern static string InternalGetCurrentState(uint entityID);
    }
}
