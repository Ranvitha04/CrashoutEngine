/*!
@file       Animator.cs
@author     shahrinidzuan.m@digipen.edu
@date       20/11/2025
@brief      Animator component class for managing keyframe animations.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    // attributes match the C++ component lifecycle functions
    [NativeComponent("AddKeyframeAnimator", "RemoveKeyframeAnimator", "EntityHasKeyframeAnimator")]
    public class Animator
    {
        public uint EntityID;

        // Internal constructor prevents "new Animator()" usage by users
        public Animator() { }

        // --- PUBLIC API ---

        // Play a specific clip by name
        public void Play(string clipName, bool restart = true)
        {
            InternalPlay(EntityID, clipName, restart);
        }

        // Check if current animation is playing
        public bool IsPlaying
        {
            get => InternalGetPlaying(EntityID);
            set => InternalSetPlaying(EntityID, value);
        }

        // Get the name of the currently playing clip
        public string CurrentClip
        {
            get => InternalGetCurrentClip(EntityID);
        }

        // --- INTERNAL CALLS ---

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalPlay(uint entityID, string clipName, bool restart);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static bool InternalGetPlaying(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetPlaying(uint entityID, bool playing);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static string InternalGetCurrentClip(uint entityID);
    }
}
