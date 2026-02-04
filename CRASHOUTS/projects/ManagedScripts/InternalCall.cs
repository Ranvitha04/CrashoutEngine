/*!
@file       InternalCalls.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      internal calls between managed and native code for Crashouts Engine

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Raycast(ref CEVec3 start, ref CEVec3 end, out uint entityID, uint ignoredEntity);

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
        internal static extern void Audio_PlayMusicFileTagged(string key, string path, bool loop);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlaySFXFileTagged(string key, string path, bool loop);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Audio_IsTaggedPlaying(string key);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_StopTagged(string key, float fadeDuration);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_StopMusic();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlayComponent(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_StopComponent(uint entityID, float fadeDuration);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_GetComponentData(uint entityID, out string name, out string path, out float vol, out float pitch, out bool loop, out bool isMusic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlayPathOnEntity(uint entityID, string path, float vol, float pitch, bool loop, bool isMusic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlayClipByName(uint entityID, string clipName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetMusicVolume(float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetSFXVolume(float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetTaggedVolume(string key, float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetMasterVolume(float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Audio_GetMusicVolume();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Audio_GetSFXVolume();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Audio_GetMasterVolume();

        // --- BUTTON COMPONENT LIFECYCLE ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddButton(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveButton(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasButton(uint entityID);

        // --- SHAPE COMPONENT LIFECYCLE ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddShape(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveShape(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasShape(uint entityID);

        // --- Tag COMPONENT LIFECYCLE ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddTag(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveTag(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasTag(uint entityID);

        // --- ANIMATOR ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddKeyframeAnimator(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveKeyframeAnimator(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasKeyframeAnimator(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void PlayDialogue(uint entityID, string fileName, string dialogueID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LoadDialogue(uint entityID);

        // --- VIDEO ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddVideo(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveVideo(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasVideo(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_Play(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_Pause(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_Stop(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_SetLoop(uint entityID, bool loop);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_AddFrame(uint entityID, string path);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_SetFPS(uint entityID, float fps);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_ClearFrames(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Video_Load(uint entityID, string path);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Particle_SetEmitting(uint entityID, bool emitting);
        // Active Component
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetActive(uint entityID, bool isActive);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool GetActive(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetActiveByName(string name, bool isActive);

        // TEXT 
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool IsTextFinished(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetText(uint entityID, string text);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetText(uint entityID);
    }
}
