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

    private AudioManager() {

        playerMusVol = InternalCalls.Audio_GetMusicVolume();
        playerSFXVol = InternalCalls.Audio_GetSFXVolume();

        RegisterSound(
                "ambient", // key you use in Play("ambient")
                "assets/audio/Lava-Volcano-Wind-Environment_LOOP.wav",
                0.05f, // base volume
                true, // loop? (ambient usually yes)
                SoundType.Music // it's music, not SFX
            );

        RegisterSound(
                "footstep1",
                "assets/audio/footsteps 1.wav",
                1.0f, // base volume
                false, // loop?
                //SoundType.SFX //SFX broken
                SoundType.SFX
            );

        RegisterSound(
                "footstep2",
                "assets/audio/footsteps 2.wav",
                1.0f, // base volume
                false, // loop?
                      //SoundType.SFX //SFX broken
                SoundType.SFX
            );

        RegisterSound(
                "footstep3",
                "assets/audio/footsteps 3.wav",
                1.0f, // base volume
                false, // loop?
                      //SoundType.SFX //SFX broken
                SoundType.SFX
            );

        RegisterSound(
                "footstep4",
                "assets/audio/footsteps 4.wav",
                1.0f, // base volume
                false, // loop?
                      //SoundType.SFX //SFX broken
                SoundType.SFX
            );

        RegisterSound(
                "footstep5",
                "assets/audio/footsteps 5.wav",
                1.0f, // base volume
                false, // loop?
                      //SoundType.SFX //SFX broken
                SoundType.SFX
            );

        RegisterSound(
                "footstep6",
                "assets/audio/footsteps 6.wav",
                1.0f, // base volume
                false, // loop?
                      //SoundType.SFX //SFX broken
                SoundType.SFX
            );

        RegisterSound(
                "pressureplate",
                "assets/audio/Switch_StepOn.wav",
                0.6f, // base volume
                false, // loop?
                SoundType.SFX // it's music, not SFX SFX DOESNT WORK LMAO
            );

        RegisterSound(
                "bgm1",
                "assets/audio/level bgm 1.wav",
                0.8f, // base volume
                true, // loop?
                SoundType.Music // it's music, not SFX SFX DOESNT WORK LMAO
            );

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

    // === Play sound attached to an entity
    public void PlayAttachedSound(uint entityID)
    {
        InternalCalls.Audio_PlayComponent(entityID);
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

    // A fade system can be added later
}
