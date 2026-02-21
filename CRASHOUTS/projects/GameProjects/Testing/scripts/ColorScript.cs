using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class ColorScript : ScriptBase
    {
        public Entity Entity;
        
        // Cache the component so we don't have to search for it every frame
        private ShapeComponent sc;
        
        // A local timer to drive the math
        private float timer = 0.0f;

        public override void Start() 
        { 
            // Grab the ShapeComponent from this specific entity
            sc = Entity.GetComponent<ShapeComponent>();
            
            if (sc == null)
            {
                Debug.Log("ColorScript Warning: No ShapeComponent attached to this Entity!");
            }
            else
            {
                Debug.Log("ColorScript started successfully! Preparing to pulse colors.");
            }
        }

        public override void Update() 
        { 
            // Safety check to prevent engine crashes if the component is missing
            if (sc == null) return;

            // Increment our fake time (assuming roughly 60 FPS for now)
            timer += 0.016f; 

            // Calculate smooth pulsing values between 0.0 and 1.0 using Sine waves
            // Multiplying 'timer' by different amounts makes the RGB values pulse at different speeds
            float r = (float)(Math.Sin(timer * 2.0f) * 0.5 + 0.5);
            float g = (float)(Math.Sin(timer * 3.0f) * 0.5 + 0.5);
            float b = (float)(Math.Sin(timer * 4.0f) * 0.5 + 0.5);

            // Apply the color beautifully in one line using our new struct!
            sc.Color = new CEColor(r, g, b, 1.0f);
        }

        public override void FixedUpdate() 
        { 
            // Leave empty for now (used for physics logic)
        }
    }
}