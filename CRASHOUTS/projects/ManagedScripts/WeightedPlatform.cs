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
        private TransformComponent pTagTf;
        private RigidBodyComponent pTagRb;

        private bool pTagOnBoard = false;
        private bool movingForward = true;

        private CEVec3 startPos;
        private CEVec3 endPos;

        private float currentYOffset = 0.0f;
        PTagMovement pTagScript;

        public override void Start()
        {
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);

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

            pTagOnBoard = false;
            currentYOffset = 0.0f;
            pTagScript = PTagMovement.Instance;
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag != null && Tag.HasTag("PTag"))
            {
                pTagTf = other.GetComponent<TransformComponent>();
                pTagRb = other.GetComponent<RigidBodyComponent>();
                pTagOnBoard = true;
            }
        }

        public void OnTriggerExit(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag != null && Tag.HasTag("PTag"))
            {
                pTagOnBoard = false;
                pTagTf = null;
                pTagRb = null;
            }
        }

        public override void Update()
        {
            if(pTagScript != null && pTagScript.isPaused) { return; }
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
            // If moving Up/Down automatically, we ignore the pTag's mass.

            float totalY = nextY;

            if (MoveRight)
            {
                float targetYOffset = 0.0f;

                if (pTagOnBoard && pTagRb != null && PTagMovement.Instance != null)
                {
                    if (pTagRb.Mass >= (PTagMovement.Instance.normalMass - 0.1f))
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
            // 4. Sync PTag Position
            // ---------------------------------------------------------
            if (pTagOnBoard && pTagTf != null)
            {
                float deltaX = transform.Position.x - previousPlatformPos.x;
                float deltaY = transform.Position.y - previousPlatformPos.y;

                pTagTf.Position = new CEVec3(
                    pTagTf.Position.x + deltaX,
                    pTagTf.Position.y + deltaY,
                    pTagTf.Position.z
                );
            }
        }
    }
}