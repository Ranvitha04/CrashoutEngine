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
        bool MoveRight = true; // TRUE = Horizontal Mode. FALSE = Vertical Mode.

        [SerializeField]
        float MovementDistance = 100.0f; // Can now be negative (e.g. -100 to move Left/Down)

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
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);

            transform = Entity.GetComponent<TransformComponent>();

            if (transform != null)
            {
                startPos = transform.Position;

                // ---------------------------------------------------------
                // Determine End Position (Works for negative distances now)
                // ---------------------------------------------------------
                if (MoveRight)
                {
                    // Horizontal Mode
                    endPos = new CEVec3(startPos.x + MovementDistance, startPos.y, startPos.z);
                }
                else
                {
                    // Vertical Mode
                    endPos = new CEVec3(startPos.x, startPos.y + MovementDistance, startPos.z);
                }
            }

            playerOnBoard = false;
            currentYOffset = 0.0f;
            playerScript = PlayerMovement.Instance;
        }

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

        public override void Update()
        {
            if (playerScript != null && playerScript.isPaused) { return; }
            if (transform == null) return;

            float dt = (float)Time.DT;
            CEVec3 previousPlatformPos = transform.Position;

            // Get Direction (1 for Positive/Right/Up, -1 for Negative/Left/Down)
            float dirSign = Math.Sign(MovementDistance);
            if (dirSign == 0) dirSign = 1; // Safety

            float step = MoveSpeed * dt * dirSign; // Step now includes direction!

            // "Next" positions relative to start
            float nextX = transform.Position.x;
            float nextY = transform.Position.y;

            // ---------------------------------------------------------
            // 1. Movement Logic (Direction Aware)
            // ---------------------------------------------------------
            if (IsMoving)
            {
                // TYPE A: Horizontal Movement (Left/Right)
                if (MoveRight)
                {
                    nextY = startPos.y; // Reset to base Y (sink logic adds offset later)

                    if (movingForward)
                    {
                        nextX += step; // Add step (which might be negative)

                        // Check if we passed the End Position
                        // If Dist > 0 (Right), check >=. If Dist < 0 (Left), check <=.
                        if ((MovementDistance > 0 && nextX >= endPos.x) ||
                            (MovementDistance < 0 && nextX <= endPos.x))
                        {
                            nextX = endPos.x;
                            movingForward = false;
                        }
                    }
                    else
                    {
                        nextX -= step; // Subtract step to return

                        // Check if we passed Start Position
                        // If Dist > 0 (Right), returning goes Left (<).
                        // If Dist < 0 (Left), returning goes Right (>).
                        if ((MovementDistance > 0 && nextX <= startPos.x) ||
                            (MovementDistance < 0 && nextX >= startPos.x))
                        {
                            nextX = startPos.x;
                            movingForward = true;
                        }
                    }
                }
                // TYPE B: Vertical Movement (Up/Down)
                else
                {
                    nextX = startPos.x;

                    if (movingForward)
                    {
                        nextY += step;

                        // Check bounds (Handle Up vs Down)
                        if ((MovementDistance > 0 && nextY >= endPos.y) ||
                            (MovementDistance < 0 && nextY <= endPos.y))
                        {
                            nextY = endPos.y;
                            movingForward = false;
                        }
                    }
                    else
                    {
                        nextY -= step;

                        // Check bounds
                        if ((MovementDistance > 0 && nextY <= startPos.y) ||
                            (MovementDistance < 0 && nextY >= startPos.y))
                        {
                            nextY = startPos.y;
                            movingForward = true;
                        }
                    }
                }
            }

            // ---------------------------------------------------------
            // 2. Weight Logic (Sinking)
            // ---------------------------------------------------------
            float totalY = nextY;

            if (MoveRight)
            {
                float targetYOffset = 0.0f;

                if (playerOnBoard && playerRb != null && PlayerMovement.Instance != null)
                {
                    // Check if player is heavy enough
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
            // 4. Sync player Position
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