/*!
@file		BetterSceneMusic.cs
@author     Mohamad Amir Akmal
@date       09/03/2026
@brief		An improved version of SceneMusic that adds DSP effects, proper class naming, and more control over the audio layer.
*/
using System;
using System.Runtime.CompilerServices;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class BetterSceneMusic : ScriptBase
    {
        // --- Core Music Settings ---
        [SerializeField] private string layerName = "Music";
        [SerializeField] private float fadeTime = 2.0f;
        [SerializeField] private bool stopLayer = false;

        // --- Advanced DSP Settings ---
        [SerializeField] private bool applyLowPassFilter = false;
        [SerializeField] private float lowPassCutoff = 22000.0f; // Default max frequency

        [SerializeField] private bool applyReverb = false;

        public override void Start()
        {
            if (stopLayer)
            {
                // Gracefully stop the layer with a fade out
                AudioManager.GetInstance().StopLayer(layerName, fadeTime);
                return;
            }

            // Retrieve audio data from the entity's AudioEmitterComponent
            InternalCalls.Audio_GetComponentData(
                Entity.ID,
                out string key,
                out string path,
                out float vol,
                out float pitch,
                out bool loop,
                out bool isMusic,
                out bool spatial3D,
                out float minDistance,
                out float maxDistance
            );

            if (!string.IsNullOrEmpty(path) && !string.IsNullOrEmpty(key))
            {
                // Play the music/ambience layer
                AudioManager.GetInstance().PlayLayer(layerName, key, path, loop, vol, fadeTime, isMusic);

                // --- Apply DSP Effects ---
                if (applyLowPassFilter)
                {
                    AudioManager.AddGroupDSP(isMusic, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                    // Set the cutoff frequency (Index 0 is typically CUTOFF for FMOD Lowpass)
                    AudioManager.SetGroupDSPParameter(isMusic, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, lowPassCutoff);
                }
                else
                {
                    // Clean up if not used
                    AudioManager.RemoveGroupDSP(isMusic, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                }

                if (applyReverb)
                {
                    AudioManager.AddGroupDSP(isMusic, AudioManager.FMOD_DSP_TYPE.SFXREVERB);
                }
                else
                {
                    // Clean up if not used
                    AudioManager.RemoveGroupDSP(isMusic, AudioManager.FMOD_DSP_TYPE.SFXREVERB);
                }
            }
        }

        public override void Update() { }

        public override void FixedUpdate() { }

        public override void OnDrawGizmos() { }

        public override void OnDestroy() 
        {
            // Optional: You could fade out the music when this entity is destroyed
            // AudioManager.GetInstance().StopLayer(layerName, fadeTime);
        }
    }
}
