using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class LapTracker : ScriptBase
    {
        public static LapTracker Instance { get; private set; }

        public int TotalLaps = 3;
        public int TotalCheckpointsInTrack = 3; 
        private int CurrentLap = 0;

        // UI Entities to drag and drop in the Inspector
        public Entity lapDisplay;
        public Entity timerDisplay;
        public string TrackName = "Level1"; // Used to name the replay file
        
        private int _targetCheckpointID = 1; 

        // --- TIMER VARIABLES ---
        private float _raceTime = 0f;
        private bool _isRacing = false;

        private float _currentLapTime = 0f;
        
        // Changed to track the FULL race time instead of a single lap
        private float _bestRaceTime = float.MaxValue; 

        public override void Start()
        {
            Instance = this;
            _raceTime = 0f;
            _currentLapTime = 0f;
            CurrentLap = 0;
            _isRacing = true; // Start the clock!

            if (TotalLaps <= 0) TotalLaps = 3;
            if (TotalCheckpointsInTrack <= 0) TotalCheckpointsInTrack = 3;

            var transform = Entity.GetComponent<TransformComponent>();
            if (transform != null) transform.Rotation = -90.0f;

            var rb = Entity.GetComponent<RigidBodyComponent>();
            if (rb != null) rb.Rotation = -90.0f;

            Debug.Log($"Race Started! Lap {(CurrentLap + 1)} / {TotalLaps}");
            
            // 1. START RECORDING THE ENTIRE RACE
            ReplayManager.StartRecording();

            if (lapDisplay != null && lapDisplay.ID != 0)
            {
                InternalCalls.SetText(lapDisplay.ID, $"Lap: [{(CurrentLap + 1)} / {TotalLaps}]");
            }
        }

        public override void Update() 
        { 
            // 1. Only tick the timer if the race is actively happening
            if (_isRacing)
            {
                float dt = (float)Time.DT;
                _raceTime += dt; 
                _currentLapTime += dt; // Track the current lap's local time

                // 2. Update the UI every frame
                if (timerDisplay != null && timerDisplay.ID != 0)
                {
                    InternalCalls.SetText(timerDisplay.ID, FormatTime(_raceTime));
                }

                // 3. Force lap display
                if (lapDisplay != null && lapDisplay.ID != 0 && CurrentLap == 0)
                {
                    InternalCalls.SetText(lapDisplay.ID, $"Lap: [1 / {TotalLaps}]");
                }
            }
        }

        public void CheckpointHit(int hitID)
        {
            // If the race is over, ignore further checkpoint hits
            if (!_isRacing) return; 

            if (hitID == _targetCheckpointID)
            {
                _targetCheckpointID++;

                if (_targetCheckpointID >= TotalCheckpointsInTrack)
                {
                    _targetCheckpointID = 0;
                }

                if (hitID == 0)
                {
                    CurrentLap++;
                    
                    if (CurrentLap >= TotalLaps)
                    {
                        Debug.Log("RACE FINISHED! YOU WIN!");
                        
                        // --- FULL RACE GHOST EVALUATION ---
                        ReplayManager.Stop(); // 2. Stop recording NOW (after all 3 laps)

                        if (_raceTime < _bestRaceTime)
                        {
                            _bestRaceTime = _raceTime;
                            Debug.Log($"NEW FULL RACE RECORD! Time: {FormatTime(_bestRaceTime)}");
                            
                            // Save the 3-lap run to disk!
                            ReplayManager.SaveRecording($"{TrackName}_Ghost");
                        }
                        else
                        {
                            Debug.Log($"Race complete. Time: {FormatTime(_raceTime)}. Best is: {FormatTime(_bestRaceTime)}");
                        }
                        // ----------------------------------

                        // Stop the timer!
                        _isRacing = false; 
                        
                        // Final UI Update just to be safe
                        if (lapDisplay != null && lapDisplay.ID != 0)
                        {
                            InternalCalls.SetText(lapDisplay.ID, "FINISHED!");
                        }
                    }
                    else
                    {
                        // The race is still going! Do NOT stop recording.
                        Debug.Log($"Lap {CurrentLap} complete! Split time: {FormatTime(_currentLapTime)}");
                        _currentLapTime = 0f; // Reset for the next lap's split time

                        if (lapDisplay != null && lapDisplay.ID != 0)
                        {
                            InternalCalls.SetText(lapDisplay.ID, $"Lap: [{(CurrentLap + 1)} / {TotalLaps}]");
                        }
                    }
                }
                else 
                {
                    Debug.Log($"Checkpoint {hitID} cleared! Head to {_targetCheckpointID}");
                }
            }
            else if (hitID != _targetCheckpointID - 1)
            {
                Debug.Log($"Wrong Way! Expected Checkpoint {_targetCheckpointID}");
            }
        }

        // Formats a raw float (e.g., 65.432) into a nice race timer string (e.g., "01:05.43")
        private string FormatTime(float timeInSeconds)
        {
            int minutes = (int)(timeInSeconds / 60f);
            int seconds = (int)(timeInSeconds % 60f);
            
            // Extract the milliseconds (e.g. .432 * 100 = 43)
            int milliseconds = (int)((timeInSeconds * 100f) % 100f);

            // Pad with leading zeros using C# string formatting
            return $"{minutes:D2}:{seconds:D2}.{milliseconds:D2}";
        }
    }
}