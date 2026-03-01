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
            float targetX = playerTf.Position.x;
            
            // Read the public facingRight boolean from the Player
            if (PlayerMovement.Instance != null)
            {
                if (PlayerMovement.Instance.facingRight)
                    targetX += lookAheadDistance;
                else
                    targetX -= lookAheadDistance;
            }

            // ==========================================
            // 2. Y-AXIS: Average Position Logic
            // ==========================================
            float targetY = playerTf.Position.y + defaultOffsetY;

            if (CurrentLookAtTarget != null)
            {
                TransformComponent lookAtTf = CurrentLookAtTarget.GetComponent<TransformComponent>();
                if (lookAtTf != null)
                {
                    // "average the y position between that and the player"
                    targetY = (playerTf.Position.y + lookAtTf.Position.y) / 2.0f;
                }
            }

            // ==========================================
            // 3. Smoothly Move the Camera
            // ==========================================
            float dt = (float)Time.DT;
            float newX = Lerp(camTf.Position.x, targetX, smoothSpeedX * dt);
            float newY = Lerp(camTf.Position.y, targetY, smoothSpeedY * dt);

            camTf.Position = new CEVec3(newX, newY, camTf.Position.z);
        }

        private float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }
    }
}