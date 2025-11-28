using System;
using System.Net;
using System.Numerics;
using System.Threading;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SlimeMovement : ScriptBase
    {
        // --- SETTINGS ---
        [SerializeField] private float moveSpeed = 5000f;
        [SerializeField] private float maxSpeed = 50.0f;
        [SerializeField] private float turnTimer = 2.0f;
        [SerializeField] private float detectionRange = 300.0f;
        [SerializeField] private float gravityScale = 5.0f;
        [SerializeField] private float grabRange = 100.0f;
        [SerializeField] private float grabEnergy = 6.25f;

        // NEW: Maximum distance from start position (Patrol Zone)
        [SerializeField] private float MaxDist = 200.0f;

        // --- STATE ---
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim; // The Animator
        private PlayerMovement playerScript;

        private bool isGrounded = false;
        private bool moveRight = true;
        private float currentTurnTime = 0.0f;
        private bool isChasing = false;
        private bool isGrabbed = false;
        private float flipCooldown = 0.0f;

        // NEW: Home Position
        private CEVec3 startPos;
        private bool returningHome = false;

        // Animation State Tracking
        private string currentAnimState = "";

        public override void Start()
        {
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>(); // Get Animator
            playerScript = PlayerMovement.Instance;

            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnTriggerExit);

            if (rb != null) rb.LinearVelocity = new CEVec3(0, 0, 0);
            if (tf != null) startPos = tf.Position;

            Console.WriteLine("[Slime] AI Initialized. Home X: " + startPos.x);
        }

        public override void Update()
        {
            if (rb == null || tf == null) return;

            float dt = (float)Time.DT;
            if (flipCooldown > 0) flipCooldown -= dt;

            // 0. GRAB LOGIC (Overrides everything)
            float dist = 0.0f;
            if (playerScript != null)
            {
                dist = Distance(tf.Position, playerScript.Entity.GetComponent<TransformComponent>().Position);
                if (dist <= grabRange) HandleGrabLogic();

                if (isGrabbed)
                {
                    UpdateAnimationState(); // Update anim even when grabbed
                    return;
                }
            }

            // 1. CHECK PLAYER (CHASE LOGIC)
            isChasing = false;
            if (playerScript != null)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                    dist = Distance(tf.Position, playerTf.Position);
                    if (dist < detectionRange)
                    {
                        isChasing = true;
                        returningHome = false;
                        moveRight = (playerTf.Position.x > tf.Position.x);
                    }
                }
            }

            // 2. PATROL / RETURN LOGIC (If not chasing)
            if (!isChasing && isGrounded)
            {
                float distFromHome = tf.Position.x - startPos.x;

                // A. If OUTSIDE zone -> Return Home
                if (Math.Abs(distFromHome) > MaxDist)
                {
                    returningHome = true;
                }

                // B. Returning Home Logic
                if (returningHome)
                {
                    moveRight = (startPos.x > tf.Position.x);
                    if (Math.Abs(distFromHome) < 10.0f)
                    {
                        returningHome = false;
                        currentTurnTime = turnTimer;
                    }
                }
                // C. Normal Patrolling
                else
                {
                    if (distFromHome > MaxDist && moveRight)
                    {
                        moveRight = false;
                        rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
                    }
                    else if (distFromHome < -MaxDist && !moveRight)
                    {
                        moveRight = true;
                        rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
                    }

                    currentTurnTime -= dt;
                    if (currentTurnTime <= 0)
                    {
                        moveRight = !moveRight;
                        currentTurnTime = turnTimer;
                    }
                }
            }

            // 3. VISUALS (Flip)
            float absX = Math.Abs(tf.Size.x);
            if (moveRight) tf.Size = new CEVec2(absX, tf.Size.y);
            else tf.Size = new CEVec2(-absX, tf.Size.y);

            // 4. ANIMATION UPDATE
            UpdateAnimationState();
        }

        private void UpdateAnimationState()
        {
            if (anim == null) return;

            string targetState = "";

            // Priority 1: Grabbed
            if (isGrabbed)
            {
                targetState = "slimeGrabbed";
            }
            // Priority 2: Chasing
            else if (isChasing)
            {
                targetState = "slimeChase";
            }
            // Priority 3: Moving (Patrolling or Returning)
            else if (Math.Abs(rb.LinearVelocity.x) > 1.0f) // Simple threshold to detect movement
            {
                targetState = "slimeWalk";
            }
            // Priority 4: Idle
            else
            {
                targetState = "slimeIdle"; // Or "slimeWalk" if you don't have idle
            }

            // Apply if changed
            if (targetState != currentAnimState)
            {
                anim.Play(targetState, true);
                currentAnimState = targetState;
            }
        }

        public override void FixedUpdate()
        {
            if (rb == null || isGrabbed) return;

            CEVec3 velocity = rb.LinearVelocity;
            float dir = moveRight ? 1.0f : -1.0f;
            float speedMult = (isChasing || returningHome) ? 1.5f : 1.0f;

            if (playerScript != null && !playerScript.alive) return;

            if (Math.Abs(velocity.x) < maxSpeed * speedMult)
            {
                rb.AddForce(new CEVec3(moveSpeed * dir * speedMult, 0, 0));
            }

            if (velocity.y < 0)
            {
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass, 0f));
            }
        }

        private void HandleGrabLogic()
        {
            if (Input.IsMouseButtonDown(0))
            {
                CEVec2 mousePos = Input.GetMouseWorldPosition();
                float halfW = Math.Abs(tf.Size.x) * 0.5f + 20.0f;
                float halfH = Math.Abs(tf.Size.y) * 0.5f + 20.0f;

                bool mouseOver = mousePos.x >= tf.Position.x - halfW &&
                                 mousePos.x <= tf.Position.x + halfW &&
                                 mousePos.y >= tf.Position.y - halfH &&
                                 mousePos.y <= tf.Position.y + halfH;

                if (mouseOver)
                {
                    float val = grabEnergy * (float)Time.DT;
                    playerScript.RemoveEnergy(val);
                    isGrabbed = true;
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                    tf.Position = new CEVec3(mousePos.x, mousePos.y, tf.Position.z);
                }
            }
            else
            {
                if (isGrabbed) isGrabbed = false;
            }
        }

        public void OnTriggerEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();
            var otherTf = other.GetComponent<TransformComponent>();

            if (layer != null && layer.HasLayer("Ground"))
            {
                if (otherTf != null && tf.Position.y > otherTf.Position.y + (otherTf.Size.y * 0.4f))
                {
                    isGrounded = true;
                }
                else
                {
                    if (!isChasing && !returningHome && flipCooldown <= 0)
                    {
                        moveRight = !moveRight;
                        rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
                        flipCooldown = 0.2f;
                    }
                }
            }
        }

        public void OnTriggerExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();
            if (layer != null && layer.HasLayer("Ground")) isGrounded = false;
        }

        // Placeholders
        public void OnTriggerStay(uint e) { }
        public void OnCollideEnter(uint e) { }
        public void OnCollideStay(uint e) { }
        public void OnCollideExit(uint e) { }

        private float Distance(CEVec3 a, CEVec3 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return (float)Math.Sqrt(dx * dx + dy * dy);
        }
    }
}