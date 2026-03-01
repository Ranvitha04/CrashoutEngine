/*!
@file       CameraController.cs
@author     lee.r@digipen.edu
@date       01/03/2026
@brief      Handles camera controller with relation to player.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class CameraController : ScriptBase
    {
        [SerializeField] public string playerEntityName = "Player";
        
        // --- X-Axis Settings ---
        [SerializeField] public float lookAheadDistance = 50.0f;
        [SerializeField] public float smoothSpeedX = 4.0f;

        // --- Y-Axis Settings ---
        [SerializeField] public float smoothSpeedY = 4.0f;
        [SerializeField] public float defaultOffsetY = 20.0f;

        // The "Look At" object set by your drop-off zones
        public static Entity CurrentLookAtTarget = null;

        // The custom speed requested by the active zone (-1 means use default)
        public static float ActiveZoneDampingY = -1.0f;

        // Dedicated speed for how fast the camera swings when you turn around
        [SerializeField] public float directionChangeSpeed = 2.0f;
        // Tracks the current active offset
        private float currentLookAhead = 0.0f;

        private Entity playerEntity;
        private TransformComponent playerTf;
        private TransformComponent camTf;

        public override void Start()
        {
            camTf = Entity.GetComponent<TransformComponent>();
            playerEntity = Entity.Find(playerEntityName);
            
            if (playerEntity != null)
            {
                playerTf = playerEntity.GetComponent<TransformComponent>();
            }
        }

        public override void LateUpdate()
        {
            if (playerTf == null || camTf == null) return;

            // ==========================================
            // 1. X-AXIS: Look Ahead Logic
            // ==========================================
            float dt = (float)Time.FixedDT;
            
            // Step A: Determine where the offset WANTS to be
            float targetLookAhead = 0.0f;
            if (PlayerMovement.Instance != null)
            {
                targetLookAhead = PlayerMovement.Instance.facingRight ? lookAheadDistance : -lookAheadDistance;
            }

            // Step B: Smoothly transition the offset itself! (This dampens the turn-around whip)
            currentLookAhead = Lerp(currentLookAhead, targetLookAhead, directionChangeSpeed * dt);

            // Step C: Apply the smoothed offset to the player's actual position
            float targetX = playerTf.Position.x + currentLookAhead;

            // ==========================================
            // 2. Y-AXIS: Average Position Logic
            // ==========================================
            float targetY = playerTf.Position.y + defaultOffsetY;
            
            // We set up a variable to hold the current speed, defaulting to your normal smoothSpeedY
            float currentSpeedY = smoothSpeedY; 

            if (CurrentLookAtTarget != null)
            {
                TransformComponent lookAtTf = CurrentLookAtTarget.GetComponent<TransformComponent>();
                if (lookAtTf != null)
                {
                    // Average the y position
                    targetY = (playerTf.Position.y + lookAtTf.Position.y) / 2.0f;

                    // If the zone provided a valid custom speed, overwrite the default!
                    if (ActiveZoneDampingY > 0.0f)
                    {
                        currentSpeedY = ActiveZoneDampingY;
                    }
                }
            }

            // ==========================================
            // 3. Smoothly Move the Camera
            // ==========================================
            float newX = Lerp(camTf.Position.x, targetX, smoothSpeedX * dt);
            
            // Notice we are passing currentSpeedY into the Lerp here instead of smoothSpeedY!
            float newY = Lerp(camTf.Position.y, targetY, currentSpeedY * dt);

            camTf.Position = new CEVec3(newX, newY, camTf.Position.z);
        }

        private float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }
    }
}