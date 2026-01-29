using System;
using System.Runtime.InteropServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class WeightedPlatform : ScriptBase
    {
        [SerializeField]
        bool IsMoving = true;
        [SerializeField]
        bool MoveRight = true; // TRUE = Horizontal + Weighted. FALSE = Vertical + Rigid.
        [SerializeField]
        float MovementDistance = 100.0f;
        [SerializeField]
        float MoveSpeed = 20.0f;

        [SerializeField]
        float VerticalSpeed = 50.0f; // Sinking speed for Horizontal platforms
        [SerializeField]
        float maxDrop = 50.0f;       // Sink depth

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
            CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, OnCollideExit);

            transform = Entity.GetComponent<TransformComponent>();

            if (transform != null)
            {
                startPos = transform.Position;

                // ---------------------------------------------------------
                // CHANGED: Determine End Position based on Mode
                // ---------------------------------------------------------
                if (MoveRight)
                {
                    // Horizontal Mode
                    endPos = new CEVec3(startPos.x + MovementDistance, startPos.y, startPos.z);
                }
                else
                {
                    // Vertical Mode (Up/Down)
                    endPos = new CEVec3(startPos.x, startPos.y + MovementDistance, startPos.z);
                }
            }

            playerOnBoard = false;
            currentYOffset = 0.0f;
            playerScript = PlayerMovement.Instance;
        }

        public void OnCollideEnter(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Player"))
            {
                playerTf = other.GetComponent<TransformComponent>();
                playerRb = other.GetComponent<RigidBodyComponent>();
                playerOnBoard = true;
            }
        }

        public void OnCollideExit(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Player"))
            {
                playerOnBoard = false;
                playerTf = null;
                playerRb = null;
            }
        }

        public override void Update()
        {
            if(playerScript != null && playerScript.isPaused) { return; }
            if (transform == null) return;

            float dt = (float)Time.DT;
            CEVec3 previousPlatformPos = transform.Position;
            float step = MoveSpeed * dt;

            // We calculate the "Next" positions relative to the start
            float nextX = transform.Position.x;
            float nextY = transform.Position.y; // This will act as the "Base Y" for vertical movement

            // ---------------------------------------------------------
            // 1. Movement Logic (Split by Type)
            // ---------------------------------------------------------
            if (IsMoving)
            {
                // TYPE A: Horizontal Movement (Left/Right)
                if (MoveRight)
                {
                    // Keep Y at start (The sinking logic below will modify it later)
                    nextY = startPos.y;

                    if (movingForward)
                    {
                        nextX += step;
                        if (nextX >= endPos.x) movingForward = false;
                    }
                    else
                    {
                        nextX -= step;
                        if (nextX <= startPos.x) movingForward = true;
                    }
                }
                // TYPE B: Vertical Movement (Up/Down)
                else
                {
                    // Keep X at start
                    nextX = startPos.x;

                    if (movingForward)
                    {
                        nextY += step;
                        // Check bounds (Handle both Upward and Downward movement distances)
                        if (MovementDistance > 0 && nextY >= endPos.y) movingForward = false;
                        else if (MovementDistance < 0 && nextY <= endPos.y) movingForward = false;
                    }
                    else
                    {
                        nextY -= step;
                        // Check bounds
                        if (MovementDistance > 0 && nextY <= startPos.y) movingForward = true;
                        else if (MovementDistance < 0 && nextY >= startPos.y) movingForward = true;
                    }
                }
            }

            // ---------------------------------------------------------
            // 2. Weight Logic (Sinking)
            // ---------------------------------------------------------
            // CHANGED: Only apply sinking if we are in Horizontal Mode (MoveRight == true).
            // If moving Up/Down automatically, we ignore the player's mass.

            float totalY = nextY;

            if (MoveRight)
            {
                float targetYOffset = 0.0f;

                if (playerOnBoard && playerRb != null && PlayerMovement.Instance != null)
                {
                    if (playerRb.Mass >= (PlayerMovement.Instance.normalMass - 0.1f))
                    {
                        targetYOffset = -maxDrop;
                    }
                }

                // Smoothly move offset
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

                // Apply offset to the base Y
                totalY = startPos.y + currentYOffset;
            }

            // ---------------------------------------------------------
            // 3. Apply Final Position
            // ---------------------------------------------------------
            transform.Position = new CEVec3(nextX, totalY, transform.Position.z);

            // ---------------------------------------------------------
            // 4. Sync Player Position
            // ---------------------------------------------------------
            if (playerOnBoard && playerTf != null)
            {
                float deltaX = transform.Position.x - previousPlatformPos.x;
                float deltaY = transform.Position.y - previousPlatformPos.y;

                playerTf.Position = new CEVec3(
                    playerTf.Position.x + deltaX,
                    playerTf.Position.y + deltaY,
                    playerTf.Position.z
                );
            }
        }
    }
}