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
        public int CurrentLap = 0;

        // UI Entities to drag and drop in the Inspector
        public Entity lapDisplay;
        public Entity timerDisplay;
        
        private int _targetCheckpointID = 1; 

        // --- NEW TIMER VARIABLES ---
        private float _raceTime = 0f;
        private bool _isRacing = false;

        public override void Start()
        {
            Instance = this;
            _raceTime = 0f;
            _isRacing = true; // Start the clock!

            var transform = Entity.GetComponent<TransformComponent>();
            if (transform != null)
            {
                transform.Rotation = -90.0f; // Points the ship correctly
            }

            Debug.Log($"Race Started! Lap {(CurrentLap + 1)} / {TotalLaps}");

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
                // 2. Add the delta time (time passed since last frame)
                // Assumes your engine's Input or Time class has a GetDeltaTime method.
                // If it's called something else, replace 'Time.DeltaTime' with your engine's equivalent!
                _raceTime += (float)Time.DT; 

                // 3. Update the UI every frame
                if (timerDisplay != null && timerDisplay.ID != 0)
                {
                    InternalCalls.SetText(timerDisplay.ID, FormatTime(_raceTime));
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
                        Debug.Log($"Lap {(CurrentLap + 1)} / {TotalLaps}");
                        
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