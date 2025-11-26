using System;
using System.Collections.Generic;
using CrashoutsEngine;

public class AudioManager
{
    public static AudioManager Instance { get; } = new AudioManager();

    private AudioManager() {
        RegisterSound(
                "ambient", // key you use in Play("ambient")
                "assets/audio/Lava-Volcano-Wind-Environment_LOOP.wav",
                0.3f, // base volume
                true, // loop? (ambient usually yes)
                SoundType.Music // it's music, not SFX
            );

        RegisterSound(
                "footsteps",
                "assets/audio/Footsteps2_Gravel-Dirt_far-small_1.wav",
                1.0f, // base volume
                true, // loop?
                SoundType.Music // it's music, not SFX SFX DOESNT WORK LMAO
            );

        RegisterSound(
                "pressureplate",
                "assets/audio/Switch_StepOn.wav",
                0.6f, // base volume
                false, // loop?
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

    public static float playerMusVol = 2f;
    public static float playerSFXVol = 2f;

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
        else
            InternalCalls.Audio_PlaySFXFileTagged(s.Name, s.Path);
    }

    // === Replacing Unity's Stop(name)
    public void Stop(string name)
    {
        InternalCalls.Audio_StopTagged(name);
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
