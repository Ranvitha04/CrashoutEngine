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
        private bool _isMusic = true;
        [SerializeField] private float smoothingSpeed = 15.0f;

        // IMPORTANT: In a 100-height UI system, 200.0f is VERY wide (wider than the screen).
        // Try setting this to something like 30.0f or 40.0f in the Inspector.
        [SerializeField] private float clickWidth = 30.0f;
        [SerializeField] private float clickHeight = 10.0f;
        //[SerializeField] private bool drawHitbox = true;

        private bool _isDragging = false;
        private bool wasDraggingLastFrame = false;
        private float _currentT = 1.0f;
        private float _targetT = 1.0f;
        private TransformComponent _tf;

        public override void Start()
        {
            _tf = Entity.GetComponent<TransformComponent>();
            if (_tf == null) return;

            string entityName = InternalCalls.GetName(Entity.ID);
            _isMusic = !entityName.Contains("SFX");

            _currentT = _isMusic ? InternalCalls.Audio_GetMusicVolume() : InternalCalls.Audio_GetSFXVolume();
            _targetT = _currentT;
            _tf.Size = new CEVec2(_currentT, 1.0f);
        }

        public override void Update()
        {
            if (_tf == null) return;

            // 1. Mouse & UI Space Calculation (Matches SceneUtils.cpp)
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

            // 3. Robust Dragging Logic
            // Start dragging if clicked inside; stop if mouse released anywhere
            if (Input.IsMouseButtonDown(0) && isHovering) _isDragging = true;
            if (!Input.IsMouseButtonDown(0)) _isDragging = false;

            if (_isDragging)
            {
                float offset = mouseUI.x - hitMinX;
                _targetT = Math.Max(0.0f, Math.Min(offset / clickWidth, 1.0f));
            }

            // 4. Volume Smoothing
            float dt = (float)InternalCalls.TimeGetDT();
            if (Math.Abs(_currentT - _targetT) > 0.001f)
            {
                _currentT = _currentT + (_targetT - _currentT) * Math.Min(dt * smoothingSpeed, 1.0f);
                if (_isMusic) AudioManager.Instance.MusicVolume(_currentT);
                else AudioManager.Instance.SFXVolume(_currentT);
            }

            // 5. Trigger a settings save when the user stops dragging
            if (!_isDragging && wasDraggingLastFrame)
            {
                InternalCalls.Settings_Save();
            }
            wasDraggingLastFrame = _isDragging;

            // 6. Apply local scale instead of absolute size
            // This allows the hierarchy to calculate the visual width correctly
            CEVec2 newLocalScale = new CEVec2(_currentT, 1.0f);
            InternalCalls.TransformSetLocalScale(Entity.ID, ref newLocalScale);

            // Debugging
            // if (drawHitbox)
            // {
            //     CEVec3 color = isHovering ? new CEVec3(0, 1, 0) : new CEVec3(1, 0, 0);
            //     DrawDebugRect(hitMinX, hitMaxX, hitMinY, hitMaxY, worldPos.z, color);
            // }
        }
        // private void DrawDebugRect(float minX, float maxX, float minY, float maxY, float z, CEVec3 color)
        // {
        //     CEVec3 tl = new CEVec3(minX, maxY, z);
        //     CEVec3 tr = new CEVec3(maxX, maxY, z);
        //     CEVec3 bl = new CEVec3(minX, minY, z);
        //     CEVec3 br = new CEVec3(maxX, minY, z);
        //     float thick = 2.0f;
        //     Debug.DrawLine(ref tl, ref tr, thick, ref color);
        //     Debug.DrawLine(ref tr, ref br, thick, ref color);
        //     Debug.DrawLine(ref br, ref bl, thick, ref color);
        //     Debug.DrawLine(ref bl, ref tl, thick, ref color);
        // }
    }
}