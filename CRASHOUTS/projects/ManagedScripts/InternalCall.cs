using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    internal static class InternalCalls
    {
        // Transform bindings
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetPosition(ulong entityID, out CEVec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetPosition(ulong entityID, ref CEVec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetRotation(ulong entityID, out float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetRotation(ulong entityID, ref float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformGetSize(ulong entityID, out CEVec2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetSize(ulong entityID, ref CEVec2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasTransform(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool EntityHasRigidBody(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalIsKeyPressed(Key key);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasCollider(uint entityID);

        // Inter Script Communications
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern object GetScript(uint entityID, string scriptName);

        // Find Entity
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint FindEntityByName(string name);

        //time
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern double TimeGetDT();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern double TimeGetFixedDT();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern double TimeGetFPS();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int TimeGetSteps();

        //plz help
        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void AddTransform(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void AddRigidBody(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void AddCollider(uint id);


        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RemoveTransform(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RemoveRigidBody(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public extern static void RemoveCollider(uint id);

        // name component
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasName(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddName(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveName(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string GetName(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetName(uint id, string name);

        // audio component
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlayMusicFileTagged(string key, string path, bool loop, float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlaySFXFileTagged(string key, string path, bool loop, float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_StopTagged(string key);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_StopMusic();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetMusicVolume(float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetSFXVolume(float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetMasterVolume(float volume);

        // --- BUTTON COMPONENT LIFECYCLE ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddButton(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveButton(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasButton(uint entityID);

        // --- LAYER COMPONENT LIFECYCLE ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddLayer(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveLayer(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasLayer(uint entityID);

        // --- ANIMATOR ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddKeyframeAnimator(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveKeyframeAnimator(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasKeyframeAnimator(uint entityID);
    }
}
