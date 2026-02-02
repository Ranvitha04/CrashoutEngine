
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class EnergyUI : ScriptBase
    {
        // Settings
        public int totalBalls = 10;
        public float maxPlayerEnergy = 100.0f;
        public string ballBaseName = "EnergyBall_"; // Expects EnergyBall_0, EnergyBall_1...

        // Tuning
        public float activeAlpha = 1.0f;  // Fully lit
        public float inactiveAlpha = 0.2f; // Dim background (instead of invisible)
        private uint[] ballShapes;

        public override void Start()
        {
            ballShapes = new uint[totalBalls];

            for (int i = 0; i < totalBalls; i++)
            {
                Debug.Log("Test: " + i + " Total is " + totalBalls);
                // Find the existing UI entities in the scene
                uint id = InternalCalls.FindEntityByName(ballBaseName + i);
                ballShapes[i] = id;
                Debug.Log("Reached End");
            }
        }

        public override void Update()
        {
            if (PlayerMovement.Instance == null) return;

            float currentEnergy = PlayerMovement.Instance.CurrentEnergy;
            float energyPerBall = maxPlayerEnergy / totalBalls;

            for (int i = 0; i < totalBalls; i++)
            {
                if (ballShapes[i] == null) continue;
                Debug.Log("hi");

                Entity ballEnt = new Entity(ballShapes[i]);
                bool hasComp = InternalCalls.
                ShapeComponent sc = ballEnt.GetComponent<ShapeComponent>();
                if (sc == null) Debug.Log(ballEnt.ID + " is null");
                else Debug.Log(ballEnt.ID + " is not null");
                Debug.Log("Yippie");

                // Determine the range this ball covers (e.g., Ball 0 covers 0-10, Ball 1 covers 10-20)
                float ballMin = i * energyPerBall;
                float ballMax = (i + 1) * energyPerBall;

                float targetAlpha = inactiveAlpha;

                if (currentEnergy >= ballMax)
                {
                    // Case 1: Energy is fully past this ball -> Full Brightness
                    targetAlpha = activeAlpha;
                }
                else if (currentEnergy > ballMin)
                {
                    // Case 2: Energy is INSIDE this ball -> Fraction transparency
                    float remainder = currentEnergy - ballMin;
                    float fraction = remainder / energyPerBall;

                    // Interpolate between dim and full
                    // Example: 0.5 fraction results in half-way transparency
                    targetAlpha = inactiveAlpha + (fraction * (activeAlpha - inactiveAlpha));
                }
                else
                {
                    // Case 3: Energy hasn't reached this ball -> Dim/Background
                    targetAlpha = inactiveAlpha;
                }

                // Apply the alpha using our new C++ binding
                sc.Alpha = targetAlpha;
            }
        }
    }
}