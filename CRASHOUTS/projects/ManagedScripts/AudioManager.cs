/*!
@file		AudioManager.cs
@author     mohamadamirakmal.b@digipen.edu
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
    public void PlayGlobal(string layerName, string key, string path, bool loop, float volume = 1.0f, float fadeDuration = 2.0f)
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

        InternalCalls.Audio_PlayMusicFileTagged(key, path, loop);
        InternalCalls.Audio_SetTaggedVolume(key, volume);

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

    // === Stop sound attached to an entity ===
    public void StopAttachedSound(uint entityID, float fadeDuration = 0.5f)
    {
        InternalCalls.Audio_StopComponent(entityID, fadeDuration);
    }

    // === Replacing Unity's Stop(name)
    public void Stop(string name, float fadeDuration = 0.5f)
    {
        InternalCalls.Audio_Stoplayerged(name, fadeDuration);
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

    // A fade system can be added later
}
