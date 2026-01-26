using System;
using System.Net;
using System.Numerics;
using System.Threading;
using System.Collections.Generic;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SlimeMovement : ScriptBase
    {
        // --- SETTINGS ---
        [SerializeField] private float moveSpeed = 600f; // Reduced force
        [SerializeField] private float maxSpeed = 15.0f; // Slower speed
        [SerializeField] private float turnTimer = 2.0f;
        [SerializeField] private float detectionRange = 250.0f;
        [SerializeField] private float gravityScale = 2.0f;
        [SerializeField] private float grabRange = 100.0f;

        // THROW SETTINGS
        [SerializeField] private float minThrowSpeed = 50.0f;
        [SerializeField] private float maxThrowSpeed = 200.0f; 
        [SerializeField] private float maxMouseDragDist = 400.0f;
        [SerializeField] private float airDrag = 1.0f; 
        [SerializeField] private float stunDuration = 1.0f;

        // Patrol Zone
        [SerializeField] private float MaxDist = 200.0f;
        [SerializeField] private float ledgeRayCastDistance = 100.0f;
        [SerializeField] private float ledgeCheckXOffset = 60.0f;

        // --- STATE ---
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim;
        private ColliderComponent col;
        private PlayerMovement playerScript;

        private bool isGrounded = false;
        private bool moveRight = true;
        private float currentTurnTime = 0.0f;
        private bool isChasing = false;
        private bool isGrabbed = false;
        private bool wasThrown = false;
        private bool isStunned = false;
        private float currentStunTime = 0.0f;
        private float flipCooldown = 0.0f;
        private float stuckTimer = 0.0f;

        private CEVec3 startPos;
        private bool returningHome = false;

        // PATH FOLLOWING STATE
        private List<CEVec3> activePath = new List<CEVec3>();
        private bool isFollowingPath = false;
        private int pathIndex = 0;

        private string currentAnimState = "";

        public override void Start()
        {
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>(); 
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

            if (isStunned)
            {
                currentStunTime -= dt;
                if (currentStunTime <= 0)
                {
                    isStunned = false;
                }
                UpdateAnimationState();
                return;
            }

            if (flipCooldown > 0) flipCooldown -= dt;

            // Handle Grab State
            if (playerScript != null)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                     float dist = Distance(tf.Position, playerTf.Position);
                     if (dist <= grabRange || isGrabbed) HandleGrabLogic();
                }
               
                if (isGrabbed)
                {
                    UpdateAnimationState(); 
                    return;
                }
            }

            // Chase logic
            isChasing = false;
            if (playerScript != null)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                    float dist = Distance(tf.Position, playerTf.Position);
                    bool hasLineOfSight = false;
                    
                    if (dist < detectionRange && !isFollowingPath)
                    {
                         CEVec3 start = tf.Position;
                         CEVec3 end = playerTf.Position;
                         uint hitID;
                         bool hit = InternalCalls.Raycast(ref start, ref end, out hitID, Entity.ID);
                         
                         if (hit && hitID == playerScript.Entity.ID)
                         {
                             hasLineOfSight = true;
                         }
                    }

                    if (hasLineOfSight) 
                    {
                        isChasing = true;
                        returningHome = false;
                        moveRight = (playerTf.Position.x > tf.Position.x);
                    }
                }
            }

            // Patrol logic
            if (!isChasing && isGrounded && !isFollowingPath && !isGrabbed)
            {
                // 1. Check for Ledges (Holes)
                float checkDir = moveRight ? 1.0f : -1.0f;
                CEVec3 rayStart = new CEVec3(tf.Position.x + (checkDir * ledgeCheckXOffset), tf.Position.y, tf.Position.z);
                CEVec3 rayEnd = new CEVec3(rayStart.x, rayStart.y - ledgeRayCastDistance, rayStart.z);
                
                uint hitGroundID;
                bool hitGround = InternalCalls.Raycast(ref rayStart, ref rayEnd, out hitGroundID, Entity.ID);

                // Debug Draw (Optional, but helpful)
                // CEVec3 col = hitGround ? new CEVec3(0,1,0) : new CEVec3(1,0,0);
                // Debug.DrawLine(ref rayStart, ref rayEnd, 2.0f, ref col);

                if (!hitGround && flipCooldown <= 0)
                {
                     moveRight = !moveRight;
                     flipCooldown = 0.5f; // Add a small cooldown to prevent rapid flipping
                }
                else
                {
                    // 2. Normal Patrol Distance Check
                    float distFromHome = tf.Position.x - startPos.x;

                    if (returningHome)
                    {
                        moveRight = (startPos.x > tf.Position.x);
                        if (Math.Abs(distFromHome) < 10.0f)
                        {
                            returningHome = false;
                        }
                    }
                    else
                    {
                        if (Math.Abs(distFromHome) > MaxDist)
                        {
                            returningHome = true; 
                        }
                    }
                }
            }

            // Stuck Detection
            if ((isChasing || returningHome || isGrounded) && !isGrabbed && !isFollowingPath)
            {
                if (Math.Abs(rb.LinearVelocity.x) < 2.0f)
                {
                    stuckTimer += dt;
                    if (stuckTimer > 0.5f)
                    {
                        if (!isChasing) moveRight = !moveRight; 
                        else rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 200.0f, 0); 
                        stuckTimer = 0.0f;
                    }
                }
                else
                {
                    stuckTimer = 0.0f;
                }
            }

            // Sprite flip
            float absX = Math.Abs(tf.Size.x);
            if (moveRight) tf.Size = new CEVec2(absX, tf.Size.y);
            else tf.Size = new CEVec2(-absX, tf.Size.y);

            UpdateAnimationState();
        }

        public override void OnDrawGizmos()
        {
            if (tf == null) tf = Entity.GetComponent<TransformComponent>();
            if (tf == null) return;

            // Debug Drawing
            CEVec3 detectionColor = new CEVec3(1.0f, 1.0f, 0.0f); // Yellow
            CEVec2 detectionSize = new CEVec2(detectionRange * 2.0f, detectionRange * 2.0f);
            CEVec3 myPos = tf.Position;
            Debug.DrawCircle(ref detectionSize, ref myPos, ref detectionColor);

            CEVec3 grabColor = new CEVec3(0.0f, 1.0f, 0.0f); // Green
            CEVec2 grabSize = new CEVec2(grabRange * 2.0f, grabRange * 2.0f);
            Debug.DrawCircle(ref grabSize, ref myPos, ref grabColor);
        }

        private void UpdateAnimationState()
        {
            if (anim == null) return;
            string targetState = isGrabbed ? "slimeGrabbed" : 
                                 isStunned ? "slimeCaution" :
                                 isChasing ? "slimeChase" : 
                                 (Math.Abs(rb.LinearVelocity.x) > 1.0f ? "slimeWalk" : "slimeIdle");

            if (targetState != currentAnimState)
            {
                anim.Play(targetState, true);
                currentAnimState = targetState;
            }
        }

        public override void FixedUpdate()
        {
            if (rb == null || isGrabbed || isStunned) return;

            // --- PATH FOLLOWING MODE ---
            if (isFollowingPath)
            {
                if (pathIndex < activePath.Count)
                {
                    CEVec3 target = activePath[pathIndex];
                    CEVec3 current = tf.Position;
                    
                    float dt = (float)Time.FixedDT;
                    if (dt > 0.0001f)
                    {
                        float vx = (target.x - current.x) / dt;
                        float vy = (target.y - current.y) / dt;
                        rb.LinearVelocity = new CEVec3(vx, vy, 0);
                    }
                    pathIndex++;
                }
                else
                {
                    isFollowingPath = false;
                    activePath.Clear();
                }
                return; 
            }

            // --- NORMAL PHYSICS ---
            CEVec3 velocity = rb.LinearVelocity;
            float dir = moveRight ? 1.0f : -1.0f;
            float speedMult = (isChasing || returningHome) ? 1.2f : 1.0f;

            if (playerScript != null && !playerScript.alive) return;

            // Ground movement
            if (isGrounded || isChasing) 
            {
                if (Math.Abs(velocity.x) < maxSpeed * speedMult)
                {
                    rb.AddForce(new CEVec3(moveSpeed * dir * speedMult, 0, 0));
                }
                
                // Manual Friction (Reduced to prevent jitter)
                rb.AddForce(new CEVec3(-velocity.x * 5.0f * rb.Mass, 0, 0));
            }
            else 
            {
                // Air Drag
                rb.AddForce(new CEVec3(-velocity.x * airDrag * rb.Mass, -velocity.y * airDrag * 0.2f * rb.Mass, 0));
            }

            // Gravity
            float gravityForce = -9.81f * 100.0f * gravityScale * rb.Mass;
            rb.AddForce(new CEVec3(0f, gravityForce, 0f));
        }

        private void HandleGrabLogic()
        {
            bool isDown = Input.IsMouseButtonDown(0);
            CEVec2 mousePos = Input.GetMouseWorldPosition();

            if (isDown)
            {
                if (!isGrabbed)
                {
                    float halfW = Math.Abs(tf.Size.x) * 0.5f + 40.0f;
                    float halfH = Math.Abs(tf.Size.y) * 0.5f + 40.0f;

                    if (mousePos.x >= tf.Position.x - halfW && mousePos.x <= tf.Position.x + halfW &&
                        mousePos.y >= tf.Position.y - halfH && mousePos.y <= tf.Position.y + halfH)
                    {
                        isGrabbed = true;
                        isFollowingPath = false;
                        activePath.Clear();
                        rb.SetDynamic(false);
                        var layer = Entity.GetComponent<LayerComponent>();
                        if (layer != null) layer.RemoveLayer("Enemy");
                    }
                }

                if (isGrabbed && playerScript != null)
                {
                    var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                    float offsetX = playerTf.Size.x < 0 ? -60.0f : 60.0f;
                    tf.Position = new CEVec3(playerTf.Position.x + offsetX, playerTf.Position.y - 5.0f, playerTf.Position.z);
                    rb.LinearVelocity = new CEVec3(0, 0, 0);

                    CEVec3 throwVel = CalculateThrowVelocity(mousePos, playerTf.Position);
                    SimulatePath(throwVel, tf.Position, true);
                }
            }
            else if (isGrabbed)
            {
                isGrabbed = false;
                wasThrown = true;
                rb.SetDynamic(true);
                var layer = Entity.GetComponent<LayerComponent>();
                if (layer != null) layer.AddLayer("Enemy");

                if (playerScript != null)
                {
                    CEVec3 throwVel = CalculateThrowVelocity(mousePos, playerScript.Entity.GetComponent<TransformComponent>().Position);
                    
                    SimulatePath(throwVel, tf.Position, false);
                    if (activePath.Count > 0)
                    {
                        isFollowingPath = true;
                        pathIndex = 0;
                    }
                }
            }
        }

        private CEVec3 CalculateThrowVelocity(CEVec2 mousePos, CEVec3 playerPos)
        {
            float dx = playerPos.x - mousePos.x;
            float dy = playerPos.y - mousePos.y;
            float dist = (float)Math.Sqrt(dx * dx + dy * dy);
            
            float dirX = 0f, dirY = 0f;
            if (dist > 0.001f) { dirX = dx / dist; dirY = dy / dist; }

            dist = Math.Min(dist, maxMouseDragDist);
            float speed = minThrowSpeed + (maxThrowSpeed - minThrowSpeed) * (dist / maxMouseDragDist);

            return new CEVec3(dirX * speed, dirY * speed, 0);
        }

        private void SimulatePath(CEVec3 velocity, CEVec3 startPos, bool drawOnly)
        {
            if (!drawOnly) activePath.Clear();

            CEVec3 currentPos = startPos;
            
            float dt = (float)Time.FixedDT;
            if (dt <= 0.0001f) dt = 0.0166f; 

            int steps = (int)(2.0f / dt);

            CEVec3 simVel = velocity;
            float totalAccelY = (-9.81f * 100.0f * gravityScale) - 70.0f;

            for (int i = 0; i < steps; i++)
            {
                simVel.y += totalAccelY * dt;
                
                simVel.x += (-simVel.x * airDrag) * dt;
                simVel.y += (-simVel.y * airDrag * 0.2f) * dt;
                
                simVel.x *= 0.999f;
                simVel.y *= 0.999f;

                CEVec3 nextPos = currentPos + (simVel * dt);

                if (playerScript != null && playerScript.IsGrounded && nextPos.y < playerScript.Entity.GetComponent<TransformComponent>().Position.y - 50.0f)
                    break;

                if (drawOnly)
                {
                    // Only draw every other segment to create a dotted line effect
                    if (i % 2 == 0)
                    {
                        CEVec3 col = new CEVec3(1.0f, 1.0f, 1.0f);
                        Debug.DrawLine(ref currentPos, ref nextPos, 2.0f, ref col); 
                    }
                }
                else
                {
                    activePath.Add(nextPos);
                }

                currentPos = nextPos;
            }
        }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();

            // Universal collision check for path cancellation
            // If we hit ANYTHING static (Ground, Wall, Platform), stop flying.
            if (layer != null && (layer.HasLayer("Ground") || layer.HasLayer("Wall") || layer.HasLayer("Platform")))
            {
                if (isFollowingPath)
                {
                    isFollowingPath = false;
                    activePath.Clear();
                    // Reset velocity to prevent "shooting" through the wall after releasing path control
                    rb.LinearVelocity = new CEVec3(0, 0, 0); 
                }

                if (wasThrown)
                {
                    wasThrown = false;
                    isStunned = true;
                    currentStunTime = stunDuration;
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }
            }

            if (layer != null && layer.HasLayer("Ground"))
            {
                var otherTf = other.GetComponent<TransformComponent>();
                if (otherTf != null && tf.Position.y > otherTf.Position.y + (otherTf.Size.y * 0.4f))
                    isGrounded = true;
                else if (flipCooldown <= 0 && !isChasing && !returningHome)
                {
                    moveRight = !moveRight;
                    flipCooldown = 0.2f;
                }
            }
        }

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();
            if (layer != null && layer.HasLayer("Ground")) isGrounded = false;
        }

        private float Distance(CEVec3 a, CEVec3 b) => (float)Math.Sqrt(Math.Pow(a.x - b.x, 2) + Math.Pow(a.y - b.y, 2));
    }
}