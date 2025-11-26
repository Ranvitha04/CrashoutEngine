using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class VolumeSlider : ScriptBase
    {
        private float _sensitivity = 0.002f;
        private float _minVisualSize = 0.01f;

        private bool _isDragging = false;
        private bool _isMusic = true;
        private float _currentT = 1.0f;
        private float _lastMouseX;

        private ButtonComponent _button;
        private TransformComponent _tf;
        private NameComponent _nameComp;

        private float _maxWidth;
        private float _startCenterX;

        public override void Start()
        {
            _button = Entity.GetComponent<ButtonComponent>();
            _tf = Entity.GetComponent<TransformComponent>();
            _nameComp = Entity.GetComponent<NameComponent>();

            if (_button == null) return;

            string name = _nameComp != null ? _nameComp.ToString() : "";
            _isMusic = !name.Contains("SFX");

            if (_tf != null)
            {
                _maxWidth = _tf.Size.x;
                _startCenterX = _tf.Position.x;
            }

            if (_isMusic) _currentT = AudioManager.playerMusVol;
            else _currentT = AudioManager.playerSFXVol;

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
                _currentT += delta * _sensitivity;

                if (_currentT < 0.0f) _currentT = 0.0f;
                if (_currentT > 1.0f) _currentT = 1.0f;

                UpdateVisuals(_currentT);

                string typeName = _isMusic ? "MUSIC" : "SFX";
                Debug.Log("[VolumeSlider] Adjusting " + typeName + " | Volume: " + _currentT);

                if (_isMusic) AudioManager.GetInstance().MusicVolume(_currentT);
                else AudioManager.GetInstance().SFXVolume(_currentT);

                _lastMouseX = mousePos.x;
            }
        }

        private void UpdateVisuals(float t)
        {
            CEVec3 newPos = _tf.Position;
            CEVec2 newSize = _tf.Size;

            float calculatedWidth = _maxWidth * t;

            float minPixelWidth = _maxWidth * _minVisualSize;

            if (calculatedWidth < minPixelWidth)
            {
                calculatedWidth = minPixelWidth;
            }

            newSize.x = calculatedWidth;

            float originalLeftEdge = _startCenterX - (_maxWidth / 2.0f);
            newPos.x = originalLeftEdge + (newSize.x / 2.0f);

            _tf.Size = newSize;
            _tf.Position = newPos;
        }
    }
}
