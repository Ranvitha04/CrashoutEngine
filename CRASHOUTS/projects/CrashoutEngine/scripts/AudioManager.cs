/*!
@file		AudioManager.cs
@author     Mohamad Amir Akmal
@author     Shahrin Idzuan
@author     mohamadamirakmal.b
@date       19/11/2025
@brief		Declaration and implementation of AudioManager class to manage audio playback.
*/
using System;
using System.Collections.Generic;
using CrashoutsEngine;

public class AudioManager
{
    public static AudioManager Instance { get; } = new AudioManager();

    private Dictionary<string, string> _activeLayers = new Dictionary<string, string>();

    private AudioManager() {

        playerMusVol = InternalCalls.Audio_GetMusicVolume();
        playerSFXVol = InternalCalls.Audio_GetSFXVolume();

        
        /*RegisterSound(
                "ambient",
                "assets/audio/Nightshift BGM_Loop_70bpm.wav",
                1.0f,
                true,
                SoundType.Music
            );*/
    }

    public static AudioManager GetInstance()
    {
        return Instance;
    }

    public static void SyncVolumesFromEngine()
    {
        playerMusVol = InternalCalls.Audio_GetMusicVolume();
        playerSFXVol = InternalCalls.Audio_GetSFXVolume();
    }

    public static void ResetInstance()
    {
        Instance._activeLayers.Clear();
        Instance.sounds.Clear();
        
        // Re-sync preferences with C++ (important if engine loaded new settings during scene load)
        SyncVolumesFromEngine();
    }

    // Converted version of your "Sound[]" array
    private Dictionary<string, SoundData> sounds = new Dictionary<string, SoundData>();
        
    public static float playerMusVol = 1f;
    public static float playerSFXVol = 1f;

    private class SoundData
    {
        public string Name;
        public string Path;
        public float BaseVolume;
        public bool Loop;
        public SoundType Type;
    }

    public enum SoundType
    {
        Music,
        SFX
    }

    public enum FMOD_DSP_TYPE
    {
        UNKNOWN,
        MIXER,
        OSCILLATOR,
        LOWPASS,
        ITLOWPASS,
        HIGHPASS,
        ECHO,
        FADER,
        FLANGER,
        DISTORTION,
        NORMALIZE,
        LIMIT,
        PARAMEQ,
        PITCHSHIFT,
        CHORUS,
        VSTPLUGIN,
        WINAMPPLUGIN,
        ITECHO,
        COMPRESSOR,
        SFXREVERB,
        LOWPASSSIMPLE,
        DELAY,
        TREMOLO,
        LADSPAPLUGIN,
        SEND,
        RETURN,
        HIGHPASSSIMPLE,
        PAN,
        THREEEQ,
        FFT,
        LOUDNESSMETER,
        ENVELOPEFOLLOWER,
        CONVOLUTIONREVERB,
        CHANNELMIX,
        TRANSCEIVER,
        OBJECTPAN,
        MULTIBANDEQ
    }

    public static void SetListenerAttributes(CEVec3 position, CEVec3 forward, CEVec3 up, CEVec3 velocity)
    {
        InternalCalls.Audio_SetListenerAttributes(ref position, ref forward, ref up, ref velocity);
    }

    /// <summary>
    /// Update listener position only (typical for 2D games).
    /// </summary>
    public static void SetListenerPosition(CEVec3 position)
    {
        CEVec3 forward = new CEVec3(0, 0, 1);
        CEVec3 up = new CEVec3(0, 1, 0);
        CEVec3 velocity = new CEVec3(0, 0, 0);
        InternalCalls.Audio_SetListenerAttributes(ref position, ref forward, ref up, ref velocity);
    }

    // Equivalent to setting Unity inspector fields
    public void RegisterSound(string name, string path, float baseVolume, bool loop, SoundType type)
    {
        sounds[name] = new SoundData
        {
            Name = name,
            Path = path,
            BaseVolume = baseVolume,
            Loop = loop,
            Type = type
        };
    }

    // === Replacing Unity's Play(name)
    public void Play(string name)
    {
        if (!sounds.TryGetValue(name, out var s))
        {
            Console.WriteLine($"[Audio] Sound '{name}' not found!");
            return;
        }

        if (s.Type == SoundType.Music)
            InternalCalls.Audio_PlayMusicFileTagged(s.Name, s.Path, s.Loop);
        else if (s.Type == SoundType.SFX)
            InternalCalls.Audio_PlaySFXFileTagged(s.Name, s.Path, s.Loop);
    }

    // === Play music or ambience globally with layering support
    public void PlayLayer(string layerName, string key, string path, bool loop, float volume, float fadeDuration, bool isMusic)
    {
        string currentKeyOnLayer = "";
        if (_activeLayers.ContainsKey(layerName))
        {
            currentKeyOnLayer = _activeLayers[layerName];
        }

        if (currentKeyOnLayer == key)
        {
            bool isActuallyPlaying = InternalCalls.Audio_IsTaggedPlaying(key);

            if (isActuallyPlaying)
            {
                InternalCalls.Audio_SetTaggedVolume(key, volume);
                return;
            }
        }

        if (!string.IsNullOrEmpty(currentKeyOnLayer))
        {
            InternalCalls.Audio_StopTagged(currentKeyOnLayer, fadeDuration);
        }

        if (isMusic)
        {
            // Plays on Music Group (Controlled by Music Slider)
            InternalCalls.Audio_PlayMusicFileTagged(key, path, loop);
        }
        else
        {
            // Plays on SFX Group (Controlled by SFX Slider)
            InternalCalls.Audio_PlaySFXFileTagged(key, path, loop);
        }

        if (fadeDuration > 0)
        {
            InternalCalls.Audio_SetTaggedVolume(key, 0.0f);
            InternalCalls.Audio_SetPausedTagged(key, false); // Now safe to unpause
            InternalCalls.Audio_FadeTaggedVolume(key, volume, fadeDuration);
        }
        else
        {
            InternalCalls.Audio_SetTaggedVolume(key, volume);
            InternalCalls.Audio_SetPausedTagged(key, false);
        }

        _activeLayers[layerName] = key;
    }

    // === Play sound attached to an entity
    public void PlayAttachedSound(uint entityID)
    {
        InternalCalls.Audio_PlayComponent(entityID);
    }

    public void PlayAttached(string clipName, uint entityID)
    {
        InternalCalls.Audio_PlayClipByName(entityID, clipName);
    }

    public static void SetClipVolume(uint entityID, string clipName, float volume)
    {
        InternalCalls.Audio_SetClipVolume(entityID, clipName, volume);
    }

    public static void SetClipPosition(uint entityID, string clipName, CEVec3 position)
    {
        InternalCalls.Audio_SetClipPosition(entityID, clipName, position.x, position.y, position.z);
    }

    public static void SetClip3DDistances(uint entityID, string clipName, float minDist, float maxDist)
    {
        InternalCalls.Audio_SetClip3DDistances(entityID, clipName, minDist, maxDist);
    }

    public static void AddClipDSP(uint entityID, string clipName, FMOD_DSP_TYPE dspType)
    {
        InternalCalls.Audio_AddClipDSP(entityID, clipName, (int)dspType);
    }

    public static void RemoveClipDSP(uint entityID, string clipName, FMOD_DSP_TYPE dspType)
    {
        InternalCalls.Audio_RemoveClipDSP(entityID, clipName, (int)dspType);
    }

    public static void SetClipDSPParameter(uint entityID, string clipName, FMOD_DSP_TYPE dspType, int index, float value)
    {
        InternalCalls.Audio_SetClipDSPParameter(entityID, clipName, (int)dspType, index, value);
    }

    public static void AddGroupDSP(bool isMusic, FMOD_DSP_TYPE dspType)
    {
        InternalCalls.Audio_AddGroupDSP(isMusic, (int)dspType);
    }

    public static void RemoveGroupDSP(bool isMusic, FMOD_DSP_TYPE dspType)
    {
        InternalCalls.Audio_RemoveGroupDSP(isMusic, (int)dspType);
    }

    public static void SetGroupDSPParameter(bool isMusic, FMOD_DSP_TYPE dspType, int index, float value)
    {
        InternalCalls.Audio_SetGroupDSPParameter(isMusic, (int)dspType, index, value);
    }

    public static void FadeGroupDSPParameter(bool isMusic, FMOD_DSP_TYPE dspType, int index, float targetValue, float duration)
    {
        InternalCalls.Audio_FadeGroupDSPParameter(isMusic, (int)dspType, index, targetValue, duration);
    }

    public static void FadeGroupVolume(bool isMusic, float targetVolume, float duration)
    {
        InternalCalls.Audio_FadeGroupVolume(isMusic, targetVolume, duration);
    }

    // === Stop sound attached to an entity ===
    public void StopAttachedSound(uint entityID, float fadeDuration = 0.5f)
    {
        InternalCalls.Audio_StopComponent(entityID, fadeDuration);
    }

    // === Replacing Unity's Stop(name)
    public void Stop(string name, float fadeDuration = 0.5f)
    {
        InternalCalls.Audio_StopTagged(name, fadeDuration);
    }

    // === Equivalent of MusicStop()
    public void MusicStop()
    {
        InternalCalls.Audio_StopMusic();
    }

    public void MusicVolume(float volume)
    {
        playerMusVol = volume;
        InternalCalls.Audio_SetMusicVolume(volume);
    }

    public void SFXVolume(float volume)
    {
        playerSFXVol = volume;
        InternalCalls.Audio_SetSFXVolume(volume);
    }

    public void StopLayer(string layerName, float fadeDuration)
    {
        // 1. Check if anything is playing on this layer
        if (_activeLayers.ContainsKey(layerName))
        {
            string keyToStop = _activeLayers[layerName];

            // 2. Tell C++ to stop that specific tag
            if (!string.IsNullOrEmpty(keyToStop))
            {
                InternalCalls.Audio_StopTagged(keyToStop, fadeDuration);
            }

            // 3. Clear the record so we know nothing is playing
            _activeLayers.Remove(layerName);
        }
    }

    // A fade system can be added later
}
