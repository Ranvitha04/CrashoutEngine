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
        internal static extern void TransformGetWorldPosition(ulong entityID, out CEVec3 position);

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
        internal static extern void Audio_GetComponentData(uint entityID, out string name, out string path, out float vol, out float pitch, out bool loop, out bool isMusic, out bool spatial3D, out float minDistance, out float maxDistance);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_PlayPathOnEntity(uint entityID, string path, float vol, float pitch, bool loop, bool isMusic, bool spatial3D, float minDistance, float maxDistance);

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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetListenerAttributes(ref CEVec3 pos, ref CEVec3 forward, ref CEVec3 up, ref CEVec3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetClipVolume(uint entityID, string clipName, float volume);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetClipPosition(uint entityID, string clipName, float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetClip3DDistances(uint entityID, string clipName, float minDist, float maxDist);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_AddClipDSP(uint entityID, string clipName, int dspType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_RemoveClipDSP(uint entityID, string clipName, int dspType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetClipDSPParameter(uint entityID, string clipName, int dspType, int index, float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_AddGroupDSP(bool isMusic, int dspType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_RemoveGroupDSP(bool isMusic, int dspType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_SetGroupDSPParameter(bool isMusic, int dspType, int index, float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_FadeGroupDSPParameter(bool isMusic, int dspType, int index, float targetValue, float duration);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Audio_FadeGroupVolume(bool isMusic, float targetVolume, float duration);

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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Particle_TriggerBurst(uint entityID);
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

        // Checkpoint
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddCheckpoint(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasCheckpoint(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveCheckpoint(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Checkpoint_GetRespawnPosition(uint entityID, out CEVec3 pos);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Checkpoint_SetRespawnPosition(uint entityID, ref CEVec3 pos);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint Checkpoint_GetTargetEntity(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Checkpoint_SetTargetEntity(uint entityID, uint targetID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string Checkpoint_GetTargetName(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Checkpoint_SetTargetName(uint entityID, string name);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Checkpoint_GetActivated(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Checkpoint_SetActivated(uint entityID, bool activated);

        // Trigger
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddTrigger(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool EntityHasTrigger(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RemoveTrigger(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string Trigger_GetID(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Trigger_SetID(uint entityID, string id);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Trigger_GetOneTimeUse(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Trigger_SetOneTimeUse(uint entityID, bool oneTime);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Trigger_GetTriggered(uint entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Trigger_SetTriggered(uint entityID, bool triggered);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetGameMousePos(out CEVec2 pos);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetGameWindowSize(out CEVec2 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformSetLocalScale(uint entityID, ref CEVec2 scale);
    }
}
