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

        public Entity lapDisplay;
        public Entity timerDisplay;
        public string TrackName = "Level1"; 
        
        // --- LEADERBOARD SETTINGS ---
        public string playerName = "Player1"; // You can hook this up to a save file/menu later

        private int _targetCheckpointID = 1; 

        private float _raceTime = 0f;
        private bool _isRacing = false;
        private float _currentLapTime = 0f;

        public override void Start()
        {
            Instance = this;
            _raceTime = 0f;
            _currentLapTime = 0f;
            CurrentLap = 0;
            _isRacing = true; 

            if (TotalLaps <= 0) TotalLaps = 3;
            if (TotalCheckpointsInTrack <= 0) TotalCheckpointsInTrack = 3;

            var transform = Entity.GetComponent<TransformComponent>();
            if (transform != null) transform.Rotation = -90.0f;

            var rb = Entity.GetComponent<RigidBodyComponent>();
            if (rb != null) rb.Rotation = -90.0f;

            Debug.Log($"Race Started! Lap {(CurrentLap + 1)} / {TotalLaps}");
            
            // Just start recording!
            ReplayManager.StartRecording();

            if (lapDisplay != null && lapDisplay.ID != 0)
            {
                InternalCalls.SetText(lapDisplay.ID, $"Lap: [{(CurrentLap + 1)} / {TotalLaps}]");
            }
        }

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

                if (lapDisplay != null && lapDisplay.ID != 0 && CurrentLap == 0)
                {
                    InternalCalls.SetText(lapDisplay.ID, $"Lap: [1 / {TotalLaps}]");
                }
            }
        }

        public void CheckpointHit(int hitID)
        {
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
                        Debug.Log("RACE FINISHED! OVERWRITING GHOST DATA...");
                        
                        ReplayManager.StopRecording();

                        ReplayManager.SaveRecording($"{TrackName}_Ghost");

                        _isRacing = false; 
                        
                        if (lapDisplay != null && lapDisplay.ID != 0)
                        {
                            InternalCalls.SetText(lapDisplay.ID, "FINISHED!");
                        }

                        // ==========================================
                        // --- LEADERBOARD SUBMISSION ---
                        // ==========================================
                        Debug.Log($"Submitting final time: {FormatTime(_raceTime)} to leaderboard...");
                        Leaderboard.SubmitTime(playerName, _raceTime);
                    }
                    else
                    {
                        Debug.Log($"Lap {CurrentLap} complete! Split time: {FormatTime(_currentLapTime)}");
                        _currentLapTime = 0f; 

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

        private string FormatTime(float timeInSeconds)
        {
            int minutes = (int)(timeInSeconds / 60f);
            int seconds = (int)(timeInSeconds % 60f);
            int milliseconds = (int)((timeInSeconds * 100f) % 100f);
            return $"{minutes:D2}:{seconds:D2}.{milliseconds:D2}";
        }
    }
}