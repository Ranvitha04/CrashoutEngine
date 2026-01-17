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
        private ColliderComponent col;
        private PlayerMovement playerScript;

        private bool isGrounded = false;
        private bool moveRight = true;
        private float currentTurnTime = 0.0f;
        private bool isChasing = false;
        private bool isGrabbed = false;
        private float flipCooldown = 0.0f;
        // private bool wasMouseDown = false; // Removed

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
            col = Entity.GetComponent<ColliderComponent>();
            playerScript = PlayerMovement.Instance;

            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);

            if (rb != null) rb.LinearVelocity = new CEVec3(0, 0, 0);
            if (tf != null) startPos = tf.Position;

            Console.WriteLine("[Slime] AI Initialized. Home X: " + startPos.x);
        }

        public override void Update()
        {
            if(playerScript != null && playerScript.isPaused) { return; } 
            if (rb == null || tf == null) return;

            float dt = (float)Time.DT;
            if (flipCooldown > 0) flipCooldown -= dt;

            // 0. GRAB LOGIC (Overrides everything)
            float dist = 0.0f;
            if (playerScript != null)
            {
                dist = Distance(tf.Position, playerScript.Entity.GetComponent<TransformComponent>().Position);
                if (dist <= grabRange || isGrabbed) HandleGrabLogic();

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
            else if (Math.Abs(rb.LinearVelocity.x) > 1.0f) 
            {
                targetState = "slimeWalk";
            }
            // Priority 4: Idle
            else
            {
                targetState = "slimeIdle";
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
            bool isDown = Input.IsMouseButtonDown(0);
            CEVec2 mousePos = Input.GetMouseWorldPosition();

            if (isDown)
            {
                // IF NOT GRABBED YET, CHECK IF MOUSE IS OVER
                if (!isGrabbed)
                {
                    // Only try to grab if we just clicked (optional, or just proximity check)
                    float halfW = Math.Abs(tf.Size.x) * 0.5f + 40.0f; // Extra buffer
                    float halfH = Math.Abs(tf.Size.y) * 0.5f + 40.0f;

                    bool mouseOver = mousePos.x >= tf.Position.x - halfW &&
                                     mousePos.x <= tf.Position.x + halfW &&
                                     mousePos.y >= tf.Position.y - halfH &&
                                     mousePos.y <= tf.Position.y + halfH;

                    if (mouseOver)
                    {
                        isGrabbed = true;
                        rb.SetDynamic(false); // Disable physics while holding
                        
                        // Remove Enemy Layer immediately
                        var layer = Entity.GetComponent<LayerComponent>();
                        if (layer != null) layer.RemoveLayer("Enemy");
                    }
                }

                // IF GRABBED, UPDATE POSITION (COMPANION MODE)
                if (isGrabbed && playerScript != null)
                {
                    var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                    
                    // Determine offset based on player direction
                    float offsetX = 60.0f; 
                    if (playerTf.Size.x < 0) offsetX = -60.0f; // If facing Left, offset Left

                    CEVec3 targetPos = new CEVec3(playerTf.Position.x + offsetX, playerTf.Position.y - 5.0f, playerTf.Position.z);
                    tf.Position = targetPos;
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }
            }
            else
            {
                // MOUSE RELEASED -> TOSS
                if (isGrabbed)
                {
                    isGrabbed = false;
                    rb.SetDynamic(true);

                    // Add Enemy Layer back
                    var layer = Entity.GetComponent<LayerComponent>();
                    if (layer != null) layer.AddLayer("Enemy");

                    // TOSS LOGIC (Parabolic)
                    // Calculate vector from Player to Mouse
                    if (playerScript != null)
                    {
                        var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                        float dx = mousePos.x - playerTf.Position.x;
                        float dy = mousePos.y - playerTf.Position.y;

                        // Normalize and apply force
                        float length = (float)Math.Sqrt(dx * dx + dy * dy);
                        if (length > 0.001f)
                        {
                            dx /= length;
                            dy /= length;
                        }

                        // Throw settings
                        float throwForceX = 1500.0f; 
                        float throwForceY = 1500.0f; // Always add some UP force for parabola

                        // If aiming down, still give it a little arc? 
                        // Or just trust the aim. Let's bias Y upwards slightly for a "toss" feel.
                        if (dy < 0.2f) dy = 0.5f; 

                        rb.LinearVelocity = new CEVec3(dx * throwForceX, dy * throwForceY, 0);
                    }
                }
            }
        }

        public void OnCollideEnter(uint otherEntity)
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

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();
            if (layer != null && layer.HasLayer("Ground")) isGrounded = false;
        }

        private float Distance(CEVec3 a, CEVec3 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return (float)Math.Sqrt(dx * dx + dy * dy);
        }
    }
}