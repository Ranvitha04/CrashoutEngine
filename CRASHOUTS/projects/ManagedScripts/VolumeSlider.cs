using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class VolumeSlider : ScriptBase
    {
        // --- SETTINGS ---
        private bool _isMusic = true;

        [SerializeField]
        private float smoothingSpeed = 15.0f;

        // --- STATE ---
        private bool _isDragging = false;
        private float _currentT = 1.0f;
        private float _targetT = 1.0f;

        // --- CACHED DATA ---
        private TransformComponent _tf;
        private float _fullWidth;
        private float _leftEdgeX;   // This acts as our "Origin" (0.0 volume)
        private float _height;

        public override void Start()
        {
            _tf = Entity.GetComponent<TransformComponent>();
            if (_tf == null) return;

            // 1. DETERMINE TYPE
            var nameComp = Entity.GetComponent<NameComponent>();
            string name = nameComp != null ? nameComp.ToString() : "";
            _isMusic = !name.Contains("SFX");

            // 2. GET CURRENT VOLUME
            if (_isMusic) _currentT = AudioManager.playerMusVol;
            else _currentT = AudioManager.playerSFXVol;

            _targetT = _currentT;

            // 3. RECOVER DIMENSIONS
            // Recover 100% width based on current scale
            float tCalc = _currentT < 0.01f ? 0.01f : _currentT;
            _fullWidth = _tf.Size.x / tCalc;
            _height = _tf.Size.y;

            // 4. CALCULATE ANCHOR (Visual Left Edge)
            _leftEdgeX = _tf.Position.x - (_tf.Size.x / 2.0f);

            // 5. UPDATE VISUALS
            UpdateVisuals(_currentT);
        }

        public override void Update()
        {
            if (_tf == null) return;

            // 1. INPUT
            CEVec2 mousePos = Input.GetMouseWorldPosition();

            float hitMinX = _leftEdgeX;
            float hitMaxX = _leftEdgeX + _fullWidth;
            float hitMinY = _tf.Position.y - (_height / 2.0f);
            float hitMaxY = _tf.Position.y + (_height / 2.0f);

            bool isHovering = (mousePos.x >= hitMinX && mousePos.x <= hitMaxX &&
                               mousePos.y >= hitMinY && mousePos.y <= hitMaxY);

            if (Input.IsMouseButtonDown(0) && isHovering) _isDragging = true;
            if (Input.IsMouseButtonReleased(0)) _isDragging = false;

            // 2. CALCULATE TARGET
            if (_isDragging)
            {
                if (mousePos.x <= _leftEdgeX)
                {
                    _targetT = 0.0f;
                }
                else
                {
                    float relativeX = mousePos.x - _leftEdgeX;
                    _targetT = relativeX / _fullWidth;
                    if (_targetT > 1.0f) _targetT = 1.0f;
                }
            }

            // 3. SMOOTHING LOGIC
            if (Math.Abs(_currentT - _targetT) > 0.001f)
            {
                float dt = (float)Time.DT;
                float t = dt * smoothingSpeed;
                if (t > 1.0f) t = 1.0f;

                _currentT = _currentT + (_targetT - _currentT) * t;

                // Only apply AUDIO volume when it actually changes
                ApplyVolume(_currentT);
            }

            // 4. FORCE VISUALS (EVERY FRAME)
            // We moved this OUTSIDE the if-check. 
            // This prevents the Layout system from snapping the bar back to the center 
            // when you stop dragging.
            UpdateVisuals(_currentT);
        }

        private void UpdateVisuals(float t)
        {
            // Ensure we never scale below a tiny sliver (prevents physics/selection bugs)
            float effectiveT = Math.Max(t, 0.001f);

            float newWidth = _fullWidth * effectiveT;

            // Size
            CEVec2 siz = _tf.Size;
            siz.x = newWidth;
            _tf.Size = siz;

            // Position (Left Pivot)
            CEVec3 pos = _tf.Position;
            pos.x = _leftEdgeX + (newWidth / 2.0f);
            _tf.Position = pos;
        }

        private void ApplyVolume(float t)
        {
            if (_isMusic) AudioManager.GetInstance().MusicVolume(t);
            else AudioManager.GetInstance().SFXVolume(t);
        }

        public override void OnDestroy() { }
    }
}