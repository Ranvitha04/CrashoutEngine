/*!
@file		AudioManager.cs
@author     mohamadamirakmal.b@digipen.edu
@date       26/11/2025
@brief		Declaration and implementation of VolumeSlider class to manage volume slider UI component.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class VolumeSlider : ScriptBase
    {
        private float _sensitivity = 0.002f;

        private bool _isDragging = false;
        private bool _isMusic = true;

        // Use _currentT for data binding
        private float _currentT = 1.0f;
        private float _lastMouseX;

        private ButtonComponent _button;
        private TransformComponent _tf;

        private float _fullWidth;
        private float _leftEdgeX;

        public override void Start()
        {
            _button = Entity.GetComponent<ButtonComponent>();
            _tf = Entity.GetComponent<TransformComponent>();

            if (_button == null)
            {
                Debug.Log("[VolumeSlider] ERROR: No Button");
                return;
            }

            var nameComp = Entity.GetComponent<NameComponent>();
            string name = nameComp != null ? nameComp.ToString() : "";
            _isMusic = !name.Contains("SFX");

            // 1. GET VOLUME FIRST
            if (_isMusic) _currentT = AudioManager.playerMusVol;
            else _currentT = AudioManager.playerSFXVol;

            // 2. CALCULATE FULL WIDTH (Reverse Engineering)
            if (_tf != null)
            {
                // We calculate the Left Edge based on current position and width. 
                // (Center Position - Half Width) is always the Left Edge, regardless of scale.
                _leftEdgeX = _tf.Position.x - (_tf.Size.x / 2.0f);

                // We assume _tf.Size.x represents the size at _currentT.
                // Therefore, FullWidth = CurrentSize / CurrentT.
                // We clamp _currentT to a tiny value to avoid division by zero if volume is 0.
                float tCalc = _currentT < 0.01f ? 0.01f : _currentT;
                _fullWidth = _tf.Size.x / tCalc;
            }

            // 3. Now apply visuals cleanly
            UpdateVisuals(_currentT);
        }

        public override void Update()
        {
            if (_button == null || _tf == null) return;

            CEVec2 mousePos = Input.GetMousePosition();

            if (Input.IsMouseButtonDown(0) && _button.IsHovered())
            {
                _isDragging = true;
                _lastMouseX = mousePos.x;
            }

            if (Input.IsMouseButtonReleased(0))
            {
                _isDragging = false;
            }

            if (_isDragging)
            {
                float delta = mousePos.x - _lastMouseX;
                float nextT = _currentT + delta * _sensitivity;

                if (nextT < 0.0f) nextT = 0.0f;
                if (nextT > 1.0f) nextT = 1.0f;

                // Only update if the value has changed by more than 0.1%
                if (Math.Abs(nextT - _currentT) > 0.001f)
                {
                    _currentT = nextT;
                    UpdateVisuals(_currentT);

                    // REMOVE LATER
                    string type = _isMusic ? "MUSIC" : "SFX";
                    Debug.Log(type + " Vol: " + _currentT);

                    if (_isMusic) AudioManager.GetInstance().MusicVolume(_currentT);
                    else AudioManager.GetInstance().SFXVolume(_currentT);
                }
                _lastMouseX = mousePos.x;
            }
        }
        
        public override void OnDestroy()
        {
            // Cleanup if necessary
        }

        private void UpdateVisuals(float t)
        {
            float newWidth = _fullWidth * t;

            if (newWidth < 1.0f) newWidth = 1.0f;

            CEVec2 siz = _tf.Size;
            siz.x = newWidth;
            _tf.Size = siz;

            CEVec3 pos = _tf.Position;
            pos.x = _leftEdgeX + (newWidth / 2.0f);
            _tf.Position = pos;
        }
    }
}
