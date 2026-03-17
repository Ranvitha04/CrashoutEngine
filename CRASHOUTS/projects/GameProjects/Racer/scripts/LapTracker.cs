/*!
@file       LapTracker.cs
@author     Renfred (RTIS)
@author     Renfred Lee
@date       13/03/2026
@brief      Tracks lap progression, timing, and ghost recording for the Racer project.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Manages lap/checkpoint progression, UI updates, and end-of-race handling.
    */
    public class LapTracker : ScriptBase
    {
        public static LapTracker Instance { get; private set; }

        public int TotalLaps = 3;
        public int TotalCheckpointsInTrack = 3; 
        private int CurrentLap = 0;

        public Entity lapDisplay;
        public Entity timerDisplay;
        public string TrackName = "Level1"; 
        public Entity nameInputPanel;
        
        public float FinalTime { get; private set; }         
        private float _raceTime = 0f;
        private int _lastClearedID = 0; 
        private bool _isRacing = false;
        private float _currentLapTime = 0f;
        private bool _isWrongWay = false;

        /*!
        @brief Initialize race state, orient the vehicle, and start ghost recording.
        */
        public override void Start()
        {
            Instance = this;
            _raceTime = 0f;
            _currentLapTime = 0f;
            CurrentLap = 0;
            _lastClearedID = 0;
            _isRacing = true; 
            _isWrongWay = false;

            if (TotalLaps <= 0) TotalLaps = 3;
            if (TotalCheckpointsInTrack <= 0) TotalCheckpointsInTrack = 3;

            var transform = Entity.GetComponent<TransformComponent>();
            if (transform != null) transform.Rotation = -90.0f;

            var rb = Entity.GetComponent<RigidBodyComponent>();
            if (rb != null) rb.Rotation = -90.0f;

            Debug.Log($"Race Started! Lap {(CurrentLap + 1)} / {TotalLaps}");
            
            ReplayManager.StartRecording();
            UpdateLapUI();

            if (nameInputPanel != null && nameInputPanel.ID != 0)
                InternalCalls.SetActive(nameInputPanel.ID, false);
        }

        /*!
        @brief Update the race timer and UI while racing.
        */
        public override void Update() 
        { 
            if (_isRacing)
            {
                float dt = (float)Time.DT;
                _raceTime += dt; 
                _currentLapTime += dt; 

                if (timerDisplay != null && timerDisplay.ID != 0)
                {
                    InternalCalls.SetText(timerDisplay.ID, FormatTime(_raceTime));
                }

                if (lapDisplay != null && lapDisplay.ID != 0 && CurrentLap == 0 && !_isWrongWay)
                {
                    InternalCalls.SetText(lapDisplay.ID, $"Lap: [1 / {TotalLaps}]");
                }
            }
        }

        /*!
        @brief Notify the tracker that a checkpoint/finish gate was hit.
        @param hitID Checkpoint id (0 is treated as the finish line).
        */
        public void CheckpointHit(int hitID)
        {
            if (!_isRacing) return;

            // --- CASE 1: HITTING THE FINISH LINE (ID 0) ---
            if (hitID == 0)
            {
                // FIX 1: You must clear ALL checkpoints (e.g., ID 3) before hitting 0.
                if (_lastClearedID == TotalCheckpointsInTrack)
                {
                    CurrentLap++;
                    _lastClearedID = 0;
                    SetWrongWay(false); 
                    
                    if (CurrentLap >= TotalLaps)
                    {
                        CompleteRace();
                    }
                    else
                    {
                        Debug.Log($"Lap {CurrentLap} Completed!");
                        UpdateLapUI(); 
                    }
                }
                else if (_lastClearedID == 0)
                {
                    // Player is just sitting on the start line or just started, ignore.
                    SetWrongWay(false);
                }
                else
                {
                    // Player hit finish line but missed checkpoints!
                    Debug.Log("Wrong Way! Missed checkpoints before finish.");
                    SetWrongWay(true);
                }
                return; // Exit here so we don't process Cases 2/3/4 below
            }

            // --- CASE 2: NORMAL PROGRESSION ---
            if (hitID == _lastClearedID + 1)
            {
                _lastClearedID = hitID;
                SetWrongWay(false); 
                Debug.Log($"Progressed to Checkpoint {hitID}");
            }
            // --- CASE 3: BACKTRACKING ---
            else if (hitID <= _lastClearedID)
            {
                // FIX 2: If you reverse into CP 2, your progress drops to CP 1.
                // This forces you to drive forward through CP 2 again to turn off the warning!
                _lastClearedID = hitID - 1; 
                SetWrongWay(true);
                Debug.Log($"Backtracked into {hitID}! Progress dropped to {_lastClearedID}.");
            }
            // --- CASE 4: SKIPPING ---
            else if (hitID > _lastClearedID + 1)
            {
                SetWrongWay(true);
                Debug.Log($"Gate Skipped! Hit {hitID} but expected {_lastClearedID + 1}");
            }
        }

        /*!
        @brief Toggle the wrong-way state and refresh lap UI.
        @param active True to show wrong-way state, false to clear it.
        */
        private void SetWrongWay(bool active)
        {
            if (_isWrongWay == active) return; 
            _isWrongWay = active;
            UpdateLapUI();
        }

        /*!
        @brief Update lap/wrong-way text display.
        */
        private void UpdateLapUI()
        {
            if (lapDisplay == null || lapDisplay.ID == 0) return;

            if (_isWrongWay)
            {
                InternalCalls.SetText(lapDisplay.ID, "WRONG WAY!");
            }
            else
            {
                InternalCalls.SetText(lapDisplay.ID, $"Lap: [{(CurrentLap + 1)} / {TotalLaps}]");
            }
        }

        /*!
        @brief Finalize the race, stop recording, and show the name input UI for score submission.
        */
        private void CompleteRace()
        {
            _isRacing = false;
            FinalTime = _raceTime;
            ReplayManager.StopRecording();
            ReplayManager.SaveRecording($"{TrackName}_Ghost");

            if (lapDisplay != null && lapDisplay.ID != 0)
                InternalCalls.SetText(lapDisplay.ID, "FINISHED!");

            if (nameInputPanel != null && nameInputPanel.ID != 0)
                InternalCalls.SetActive(nameInputPanel.ID, true);
        }

        /*!
        @brief Format a time value (seconds) into a UI-friendly string.
        @param timeInSeconds Time in seconds.
        @return Formatted string in MM:SS.xx format.
        */
        private string FormatTime(float timeInSeconds)
        {
            int minutes = (int)(timeInSeconds / 60f);
            int seconds = (int)(timeInSeconds % 60f);
            int milliseconds = (int)((timeInSeconds * 100f) % 100f);
            return $"{minutes:D2}:{seconds:D2}.{milliseconds:D2}";
        }
    }
}
