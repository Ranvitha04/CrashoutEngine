/*!
@file       VolumeSlider.cs
@author     mohamadamirakmal.b@digipen.edu
@date       26/11/2025
@brief      Handles volume slider UI logic for music and SFX volume control.
*/
using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class VolumeSlider : ScriptBase
    {
        [SerializeField] private bool isMusicVolume = true;
        [SerializeField] private float smoothingSpeed = 15.0f;

        [SerializeField] private float clickWidth = 30.0f;
        [SerializeField] private float clickHeight = 10.0f;

        private bool _isDragging = false;
        private bool wasDraggingLastFrame = false;
        private float _currentT = 1.0f;
        private float _targetT = 1.0f;
        private TransformComponent _tf;

        public override void Start()
        {
            _tf = Entity.GetComponent<TransformComponent>();
            if (_tf == null) return;

            // Read from AudioManager preferences instead of live volume to avoid pause-dip values (30%/40%)
            _currentT = isMusicVolume ? AudioManager.playerMusVol : AudioManager.playerSFXVol;
            _targetT = _currentT;
            
            // Sync local scale immediately
            CEVec2 initialScale = new CEVec2(_currentT, 1.0f);
            InternalCalls.TransformSetLocalScale(Entity.ID, ref initialScale);
        }

        public override void Update()
        {
            if (_tf == null) return;

            // 1. Mouse & UI Space Calculation
            InternalCalls.GetGameMousePos(out CEVec2 rawMouse);
            InternalCalls.GetGameWindowSize(out CEVec2 gameSize);

            float aspect = gameSize.x / gameSize.y;
            float uiHeight = 100.0f;
            float uiWidth = uiHeight * aspect;

            float ndcX = (rawMouse.x / gameSize.x) * 2.0f - 1.0f;
            float ndcY = 1.0f - (rawMouse.y / gameSize.y) * 2.0f;
            CEVec2 mouseUI = new CEVec2(ndcX * (uiWidth * 0.5f), ndcY * (uiHeight * 0.5f));

            // 2. Position & Hitbox
            CEVec3 worldPos = _tf.WorldPosition;
            float hitMinX = worldPos.x;
            float hitMaxX = worldPos.x + clickWidth;
            float hitMinY = worldPos.y - (clickHeight * 0.5f);
            float hitMaxY = worldPos.y + (clickHeight * 0.5f);

            bool isHovering = (mouseUI.x >= hitMinX && mouseUI.x <= hitMaxX &&
                               mouseUI.y >= hitMinY && mouseUI.y <= hitMaxY);

            // 3. Dragging Logic
            if (Input.IsMouseButtonDown(0) && isHovering) _isDragging = true;
            if (!Input.IsMouseButtonDown(0)) _isDragging = false;

            if (_isDragging)
            {
                float offset = mouseUI.x - hitMinX;
                _targetT = Math.Max(0.0f, Math.Min(offset / clickWidth, 1.0f));
            }

            // 4. Volume Smoothing & Application
            float dt = (float)InternalCalls.TimeGetDT();
            if (Math.Abs(_currentT - _targetT) > 0.001f)
            {
                _currentT = _currentT + (_targetT - _currentT) * Math.Min(dt * smoothingSpeed, 1.0f);
                if (isMusicVolume) AudioManager.Instance.MusicVolume(_currentT);
                else AudioManager.Instance.SFXVolume(_currentT);
            }

            // 5. Settings Save
            if (!_isDragging && wasDraggingLastFrame)
            {
                InternalCalls.Settings_Save();
            }
            wasDraggingLastFrame = _isDragging;

            // 6. Update Visual Scale
            CEVec2 newLocalScale = new CEVec2(_currentT, 1.0f);
            InternalCalls.TransformSetLocalScale(Entity.ID, ref newLocalScale);
        }
    }
}