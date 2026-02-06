/*!
@file       VideoComponent.cs
@author     m.biniswandi@digipen.edu
@date       31/01/2026
@brief      C# ScriptBehaviour wrapper for the engine's VideoComponent, 
            providing script-side control for video playback.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddVideo", "RemoveVideo", "EntityHasVideo")]
    public class VideoComponent
    {
        public uint EntityID;
        public VideoComponent() { }
        public VideoComponent(uint id) { EntityID = id; }

        public void Play() { InternalCalls.Video_Play(EntityID); }
        public void Pause() { InternalCalls.Video_Pause(EntityID); }
        public void Stop() { InternalCalls.Video_Stop(EntityID); }
        public void SetLoop(bool loop) { InternalCalls.Video_SetLoop(EntityID, loop); }
        public void AddFrame(string path) { InternalCalls.Video_AddFrame(EntityID, path); }
        public void SetFPS(float fps) { InternalCalls.Video_SetFPS(EntityID, fps); }
        public void ClearFrames() { InternalCalls.Video_ClearFrames(EntityID); }
        public void Load(string path) { InternalCalls.Video_Load(EntityID, path); }

        public void LoadSequence(string basePath, int count, string ext = ".png") {
            ClearFrames();
            for(int i=0; i<count; ++i) {
                AddFrame(basePath + i + ext);
            }
        }
    }
}
