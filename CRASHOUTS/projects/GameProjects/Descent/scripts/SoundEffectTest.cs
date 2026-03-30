/*!
@file		SoundEffectTest.cs
@author     Mohamad Amir Akmal Bin Nasrudin
@date       25/03/2026
@brief		Test for Sound Effects.
            Configure FX settings, select a test (0-16), and press 'G'.
*/
using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    /**
     * Test Case Mapping:
     * 0: PlayTagged
     * 1: TogglePause
     * 2: FadeVolume
     * 3: RestoreVolume
     * 4: StopTagged
     * 5: PlayComponent
     * 6: PlayClipByName
     * 7: ClipProperties
     * 8: ClipDSPEcho
     * 9: ClipDSPLowPass
     * 10: ClipDSPDistortion
     * 11: GroupDSPLowPass
     * 12: GroupDSPDistortion
     * 13: RemoveAllDSP
     * 14: UpdateActiveFX (Apply params to currently playing sound)
     * 15: ResetSystem
     * 16: CycleTests
     */
    [ScriptBehaviour]
    public class SoundEffectTest : ScriptBase
    {
        [SerializeField] public int selectedTest = 0;

        // --- Adjustable Parameters for sound effects ---
        [SerializeField] private float lowPassCutoff = 500.0f; // Hz
        [SerializeField] private float distortionLevel = 0.8f; // 0.0 to 1.0
        [SerializeField] private float echoDelay = 500.0f; // ms (0 to 5000)
        
        private int cycleIndex = 0;
        private const int MaxCycleTests = 16;

        private string engineClipName;
        private string enginePath;
        private float engineVol;
        private float enginePitch;
        private bool engineLoop;
        private bool engineIsMusic;
        private bool engineSpatial3D;
        private float engineMinDist;
        private float engineMaxDist;

        public override void Start()
        {
            Debug.Log("[AudioTest] SoundEffectTest (Adjustable) started on Entity: " + Entity.ID);
            RefreshEngineData();
        }

        private void RefreshEngineData()
        {
            InternalCalls.Audio_GetComponentData(
                Entity.ID, 
                out engineClipName, 
                out enginePath, 
                out engineVol, 
                out enginePitch, 
                out engineLoop, 
                out engineIsMusic, 
                out engineSpatial3D, 
                out engineMinDist, 
                out engineMaxDist
            );
        }

        public override void Update()
        {
            if (Input.IsKeyTriggered(Key.G))
            {
                RefreshEngineData();

                if (selectedTest == 16) // CycleTests
                {
                    cycleIndex++;
                    if (cycleIndex >= 16) cycleIndex = 0;
                    RunTestByIndex(cycleIndex);
                }
                else
                {
                    RunTestByIndex(selectedTest);
                }
            }
        }

        private void RunTestByIndex(int index)
        {
            if (index < 0 || index > 16) return;
            Debug.Log($"\n[AudioTest] >>> Running Test Index: {index} <<<");
            ExecuteTestLogic(index);
        }

        private void ExecuteTestLogic(int index)
        {
            switch (index)
            {
                case 0: PlayCurrentTagged(); break;
                case 1:
                    bool isPlaying = InternalCalls.Audio_IsTaggedPlaying(engineClipName);
                    InternalCalls.Audio_SetPausedTagged(engineClipName, isPlaying);
                    break;
                case 2: InternalCalls.Audio_FadeTaggedVolume(engineClipName, 0.1f, 2.0f); break;
                case 3:
                    InternalCalls.Audio_SetTaggedVolume(engineClipName, engineVol);
                    InternalCalls.Audio_SetPausedTagged(engineClipName, false);
                    break;
                case 4: InternalCalls.Audio_StopTagged(engineClipName, 1.0f); break;
                case 5: InternalCalls.Audio_PlayComponent(Entity.ID); break;
                case 6: 
                    if (!string.IsNullOrEmpty(engineClipName))
                        InternalCalls.Audio_PlayClipByName(Entity.ID, engineClipName);
                    break;
                case 7:
                    if (!string.IsNullOrEmpty(engineClipName)) {
                        InternalCalls.Audio_SetClipVolume(Entity.ID, engineClipName, 0.5f);
                        InternalCalls.Audio_SetClip3DDistances(Entity.ID, engineClipName, 2.0f, 50.0f);
                    }
                    break;

                case 8: // ClipDSPEcho
                    if (!string.IsNullOrEmpty(engineClipName)) {
                        Debug.Log($"[AudioTest] Playing '{engineClipName}' with Echo Delay: {echoDelay}ms");
                        InternalCalls.Audio_PlayClipByName(Entity.ID, engineClipName);
                        AudioManager.AddClipDSP(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.ECHO);
                        AudioManager.SetClipDSPParameter(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.ECHO, 0, echoDelay);
                    }
                    break;

                case 9: // ClipDSPLowPass
                    if (!string.IsNullOrEmpty(engineClipName)) {
                        Debug.Log($"[AudioTest] Playing '{engineClipName}' with LowPass: {lowPassCutoff}Hz");
                        InternalCalls.Audio_PlayClipByName(Entity.ID, engineClipName);
                        AudioManager.AddClipDSP(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                        AudioManager.SetClipDSPParameter(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, lowPassCutoff);
                    }
                    break;

                case 10: // ClipDSPDistortion
                    if (!string.IsNullOrEmpty(engineClipName)) {
                        Debug.Log($"[AudioTest] Playing '{engineClipName}' with Distortion: {distortionLevel}");
                        InternalCalls.Audio_PlayClipByName(Entity.ID, engineClipName);
                        AudioManager.AddClipDSP(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.DISTORTION);
                        AudioManager.SetClipDSPParameter(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.DISTORTION, 0, distortionLevel);
                    }
                    break;

                case 11: // GroupDSPLowPass
                    Debug.Log($"[AudioTest] Applying Group LowPass: {lowPassCutoff}Hz");
                    AudioManager.AddGroupDSP(false, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                    AudioManager.SetGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, lowPassCutoff);
                    PlayCurrentTagged();
                    break;

                case 12: // GroupDSPDistortion
                    Debug.Log($"[AudioTest] Applying Group Distortion: {distortionLevel}");
                    AudioManager.AddGroupDSP(false, AudioManager.FMOD_DSP_TYPE.DISTORTION);
                    AudioManager.SetGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.DISTORTION, 0, distortionLevel);
                    PlayCurrentTagged();
                    break;

                case 13: // RemoveAllDSP
                    Debug.Log("[AudioTest] Removing all DSP effects.");
                    if (!string.IsNullOrEmpty(engineClipName)) {
                        AudioManager.RemoveClipDSP(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.ECHO);
                        AudioManager.RemoveClipDSP(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                        AudioManager.RemoveClipDSP(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.DISTORTION);
                    }
                    AudioManager.RemoveGroupDSP(false, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                    AudioManager.RemoveGroupDSP(false, AudioManager.FMOD_DSP_TYPE.DISTORTION);
                    break;

                case 14: // UpdateActiveFX
                    if (!string.IsNullOrEmpty(engineClipName)) {
                        Debug.Log("[AudioTest] Updating parameters for ALL potential active FX on clip and group.");
                        AudioManager.SetClipDSPParameter(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, lowPassCutoff);
                        AudioManager.SetClipDSPParameter(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.DISTORTION, 0, distortionLevel);
                        AudioManager.SetClipDSPParameter(Entity.ID, engineClipName, AudioManager.FMOD_DSP_TYPE.ECHO, 0, echoDelay);
                        
                        AudioManager.SetGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, lowPassCutoff);
                        AudioManager.SetGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.DISTORTION, 0, distortionLevel);
                    }
                    break;

                case 15: InternalCalls.Audio_Reset(); break;
            }
        }

        private void PlayCurrentTagged()
        {
            if (!string.IsNullOrEmpty(enginePath)) {
                if (engineIsMusic) InternalCalls.Audio_PlayMusicFileTagged(engineClipName, enginePath, engineLoop);
                else InternalCalls.Audio_PlaySFXFileTagged(engineClipName, enginePath, engineLoop);
            }
        }

        public override void OnDestroy()
        {
            if (!string.IsNullOrEmpty(engineClipName))
                InternalCalls.Audio_StopTagged(engineClipName, 0.2f);
        }
    }
}
