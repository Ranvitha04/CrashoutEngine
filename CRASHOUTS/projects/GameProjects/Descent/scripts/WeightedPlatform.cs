/*!
@file       WeightedPlatform.cs
@author     lee.r@digipen.edu
@date       27/11/2025
@brief      Weighted Platform Script - Makes a platform that moves and sinks under player weight.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.InteropServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class WeightedPlatform : ScriptBase
    {
        [SerializeField] bool IsMoving = true;
        [SerializeField] bool MoveRight = true;
        [SerializeField] float MovementDistance = 100.0f;
        [SerializeField] float MoveSpeed = 20.0f;
        [SerializeField] float VerticalSpeed = 50.0f;
        [SerializeField] float maxDrop = 50.0f;

        private TransformComponent transform;
        private TransformComponent playerTf;
        private RigidBodyComponent playerRb;

        private bool playerOnBoard = false;
        private bool movingForward = true;

        private CEVec3 startPos;
        private CEVec3 endPos;

        private float currentYOffset = 0.0f;
        PlayerMovement playerScript;

        public override void Start()
        {
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);

            transform = Entity.GetComponent<TransformComponent>();

            if (transform != null)
            {
                startPos = transform.Position;
                if (MoveRight)
                    endPos = new CEVec3(startPos.x + MovementDistance, startPos.y, startPos.z);
                else
                    endPos = new CEVec3(startPos.x, startPos.y + MovementDistance, startPos.z);
            }

            playerOnBoard = false;
            currentYOffset = 0.0f;
            playerScript = PlayerMovement.Instance;
        }

        // --- PHYSICS CALLBACKS ---
        public void OnCollideStay(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Player"))
            {
                playerTf = other.GetComponent<TransformComponent>();
                playerRb = other.GetComponent<RigidBodyComponent>();
                playerOnBoard = true;
            }
        }

        public void OnCollideExit(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Player"))
            {
                playerOnBoard = false;
                playerTf = null;
                playerRb = null;
            }
        }

        // --- USE FIXED UPDATE FOR PHYSICS SYNC ---
        public override void FixedUpdate()
        {
            if (playerScript != null) playerScript = PlayerMovement.Instance;
            if (playerScript.isPaused) return;
            if (transform == null) return;

            // Use FixedDT for smoother physics interaction
            float dt = (float)Time.FixedDT;
            CEVec3 previousPlatformPos = transform.Position;

            float dirSign = Math.Sign(MovementDistance);
            if (dirSign == 0) dirSign = 1;

            float step = MoveSpeed * dt * dirSign;
            float nextX = transform.Position.x;
            float nextY = transform.Position.y;

            // 1. MOVEMENT LOGIC
            if (IsMoving)
            {
                if (MoveRight) // Horizontal
                {
                    nextY = startPos.y;
                    if (movingForward)
                    {
                        nextX += step;
                        if ((MovementDistance > 0 && nextX >= endPos.x) || (MovementDistance < 0 && nextX <= endPos.x))
                        {
                            nextX = endPos.x;
                            movingForward = false;
                        }
                    }
                    else
                    {
                        nextX -= step;
                        if ((MovementDistance > 0 && nextX <= startPos.x) || (MovementDistance < 0 && nextX >= startPos.x))
                        {
                            nextX = startPos.x;
                            movingForward = true;
                        }
                    }
                }
                else // Vertical
                {
                    nextX = startPos.x;
                    if (movingForward)
                    {
                        nextY += step;
                        if ((MovementDistance > 0 && nextY >= endPos.y) || (MovementDistance < 0 && nextY <= endPos.y))
                        {
                            nextY = endPos.y;
                            movingForward = false;
                        }
                    }
                    else
                    {
                        nextY -= step;
                        if ((MovementDistance > 0 && nextY <= startPos.y) || (MovementDistance < 0 && nextY >= startPos.y))
                        {
                            nextY = startPos.y;
                            movingForward = true;
                        }
                    }
                }
            }

            // 2. WEIGHT LOGIC (SINKING)
            float totalY = nextY;
            if (MoveRight)
            {
                float targetYOffset = 0.0f;
                if (playerOnBoard && playerRb != null && PlayerMovement.Instance != null)
                {
                    if (playerRb.Mass >= (PlayerMovement.Instance.normalMass - 0.1f))
                        targetYOffset = -maxDrop;
                }

                if (currentYOffset > targetYOffset)
                {
                    currentYOffset -= VerticalSpeed * dt;
                    if (currentYOffset < targetYOffset) currentYOffset = targetYOffset;
                }
                else if (currentYOffset < targetYOffset)
                {
                    currentYOffset += VerticalSpeed * dt;
                    if (currentYOffset > targetYOffset) currentYOffset = targetYOffset;
                }
                totalY = startPos.y + currentYOffset;
            }

            // 3. APPLY PLATFORM POSITION
            transform.Position = new CEVec3(nextX, totalY, transform.Position.z);

            // 4. SYNC PLAYER (THE FIX)
            if (playerOnBoard && playerTf != null)
            {
                float deltaX = transform.Position.x - previousPlatformPos.x;
                float deltaY = transform.Position.y - previousPlatformPos.y;

                // --- KEY FIX: DETECT PLAYER INPUT ---
                // If player is pressing A or D, they are controlling their own X velocity.
                // We should NOT add the platform delta X, or they will fight against it.
                bool isPlayerMovingInput = Input.IsKeyPressed(Key.A) || Input.IsKeyPressed(Key.D);

                if (isPlayerMovingInput)
                {
                    deltaX = 0; // Don't carry them horizontally if they are running
                }

                // Apply changes
                playerTf.Position = new CEVec3(
                    playerTf.Position.x + deltaX,
                    playerTf.Position.y + deltaY, // Always carry vertically
                    playerTf.Position.z
                );
            }
        }
    }
}
