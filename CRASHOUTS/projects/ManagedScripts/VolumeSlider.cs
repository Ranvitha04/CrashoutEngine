using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class VolumeBar : ScriptBase
    {
        public bool IsMusicSlider = true;   // true = Music, false = SFX

        // Screen/world X coordinates for left/right edges of the bar
        public float LeftX;
        public float RightX;

        private ButtonComponent button;
        private TransformComponent tf;
        private float baseWidth; // original Size.x

        public override void Start()
        {
            button = Entity.GetComponent<ButtonComponent>();
            tf = Entity.GetComponent<TransformComponent>();

            if (button == null || tf == null)
            {
                Console.WriteLine("[VolumeBar] Missing Button or Transform on entity " + Entity.ID);
                return;
            }

            if (Math.Abs(RightX - LeftX) < 0.0001f)
            {
                Console.WriteLine("[VolumeBar] LeftX and RightX are equal on entity " + Entity.ID);
                return;
            }

            baseWidth = tf.Size.x;  // full bar width

            // Initialise from stored volume
            float v = IsMusicSlider ? AudioManager.playerMusVol : AudioManager.playerSFXVol;
            v = Clamp01(v);
            ApplyValue(v);
        }

        public override void Update()
        {
            if (button == null || tf == null)
                return;

            // Only react while mouse is held on the bar
            if (!button.IsHovered() || !Input.IsMouseButtonDown(0))
                return;

            CEVec2 mouse = Input.GetMousePosition();
            float mouseX = mouse.x;

            // Map mouse X → 0..1 along the bar
            float t = (mouseX - LeftX) / (RightX - LeftX);
            t = Clamp01(t);

            ApplyValue(t);
        }

        private void ApplyValue(float t)
        {
            // Scale bar width
            var size = tf.Size;
            size.x = baseWidth * t;
            tf.Size = size;

            // Send volume
            var audio = AudioManager.GetInstance();
            if (IsMusicSlider)
                audio.MusicVolume(t);
            else
                audio.SFXVolume(t);
        }

        private float Clamp01(float v)
        {
            if (v < 0f) return 0f;
            if (v > 1f) return 1f;
            return v;
        }
    }
}
