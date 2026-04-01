/*!
@file       SlimeMovement.cs
@author     Shahrin Idzuan
@author     Renfred (RTIS)
@author     mohamadamirakmal.b
@date       27/11/2025
@brief      Handles the AI movement, state machine, and pathfinding logic for Slime enemies.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Collections.Generic;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SlimeMovement : ScriptBase
    {
        // --- SETTINGS ---
        [SerializeField] private float moveSpeed = 600f; 
        [SerializeField] private float maxSpeed = 15.0f; 
        [SerializeField] private float moveAudioInterval = 0.4f;
        //[SerializeField] private float stopAudioDistance = 40.0f;
        [SerializeField] private float MaxVolumeMinDistance = 50.0f;
        [SerializeField] private float MinVolumeMaxDistance = 250.0f;
        [SerializeField] private float detectionRange = 250.0f;
        [SerializeField] private float gravityScale = 2.0f;
        [SerializeField] private float grabRange = 100.0f;
        [SerializeField] private float lineOfSightYTolerance = 25.0f;

        // THROW SETTINGS
        [SerializeField] private float minThrowSpeed = 300.0f;
        [SerializeField] private float maxThrowSpeed = 2000.0f; 
        [SerializeField] private float maxMouseDragDist = 400.0f;
        [SerializeField] private float controllerAimDeadzone = 0.2f;
        [SerializeField] private float airDrag = 1.0f; 
        [SerializeField] private float stunDuration = 1.0f;
        [SerializeField] private float pickAnimDuration = 60.0f;

        // Patrol Zone
        [SerializeField] private float ledgeBuffer = 40.0f; 
        [SerializeField] private float ledgeRayCastDistance = 50.0f;
        [SerializeField] private float grabEnergyDrainRate = 5.0f;
        [SerializeField] private float patrolTurnInterval = 3.0f;
        [SerializeField] private bool resizePlayerColliderWhileHolding = true;
        [SerializeField] private float carryBoundsPadding = 10.0f;
        [SerializeField] private float heldRumbleLow = 0.08f;
        [SerializeField] private float heldRumbleHigh = 0.14f;
        [SerializeField] private float heldRumbleDuration = 0.12f;
        [SerializeField] private float heldRumbleRefreshInterval = 0.08f;
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim;
        private ColliderComponent col;
        private PlayerMovement playerScript;
        private ColliderComponent playerCol;
        private bool holdCollidersDisabled = false;
        private CEVec3 playerAabbLocalMin;
        private CEVec3 playerAabbLocalMax;
        private bool playerAabbCached = false;
        private bool playerAabbResized = false;

        private bool isGrounded = false;
        private bool wasGroundedLastFrame = true;
        private Random rng = new Random();
        private bool moveRight = true;
        private float moveAudioTimer = 0.0f;
        private bool isChasing = false;
        private bool isGrabbed = false;
        private bool wasThrown = false;
        private bool isStunned = false;
        private float currentStunTime = 0.0f;
        private bool isPicking = false;
        private float pickAnimTimer = 0.0f;
        private CEVec3 pickStartPos;
        private bool pickStartPosValid = false;
        private float flipCooldown = 0.0f;
        private float ignoreHomeTimer = 0.0f;
        private float patrolTurnTimer = 0.0f;
        [SerializeField] private float ignorePlayerStopDuration = 0.2f;
        private float ignorePlayerStopTimer = 0.0f;
        private float heldRumbleTimer = 0.0f;

        private CEVec3 startPos;
        private bool returningHome = false;

        private List<CEVec3> activePath = new List<CEVec3>();
        private bool isFollowingPath = false;

        bool STOP = false;
        private bool stopApplied = false;
        private string currentAnimState = "";

        private bool HasLineOfSight(CEVec3 targetPos)
        {
            if (Math.Abs(targetPos.y - tf.Position.y) > lineOfSightYTolerance)
                return false;

            CEVec3 start = new CEVec3(tf.Position.x, targetPos.y, tf.Position.z);
            CEVec3 end = new CEVec3(targetPos.x, targetPos.y, targetPos.z);
            uint hitID;
            
            if (InternalCalls.Raycast(ref start, ref end, out hitID, Entity.ID))
            {
                Entity hitEnt = new Entity(hitID);
                if (hitEnt.ID == playerScript.Entity.ID) return true; 

                var tag = hitEnt.GetComponent<TagComponent>();
                if (tag != null && (tag.HasTag("Ground") || tag.HasTag("Wall") || tag.HasTag("Platform") || tag.HasTag("Tilemap")))
                {
                    return false; 
                }
            }
            return true; 
        }

        private bool IsNearLedge()
        {
            float checkDir = moveRight ? 1.0f : -1.0f;
            float halfWidth = Math.Abs(tf.Size.x) * 0.5f;
            float halfHeight = Math.Abs(tf.Size.y) * 0.5f;
            float rayOriginX = tf.Position.x + (checkDir * (halfWidth + ledgeBuffer));
            
            CEVec3 rayStart = new CEVec3(rayOriginX, tf.Position.y - (halfHeight * 0.5f), tf.Position.z);
            CEVec3 rayEnd = new CEVec3(rayStart.x, rayStart.y - ledgeRayCastDistance, rayStart.z);

            uint hitID;
            if (InternalCalls.Raycast(ref rayStart, ref rayEnd, out hitID, Entity.ID))
            {
                Entity hitEnt = new Entity(hitID);
                var tag = hitEnt.GetComponent<TagComponent>();
                if (tag != null && (tag.HasTag("Ground") || tag.HasTag("Platform")))
                {
                    return false; 
                }
            }
            return true; 
        }

        private void CheckGrounded()
        {
            float halfHeight = Math.Abs(tf.Size.y) * 0.5f;
            float halfWidth = Math.Abs(tf.Size.x) * 0.5f;
            float checkDist = 15.0f; 

            CEVec3 startLeft = new CEVec3(tf.Position.x - (halfWidth * 0.5f), tf.Position.y, tf.Position.z);
            CEVec3 endLeft = new CEVec3(startLeft.x, startLeft.y - (halfHeight + checkDist), startLeft.z);

            CEVec3 startRight = new CEVec3(tf.Position.x + (halfWidth * 0.5f), tf.Position.y, tf.Position.z);
            CEVec3 endRight = new CEVec3(startRight.x, startRight.y - (halfHeight + checkDist), startRight.z);

            uint hitID;
            bool hitLeft = InternalCalls.Raycast(ref startLeft, ref endLeft, out hitID, Entity.ID);
            bool isLeftGrounded = false;
            if (hitLeft) {
                 Entity hitEnt = new Entity(hitID);
                 var Tag = hitEnt.GetComponent<TagComponent>();
                 if (Tag != null && (Tag.HasTag("Ground") || Tag.HasTag("Platform") || Tag.HasTag("Wall"))) isLeftGrounded = true;
            }

            bool hitRight = InternalCalls.Raycast(ref startRight, ref endRight, out hitID, Entity.ID);
            bool isRightGrounded = false;
            if (hitRight) {
                 Entity hitEnt = new Entity(hitID);
                 var Tag = hitEnt.GetComponent<TagComponent>();
                 if (Tag != null && (Tag.HasTag("Ground") || Tag.HasTag("Platform") || Tag.HasTag("Wall"))) isRightGrounded = true;
            }
            
            isGrounded = isLeftGrounded || isRightGrounded;
        }

        public override void Start()
        {
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>(); 
            col = Entity.GetComponent<ColliderComponent>();
            playerScript = PlayerMovement.Instance;
            CachePlayerCollider();

            // Set 3D Distances for all slime sounds to handle distance-based attenuation
            for (int i = 1; i <= 5; i++)
            {
                AudioManager.SetClip3DDistances(Entity.ID, "slimeMove" + i, MaxVolumeMinDistance, MinVolumeMaxDistance);
            }
            AudioManager.SetClip3DDistances(Entity.ID, "slimeImpact", MaxVolumeMinDistance, MinVolumeMaxDistance);

            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);

            if (rb != null) rb.LinearVelocity = new CEVec3(0, 0, 0);
            if (tf != null) startPos = tf.Position;

            Console.WriteLine("[Slime] AI Initialized. Home X: " + startPos.x);
        }

        // A large distance to ensure the slime resets if it gets knocked too far away or stuck.
        [SerializeField] private float maxLeashDistance = 3000.0f;

        public override void Update()
        {
            float distanceFromStart = Distance(tf.Position, startPos);

            // --- OUT OF BOUNDS / LEASH CHECK ---
            // If the slime is too far from its spawn OR has fallen through the world, reset it.
            if (distanceFromStart > maxLeashDistance || tf.Position.y < -5000.0f)
            {
                // Reset position and velocity
                tf.Position = startPos;
                if (rb != null)
                {
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }

                // Reset all state flags to be safe
                isGrabbed = false;
                wasThrown = false;
                isStunned = false;
                isPicking = false;
                pickAnimTimer = 0.0f;
                pickStartPosValid = false;
                isChasing = false;
                returningHome = false;
                STOP = false;
                UpdatePlayerAabb(false, playerScript != null ? playerScript.Entity.GetComponent<TransformComponent>() : null);
                UpdateHoldColliders(false);

                // Ensure it's a dynamic rigidbody and has the Enemy tag
                if(rb != null) rb.SetDynamic(true);
                var tag = Entity.GetComponent<TagComponent>();
                if (tag != null) tag.AddTag("Enemy");

                Console.WriteLine($"[Slime] Leash broken or out of bounds (Distance: {distanceFromStart}), resetting.");
                return; // Skip the rest of this frame's update
            }

            // BUG FIX: Always grab the current player instance to prevent stale references across scene loads
            playerScript = PlayerMovement.Instance;
            CachePlayerCollider();
            
            if (playerScript != null && playerScript.isPaused) { return; } 
            if (rb == null || tf == null || playerScript == null) return;

            if (playerScript != null)
            {
                HandleGrabInput();

                if (!isGrabbed)
                {
                    heldRumbleTimer = 0.0f;
                    UpdatePlayerAabb(false, playerScript != null ? playerScript.Entity.GetComponent<TransformComponent>() : null);
                    UpdateHoldColliders(false);
                }
            }

            if (STOP)
            {
                if (rb != null)
                {
                    if (!stopApplied) rb.SetDynamic(false);
                    rb.LinearVelocity = new CEVec3(0.0f, 0.0f, 0.0f);
                    stopApplied = true;
                }
                return;
            }

            float dt = (float)Time.DT;

            if (isPicking)
            {
                pickAnimTimer -= dt;
                if (pickAnimTimer <= 0.0f || !isGrabbed)
                {
                    isPicking = false;
                    pickAnimTimer = 0.0f;
                }
            }

            if (isGrabbed)
            {
                UpdateHeldRumble(dt);
                ClearStopState();
                UpdateAnimationState();
                return;
            }

            if (isStunned)
            {
                currentStunTime -= dt;
                if (currentStunTime <= 0)
                {
                    isStunned = false;
                    ClearStopState(); // Clear STOP so AI/physics resumes after stun
                    var tag = Entity.GetComponent<TagComponent>();
                    if (tag != null) tag.AddTag("Enemy");
                }
                
                // FEATURE FIX: Force facing right while stunned
                moveRight = true;
                
                UpdateAnimationState();
            }

            if (flipCooldown > 0) flipCooldown -= dt;
            if (ignorePlayerStopTimer > 0) ignorePlayerStopTimer -= dt;

            float horizontalSpeed = Math.Abs(rb.LinearVelocity.x);

            if (horizontalSpeed > 1.0f && isGrounded && !isStunned)
            {
                moveAudioTimer -= dt;
                if (moveAudioTimer <= 0.0f)
                {
                    int index = rng.Next(1, 6);
                    AudioManager.GetInstance().PlayAttached("slimeMove" + index, Entity.ID);

                    float speedFactor = Math.Min(Math.Max(horizontalSpeed / maxSpeed, 0.5f), 2.0f);
                    moveAudioTimer = moveAudioInterval / speedFactor;
                }
            }
            else
            {
                moveAudioTimer = 0.0f;
            }

            // Sprite flip visual update
            float absX = Math.Abs(tf.Size.x);
            if (moveRight) tf.Size = new CEVec2(absX, tf.Size.y);
            else tf.Size = new CEVec2(-absX, tf.Size.y);

            UpdateAnimationState();
        }

        public override void OnDrawGizmos()
        {
            if (tf == null) tf = Entity.GetComponent<TransformComponent>();
            if (tf == null) return;

            float drawZ = 51.0f;

            // --- Leash Range Visualization ---
            // Determines the center of the circle based on the game state.
            CEVec3 leashCenter;
            // If startPos is default, game is stopped; use the current transform for editor preview.
            if (startPos.x == 0.0f && startPos.y == 0.0f && startPos.z == 0.0f)
            {
                leashCenter = new CEVec3(tf.Position.x, tf.Position.y, drawZ);
            }
            else // Otherwise, the game is playing; use the actual startPos anchor.
            {
                leashCenter = new CEVec3(startPos.x, startPos.y, drawZ);
            }
            CEVec3 leashColor = new CEVec3(1.0f, 0.0f, 0.0f); // Red
            CEVec2 leashSize = new CEVec2(maxLeashDistance * 2.0f, maxLeashDistance * 2.0f);
            Debug.DrawCircle(ref leashSize, ref leashCenter, ref leashColor);


            // --- Detection Range Visualization ---
            CEVec3 detectionColor = new CEVec3(1.0f, 1.0f, 0.0f);
            CEVec2 detectionSize = new CEVec2(detectionRange * 2.0f, detectionRange * 2.0f);
            CEVec3 myPos = new CEVec3(tf.Position.x, tf.Position.y, drawZ);
            Debug.DrawCircle(ref detectionSize, ref myPos, ref detectionColor);

            CEVec3 grabColor = new CEVec3(0.0f, 1.0f, 0.0f); 
            CEVec2 grabSize = new CEVec2(grabRange * 2.0f, grabRange * 2.0f);
            Debug.DrawCircle(ref grabSize, ref myPos, ref grabColor);

            if (Debug.IsGameDebugDrawEnabled())
            {
                float checkDir = moveRight ? 1.0f : -1.0f;
                float halfWidth = Math.Abs(tf.Size.x) * 0.5f;
                float halfHeight = Math.Abs(tf.Size.y) * 0.5f;
                float rayOriginX = tf.Position.x + (checkDir * (halfWidth + ledgeBuffer));
                
                CEVec3 rayStart = new CEVec3(rayOriginX, tf.Position.y - (halfHeight * 0.5f), drawZ);
                CEVec3 rayEnd = new CEVec3(rayStart.x, rayStart.y - ledgeRayCastDistance, drawZ);
                
                bool nearLedge = IsNearLedge();
                CEVec3 rayColor = nearLedge ? new CEVec3(1.0f, 0.0f, 0.0f) : new CEVec3(0.0f, 1.0f, 1.0f); 
                Debug.DrawLine(ref rayStart, ref rayEnd, 6.0f, ref rayColor);
            }

            if (isGrabbed)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();

                if (playerTf != null)
                {
                    CEVec2 mousePos = GetThrowAimPosition(tf.Position);
                    CEVec3 slimePos = new CEVec3(tf.Position.x, tf.Position.y, drawZ);
                    CEVec3 throwVel = CalculateThrowVelocity(mousePos, tf.Position);
                    SimulatePath(throwVel, slimePos);
                }

                if (activePath.Count > 1)
                {
                    CEVec3 pathColor = new CEVec3(1.0f, 1.0f, 1.0f); 
                    for (int i = 0; i < activePath.Count - 2; i += 4)
                    {
                        CEVec3 p1 = activePath[i];
                        CEVec3 p2 = activePath[i + 1];
                        p1.z = drawZ; p2.z = drawZ;
                        Debug.DrawTrajectory(ref p1, ref p2, 2.0f, ref pathColor);
                    }
                }
            }
        }

        private void UpdateAnimationState()
        {
            if (anim == null) return;

            string targetState = "slimeIdle";

            if (isGrabbed) targetState = "slimePick";
            else if (isStunned) targetState = "slimeStunned";
            else if (Math.Abs(rb.LinearVelocity.x) > 1.0f)
            {
                if (isChasing) targetState = "slimeChase";
                else targetState = "slimeWalk";
            }

            if (targetState != currentAnimState)
            {
                anim.Play(targetState, true);
                currentAnimState = targetState;
            }
        }

        public override void FixedUpdate()
        {
            if (STOP) return;

            if (isGrabbed && playerScript != null)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                    float offsetX = playerTf.Size.x < 0 ? -30.0f : 30.0f;
                    CEVec3 targetPos = new CEVec3(playerTf.Position.x + offsetX, playerTf.Position.y - 5.0f, playerTf.Position.z + 1f);

                    if (isPicking && pickStartPosValid)
                    {
                        float total = Math.Max(pickAnimDuration, 0.0001f);
                        float progress = 1.0f - (pickAnimTimer / total);
                        progress = Math.Min(1.0f, Math.Max(0.0f, progress));

                        tf.Position = new CEVec3(
                            pickStartPos.x + (targetPos.x - pickStartPos.x) * progress,
                            pickStartPos.y + (targetPos.y - pickStartPos.y) * progress,
                            pickStartPos.z + (targetPos.z - pickStartPos.z) * progress
                        );
                    }
                    else
                    {
                        tf.Position = targetPos;
                    }

                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                    UpdatePlayerAabb(true, playerTf);
                    UpdateHoldColliders(true);
                }
                return; 
            }
            
            if (rb == null || isStunned) return; 
            if (playerScript != null && playerScript.isPaused) return;

            CheckGrounded();
            float dt = (float)Time.FixedDT;

            if (isGrounded != wasGroundedLastFrame)
            {
                if (!isGrounded && rb.LinearVelocity.y > 5.0f)
                    AudioManager.GetInstance().PlayAttached("slimeMove" + rng.Next(1, 6), Entity.ID);
                else if (isGrounded)
                    AudioManager.GetInstance().PlayAttached("slimeImpact", Entity.ID);
            }

            if (flipCooldown > 0) flipCooldown -= dt;
            if (ignoreHomeTimer > 0) ignoreHomeTimer -= dt;

            if (!wasThrown && !isGrabbed && !isStunned && playerScript != null && playerScript.alive)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                    float distToPlayer = Distance(tf.Position, playerTf.Position);

                    if (distToPlayer < detectionRange && HasLineOfSight(playerTf.Position))
                    {
                        isChasing = true;
                        returningHome = false;
                        if (playerTf.Position.x > tf.Position.x) moveRight = true;
                        else moveRight = false;
                    }
                    else
                    {
                        isChasing = false;
                        float distToHome = Distance(tf.Position, startPos);
                        if (distToHome > 100.0f && ignoreHomeTimer <= 0)
                        {
                            returningHome = true;
                            if (startPos.x > tf.Position.x) moveRight = true;
                            else moveRight = false;
                        }
                        else
                        {
                            returningHome = false;
                            if (isGrounded && flipCooldown <= 0)
                            {
                                patrolTurnTimer -= dt;
                                if (IsNearLedge() || patrolTurnTimer <= 0)
                                {
                                    moveRight = !moveRight;
                                    patrolTurnTimer = patrolTurnInterval;
                                    flipCooldown = 0.5f;
                                }
                            }
                        }
                    }
                }
            }

            if (!wasThrown && (playerScript == null || playerScript.alive))
            {
                CEVec3 velocity = rb.LinearVelocity;
                float dir = moveRight ? 1.0f : -1.0f;
                float speedMult = (isChasing || returningHome) ? 1.2f : 1.0f;

                if (isGrounded || isChasing)
                {
                    if (Math.Abs(velocity.x) < maxSpeed * speedMult)
                    {
                        rb.AddForce(new CEVec3(moveSpeed * dir * speedMult, 0, 0));
                    }
                    rb.AddForce(new CEVec3(-velocity.x * 5.0f * rb.Mass, 0, 0));
                }
                else
                {
                    rb.AddForce(new CEVec3(-velocity.x * airDrag * rb.Mass, 0, 0));
                }
            }
            else
            {
                CEVec3 velocity = rb.LinearVelocity;
                if (isGrounded) rb.AddForce(new CEVec3(-velocity.x * 2.0f * rb.Mass, 0, 0));
            }

            float gravityForce = -9.81f * 100.0f * gravityScale * rb.Mass;
            rb.AddForce(new CEVec3(0f, gravityForce, 0f));

            wasGroundedLastFrame = isGrounded;
        }

        public void RemoveEnergy(float amount)
        {
            PlayerMovement.Instance.CurrentEnergy = Math.Max(PlayerMovement.Instance.CurrentEnergy - amount, 0.0f);
        }

        private bool TryGetControllerAimWorldPosition(CEVec3 anchorPos, float maxDistance, out CEVec2 aimWorldPos)
        {
            float aimX = Input.GetAxis("AimX");
            float aimY = Input.GetAxis("AimY");
            float magnitude = (float)Math.Sqrt((aimX * aimX) + (aimY * aimY));

            if (magnitude <= controllerAimDeadzone)
            {
                aimWorldPos = new CEVec2(anchorPos.x, anchorPos.y);
                return false;
            }

            float normalizedX = aimX / magnitude;
            float normalizedY = aimY / magnitude;
            float clampedMagnitude = Math.Min(magnitude, 1.0f);
            float aimDistance = maxDistance * clampedMagnitude;

            aimWorldPos = new CEVec2(
                anchorPos.x + (normalizedX * aimDistance),
                anchorPos.y + (normalizedY * aimDistance));

            return true;
        }

        private CEVec2 GetGrabAimPosition(CEVec3 slimeWorldPos)
        {
            var playerTf = playerScript != null ? playerScript.Entity.GetComponent<TransformComponent>() : null;
            CEVec3 anchorPos = playerTf != null ? playerTf.WorldPosition : slimeWorldPos;

            if (TryGetControllerAimWorldPosition(anchorPos, grabRange + 100.0f, out CEVec2 controllerAimPos))
            {
                return controllerAimPos;
            }

            return Input.GetMouseWorldPosition();
        }

        private CEVec2 GetThrowAimPosition(CEVec3 anchorPos)
        {
            if (TryGetControllerAimWorldPosition(anchorPos, maxMouseDragDist, out CEVec2 controllerAimPos))
            {
                return controllerAimPos;
            }

            return Input.GetMouseWorldPosition();
        }

        /*!
        @brief Handle mouse/controller slime grab and release input.

        Mouse grab still requires aiming over the slime, but controller grab can
        acquire any slime already inside grab range without right-stick aiming.
        */
        private void HandleGrabInput()
        {
            bool mouseGrabHeld = Input.IsMouseButtonDown(0);
            bool controllerGrabHeld = Input.GetAction("GrabSlime");
            bool isDown = mouseGrabHeld || controllerGrabHeld;
            CEVec3 worldPos = tf.WorldPosition;
            CEVec2 aimPos = isGrabbed ? GetThrowAimPosition(worldPos) : GetGrabAimPosition(worldPos);

            if (isDown)
            {
                if (isGrabbed && playerScript != null && playerScript.CurrentEnergy <= 0.0f)
                {
                    ReleaseSlime(false, aimPos);
                    return;
                }

                if (!isGrabbed)
                {
                    float halfW = Math.Abs(tf.Size.x) * 0.5f + 100.0f;
                    float halfH = Math.Abs(tf.Size.y) * 0.5f + 100.0f;

                    bool aimOverSlime = controllerGrabHeld ||
                                        (aimPos.x >= worldPos.x - halfW && aimPos.x <= worldPos.x + halfW &&
                                         aimPos.y >= worldPos.y - halfH && aimPos.y <= worldPos.y + halfH);

                    if (aimOverSlime)
                    {
                        var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                        float distToPlayer = Distance(worldPos, playerTf.WorldPosition);
                        
                        if (distToPlayer <= grabRange + 50.0f && playerScript.CurrentEnergy > 0) {
                            isGrabbed = true;
                            isPicking = true;
                            pickAnimTimer = pickAnimDuration;
                            pickStartPos = tf.Position;
                            pickStartPosValid = true;
                            isFollowingPath = false;
                            activePath.Clear();
                            rb.SetDynamic(false);
                            var Tag = Entity.GetComponent<TagComponent>();
                            if (Tag != null) Tag.RemoveTag("Enemy");
                            if (anim != null)
                            {
                                anim.Play("slimePick", true);
                                currentAnimState = "slimePick";
                            }
                            Console.WriteLine("[Slime] Grabbed!");
                        }
                    }
                }

                if (isGrabbed && playerScript != null)
                {
                    RemoveEnergy(grabEnergyDrainRate * (float)Time.DT);
                    if (playerScript.CurrentEnergy <= 0.0f)
                    {
                        ReleaseSlime(false, aimPos);
                        return;
                    }
                    var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                    if (playerTf != null)
                    {
                        CEVec3 throwVel = CalculateThrowVelocity(aimPos, worldPos);
                        SimulatePath(throwVel, worldPos);
                    }
                }
            }
            else if (isGrabbed)
            {
                ReleaseSlime(true, aimPos);
            }
        }

        private void ReleaseSlime(bool applyThrow, CEVec2 mousePos)
        {
            isGrabbed = false;
            heldRumbleTimer = 0.0f;
            wasThrown = applyThrow;
            isPicking = false;
            pickAnimTimer = 0.0f;
            pickStartPosValid = false;
            UpdatePlayerAabb(false, playerScript != null ? playerScript.Entity.GetComponent<TransformComponent>() : null);
            UpdateHoldColliders(false);

            // BUG FIX 1: Ensure STOP is false so physics immediately applies
            ClearStopState();

            activePath.Clear();
            rb.SetDynamic(true);
            var Tag = Entity.GetComponent<TagComponent>();
            if (Tag == null)
            {
                Entity.Add<TagComponent>();
                Tag = Entity.GetComponent<TagComponent>();
            }
            // Removed AddTag("Enemy") so it doesn't damage the player while being thrown
            Console.WriteLine(applyThrow ? "[Slime] Released/Thrown!" : "[Slime] Released (Energy Depleted)!");

            if (applyThrow && playerScript != null)
            {
                CEVec3 throwVel = CalculateThrowVelocity(mousePos, tf.Position);
                rb.LinearVelocity = throwVel;
            }
            else
            {
                rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
                ignorePlayerStopTimer = ignorePlayerStopDuration;
                if (Tag != null) Tag.AddTag("Enemy");
            }
        }

        private void UpdateHoldColliders(bool shouldDisable)
        {
            if (shouldDisable == holdCollidersDisabled) return;

            if (col == null) col = Entity.GetComponent<ColliderComponent>();
            if (col != null) col.Enabled = !shouldDisable;

            holdCollidersDisabled = shouldDisable;
        }

        private void CachePlayerCollider()
        {
            if (playerScript == null) return;
            if (playerCol == null) playerCol = playerScript.Entity.GetComponent<ColliderComponent>();

            if (!playerAabbCached && playerCol != null)
            {
                var playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                    playerCol.GetAABB(out CEVec3 min, out CEVec3 max);
                    playerAabbLocalMin = min - playerTf.Position;
                    playerAabbLocalMax = max - playerTf.Position;
                    playerAabbCached = true;
                }
            }
        }

        private void UpdatePlayerAabb(bool shouldResize, TransformComponent playerTf)
        {
            if (!resizePlayerColliderWhileHolding || playerTf == null) return;

            if (playerCol == null) playerCol = playerScript.Entity.GetComponent<ColliderComponent>();
            if (playerCol == null) return;

            if (!playerAabbCached)
            {
                playerCol.GetAABB(out CEVec3 min, out CEVec3 max);
                playerAabbLocalMin = min - playerTf.Position;
                playerAabbLocalMax = max - playerTf.Position;
                playerAabbCached = true;
            }

            if (!shouldResize)
            {
                if (playerAabbResized)
                {
                    playerCol.SetAABB(playerAabbLocalMin, playerAabbLocalMax);
                    playerAabbResized = false;
                }
                return;
            }

            CEVec3 playerPos = playerTf.Position;
            CEVec3 playerWorldMin = playerAabbLocalMin + playerPos;
            CEVec3 playerWorldMax = playerAabbLocalMax + playerPos;

            if (col == null) col = Entity.GetComponent<ColliderComponent>();
            CEVec3 slimeWorldMin = tf.Position;
            CEVec3 slimeWorldMax = tf.Position;
            if (col != null)
            {
                col.GetAABB(out CEVec3 sMin, out CEVec3 sMax);
                slimeWorldMin = sMin;
                slimeWorldMax = sMax;
            }

            float minX = Math.Min(playerWorldMin.x, slimeWorldMin.x) - carryBoundsPadding;
            float maxX = Math.Max(playerWorldMax.x, slimeWorldMax.x) + carryBoundsPadding;
            float minY = playerWorldMin.y;
            float maxY = Math.Max(playerWorldMax.y, slimeWorldMax.y) + carryBoundsPadding;

            CEVec3 localMin = new CEVec3(minX - playerPos.x, minY - playerPos.y, 0.0f);
            CEVec3 localMax = new CEVec3(maxX - playerPos.x, maxY - playerPos.y, 0.0f);
            playerCol.SetAABB(localMin, localMax);
            playerAabbResized = true;
        }

        private CEVec3 CalculateThrowVelocity(CEVec2 mousePos, CEVec3 anchorPos)
        {
            float dx = anchorPos.x - mousePos.x;
            float dy = anchorPos.y - mousePos.y;
            float dist = (float)Math.Sqrt(dx * dx + dy * dy);
            
            float dirX = 0f, dirY = 0f;
            if (dist > 0.001f) { dirX = dx / dist; dirY = dy / dist; }

            dist = Math.Min(dist, maxMouseDragDist);
            float speed = minThrowSpeed + (maxThrowSpeed - minThrowSpeed) * (dist / maxMouseDragDist);

            return new CEVec3(dirX * speed, dirY * speed, 0);
        }

        private void SimulatePath(CEVec3 velocity, CEVec3 startPos)
        {
            activePath.Clear();

            CEVec3 currentPos = startPos;
            float dt = (float)Time.FixedDT;
            if (dt <= 0.0001f) dt = 0.0166f; 

            int steps = (int)(2.0f / dt);
            CEVec3 simVel = velocity;
            float totalAccelY = (-9.81f * 100.0f * gravityScale); 

            for (int i = 0; i < steps; i++)
            {
                simVel.y += totalAccelY * dt;
                simVel.x += (-simVel.x * airDrag) * dt;
                simVel.y += (-simVel.y * airDrag * 0.2f) * dt;
                CEVec3 nextPos = currentPos + (simVel * dt);

                uint hitID;
                if (InternalCalls.Raycast(ref currentPos, ref nextPos, out hitID, Entity.ID))
                {
                    Entity hitEnt = new Entity(hitID);
                    var hitTag = hitEnt.GetComponent<TagComponent>();

                    // Ignore the player so the preview does not terminate immediately while the
                    // slime is being held inside the player's merged carry collider.
                    if (hitTag == null || !hitTag.HasTag("Player"))
                    {
                        activePath.Add(currentPos);
                        break;
                    }
                }

                if (playerScript != null && nextPos.y < playerScript.Entity.GetComponent<TransformComponent>().Position.y - 200.0f)
                    break;

                activePath.Add(nextPos);
                currentPos = nextPos;
            }
        }

        private void ClearStopState()
        {
            if (!STOP) return;
            STOP = false;
            stopApplied = false;
            if (rb != null) rb.SetDynamic(true);
        }

        /*!
        @brief Refresh a low continuous controller rumble while the slime is held.
        @param dt Delta time for the current frame.

        The engine rumble call is duration-based, so this method periodically
        reapplies a short pulse while `isGrabbed` remains true.
        */
        private void UpdateHeldRumble(float dt)
        {
            if (heldRumbleRefreshInterval <= 0.0f)
            {
                Input.SetControllerVibration(heldRumbleLow, heldRumbleHigh, heldRumbleDuration);
                return;
            }

            heldRumbleTimer -= dt;
            if (heldRumbleTimer > 0.0f)
            {
                return;
            }

            Input.SetControllerVibration(heldRumbleLow, heldRumbleHigh, heldRumbleDuration);
            heldRumbleTimer = heldRumbleRefreshInterval;
        }

        public bool OnPreSolve(uint otherEntity)
        {
            if (isGrabbed) return false;

            // When thrown, ignore collisions ONLY with the player so it can pass through.
            if (wasThrown)
            {
                Entity other = new Entity(otherEntity);
                var tag = other.GetComponent<TagComponent>();
                if (tag != null && tag.HasTag("Player"))
                {
                    return false; // Don't collide with the player while flying through the air.
                }
            }

            return true; // Collide with everything else normally.
        }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag == null) return;

            // BUG FIX 2: Prevent instant stop race-condition when thrown next to player
            if (Tag.HasTag("Player")) 
            {
                if (!isGrabbed && !wasThrown && !isStunned && !isChasing && !returningHome && ignorePlayerStopTimer <= 0.0f) 
                {
                    STOP = true;
                }
            }

            if (Tag != null && (Tag.HasTag("Ground") || Tag.HasTag("Wall") || Tag.HasTag("Platform") || Tag.HasTag("Tilemap")))
            {
                if (isFollowingPath)
                {
                    isFollowingPath = false;
                    activePath.Clear();
                    rb.LinearVelocity = new CEVec3(0, 0, 0); 
                }

                if (wasThrown)
                {
                    AudioManager.GetInstance().PlayAttached("slimeImpact", Entity.ID);
                    wasThrown = false;
                    isStunned = true;
                    currentStunTime = stunDuration;
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }
                
                var otherTf = other.GetComponent<TransformComponent>();
                bool isOnTop = otherTf != null && tf.Position.y > otherTf.Position.y + (otherTf.Size.y * 0.4f);

                if (!isOnTop && flipCooldown <= 0 && !isChasing && !returningHome)
                {
                    moveRight = !moveRight;
                    float newDir = moveRight ? 1.0f : -1.0f;
                    rb.LinearVelocity = new CEVec3(newDir * 50.0f, rb.LinearVelocity.y, 0); 
                    flipCooldown = 0.2f;
                    ignoreHomeTimer = 4.0f; 
                }
            }
        }

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Player"))
            {
                ClearStopState();
            }
        }

        // PERFORMANCE MATH FIX: Removed slow Math.Pow implementation
        private float Distance(CEVec3 a, CEVec3 b) 
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return (float)Math.Sqrt((dx * dx) + (dy * dy));
        }
    }
}
