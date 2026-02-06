/*!
@file       PlayerMovement.cs
@author     lee.r@digipen.edu
@date       19/11/2025
@brief      This file contains the PlayerMovement class which handles the player's movement, 
            jumping, state switching (normal/light mass), and interaction logic.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerMovement : ScriptBase
    {
        // --- TUNING VARIABLES ---
        public float maxRunSpeed = 150.0f;
        public float maxAirSpeed = 120.0f;
        public float acceleration = 10.0f;
        public float deceleration = 10.0f;
        public float airAcceleration = 2.0f;
        public float jumpPower = 20000f;
        public float gravityScale = 2.5f;
        public float gravityForceMultiplier = 10.0f;
        public float footstepInterval = 0.075f;
        public float swapPause = 1f;
        public float energyConsumption = 12.5f;

        public int goalNoOfPlates = 1;
        public int currentGoalPlates = 0;

        [SerializeField] private string landingDustName = "LandingDust";
        [SerializeField] private float dustBurstDuration = 0.15f;

        public float normalMass = 7.0f;
        public float lightMass = 4.5f;

        // --- RAYCAST SETTINGS ---
        public float groundCheckDistance = 26.0f;
        public float groundCheckWidth = 10.0f;

        // --- DIALOGUE VARIABLES ---
        [SerializeField] public float dialogueCooldown = 1.0f;
        private Entity[] dialogueEntities;
        private int currentDialogueIndex = 0;
        public bool isDialogueActive = false;
        private bool hasDialogueStarted = false;
        private float dialogueTimer = 0.0f;

        // --- STATE VARIABLES ---
        private float horizontalInput;
        private bool isJumpHeld;
        private bool jumpRequested;
        private bool facingRight = true;
        public bool IsGrounded { get; private set; } = false;
        public bool OnWall { get; private set; } = false;

        public static PlayerMovement Instance;
        public float CurrentEnergy = 0.0f;
        public float MaxEnergy = 100.0f;

        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim;

        private float footstepTimer = 0.0f;
        private bool wasGroundedLastFrame = true;
        private Random rng = new Random();
        private const float FIXED_TIMESTEP = 0.02f;

        public bool alive = true;
        public bool isPaused = false; // Controls the state

        private string currentAnimState = "";
        private bool isLight = false;
        private float swapPauseTimer = 0.0f;

        private uint landingDustID = 0;
        private float landingDustTimer = 0.0f;
        private bool landingDustArmed = false;

        private float jumpCooldown = 0.0f;
        private bool isPassthrough = false;
        private bool canPassthrough = false;

        // --- UI CACHE (New) ---
        private Entity pauseScreenEntity;
        private CEVec3 storedVelocity; // To resume physics later
        
        public void SetPauseState(bool p) { isPaused = p; }

        public override void Start()
        {
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>();

            // --- 1. FIND PAUSE SCREEN ONCE ---
            pauseScreenEntity = Entity.Find("Pause");
            if (pauseScreenEntity != null)
            {
                // Ensure it starts hidden
                InternalCalls.SetActive(pauseScreenEntity.ID, false);
            }
            else
            {
                Console.WriteLine("[PlayerMovement] Warning: 'Pause' entity not found!");
            }

            if (!string.IsNullOrEmpty(landingDustName))
            {
                Entity e = Entity.Find(landingDustName);
                if (e != null)
                {
                    landingDustID = e.ID;
                    InternalCalls.Particle_SetEmitting(landingDustID, false);
                }
            }

            rb.Mass = normalMass;
            IsGrounded = false;
            CurrentEnergy = MaxEnergy;
            swapPauseTimer = 0.0f;

            // Setup Dialogues
            dialogueEntities = new Entity[5];
            for (int i = 0; i < 5; ++i)
            {
                dialogueEntities[i] = Entity.Find("PlayerDialogue_" + (i + 1));
                if (dialogueEntities[i] != null) InternalCalls.SetActive(dialogueEntities[i].ID, false);
            }

            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
        }

        public override void Update()
        {
            //Debug.Log(isPaused ? "PAUSED" : "NOT PAUSED");
            // --- 2. PAUSE INPUT ---
            // Moved Book logic out to keep this clean.
            // Using 'Escape' is standard, but you can keep 'P' if you prefer.
            if (Input.IsKeyTriggered(Key.Escape) || Input.IsKeyTriggered(Key.P))
            {
                if (!isDialogueActive) // Don't pause during cutscenes/dialogue
                {
                    TogglePause();
                }
            }

            // --- 3. PAUSE CHECK ---
            // If paused or dead, do NOTHING else.
            if (isPaused || !alive) return;


            // --- 4. DIALOGUE LOGIC ---
            if (!hasDialogueStarted)
            {
                if (!SceneManager.IsTransitioning())
                {
                    hasDialogueStarted = true;
                    isDialogueActive = true;
                    currentDialogueIndex = 0;
                    dialogueTimer = 0.0f;
                    if (dialogueEntities[0] != null) InternalCalls.SetActive(dialogueEntities[0].ID, true);
                }
            }

            if (isDialogueActive)
            {
                HandleDialogueSequence();
                horizontalInput = 0f;
                UpdateAnimationState(false);
                return;
            }

            // --- 5. DUST LOGIC ---
            if (landingDustTimer > 0.0f)
            {
                landingDustTimer -= (float)Time.DT;
                if (landingDustTimer <= 0.0f && landingDustID != 0)
                {
                    InternalCalls.Particle_SetEmitting(landingDustID, false);
                }
            }

            // --- 6. STATE SWITCH LOGIC ---
            if (swapPauseTimer > 0.0f)
            {
                horizontalInput = 0f;
                UpdateAnimationState(true);
                return;
            }

            // --- 7. INPUT POLLING ---
            horizontalInput = 0f;
            if (Input.IsKeyPressed(Key.A))
            {
                horizontalInput = -1f;
                facingRight = false;
            }
            else if (Input.IsKeyPressed(Key.D))
            {
                horizontalInput = 1f;
                facingRight = true;
            }

            if (!facingRight && tf.Size.x > 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);
            else if (facingRight && tf.Size.x < 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);

            isJumpHeld = Input.IsKeyPressed(Key.Space);

            if (Input.IsKeyTriggered(Key.Space) && IsGrounded && !isPassthrough)
            {
                jumpRequested = true;
            }

            if (Input.IsKeyTriggered(Key.E) && IsGrounded && CurrentEnergy >= energyConsumption)
            {
                RemoveEnergy(energyConsumption);
                isLight = !isLight;
                AudioManager.GetInstance().PlayAttached("switchMassSound", Entity.ID);
                rb.Mass = isLight ? lightMass : normalMass;
                swapPauseTimer = swapPause;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
            }

            HandleFootsteps();
            UpdateAnimationState(false);
        }

        // --- NEW HELPER: CLEAN TOGGLE ---
        public void TogglePause()
        {
            isPaused = !isPaused;
            Debug.Log(isPaused ? "PAUSED" : "NOT PAUSED");

            if (pauseScreenEntity != null)
            {
                InternalCalls.SetActive(pauseScreenEntity.ID, isPaused);
            }

            if (isPaused)
            {
                // STORE velocity so we don't lose momentum, then freeze
                storedVelocity = rb.LinearVelocity;
                rb.LinearVelocity = new CEVec3(0, 0, 0);

                // Optional: Stop animations
                //if (anim != null) anim.Stop();
                anim.Play("playerIdle" + (isLight ? "Light" : ""));
            }
            else
            {
                // RESTORE velocity (optional - if you want them to keep moving after unpause)
                // rb.LinearVelocity = storedVelocity; 

                // Or just keep it 0 if you want them to start fresh
            }
        }

        // ... [Rest of logic: HandleDialogueSequence, FixedUpdate, etc. is fine] ...

        public override void FixedUpdate()
        {
            // CRITICAL: Return early if paused so physics don't run
            if (!alive || isPaused) return;

            // Handle Timers
            if (swapPauseTimer > 0.0f) swapPauseTimer -= FIXED_TIMESTEP;
            if (jumpCooldown > 0.0f) jumpCooldown -= FIXED_TIMESTEP;

            // 1. GROUND CHECK
            bool wasGrounded = IsGrounded;
            CheckGround();

            if (!wasGrounded && IsGrounded && landingDustArmed)
            {
                TriggerLandingDust();
                landingDustArmed = false;
            }

            HandleLanding();

            // 3. JUMP
            if (jumpRequested && !isDialogueActive)
            {
                AudioManager.GetInstance().PlayAttached("JumpUp", Entity.ID);
                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                IsGrounded = false;
                jumpRequested = false;
                jumpCooldown = 0.1f;
                landingDustArmed = true;
            }

            // 4. PHYSICS
            ApplyMovementPhysics();
            ApplyGravity();
        }

        // ... [Paste the rest of your helper functions: CheckGround, ApplyMovementPhysics, etc.] ...
        // (I omitted them to keep the answer short, but they don't need changes)

        // KEEP THESE HELPER FUNCTIONS EXACTLY AS THEY WERE IN YOUR CODE:
        // HandleDialogueSequence, TriggerLandingDust, CheckGround, IsGroundOrPlatform, 
        // ApplyMovementPhysics, ApplyGravity, OnPreSolve, OnCollideEnter/Stay/Exit, 
        // HandleFootsteps, HandleLanding, UpdateAnimationState, HandleDeath, Add/RemoveEnergy

        private void HandleDialogueSequence()
        {
            // Safety check
            if (currentDialogueIndex >= dialogueEntities.Length)
            {
                isDialogueActive = false; // Sequence done
                return;
            }

            Entity currentEnt = dialogueEntities[currentDialogueIndex];

            // If entity is missing, skip it
            if (currentEnt == null)
            {
                currentDialogueIndex++;
                if (currentDialogueIndex < dialogueEntities.Length && dialogueEntities[currentDialogueIndex] != null)
                    InternalCalls.SetActive(dialogueEntities[currentDialogueIndex].ID, true);
                else
                    isDialogueActive = false; // End if we ran out

                return;
            }

            // --- CHECK IF TYPING IS DONE ---
            bool isTypingDone = InternalCalls.IsTextFinished(currentEnt.ID);

            if (isTypingDone)
            {
                // Check if this is the LAST dialogue
                bool isLastDialogue = (currentDialogueIndex == dialogueEntities.Length - 1) || 
                                      (currentDialogueIndex + 1 < dialogueEntities.Length && dialogueEntities[currentDialogueIndex + 1] == null);

                // --- FIX: IMMEDIATE EXIT FOR LAST DIALOGUE ---
                if (isLastDialogue)
                {
                    InternalCalls.SetActive(currentEnt.ID, false); // Hide current
                    isDialogueActive = false; // Return control immediately!
                    return; 
                }

                // Otherwise, wait for cooldown before showing next
                dialogueTimer += (float)Time.DT;

                if (dialogueTimer >= dialogueCooldown)
                {
                    // Cooldown finished, move to next
                    InternalCalls.SetActive(currentEnt.ID, false); // Hide current

                    currentDialogueIndex++;
                    dialogueTimer = 0.0f;

                    if (currentDialogueIndex < dialogueEntities.Length)
                    {
                        if (dialogueEntities[currentDialogueIndex] != null) 
                            InternalCalls.SetActive(dialogueEntities[currentDialogueIndex].ID, true);
                    }
                    else
                    {
                        // All done
                        isDialogueActive = false;
                    }
                }
            }
        }

        private void TriggerLandingDust()
        {
            if (landingDustID == 0 || tf == null) return;
            Entity e = new Entity(landingDustID);
            var dustTf = e.GetComponent<TransformComponent>();
            if (dustTf != null)
            {
                float offsetY = -Math.Abs(tf.Size.y) * 0.5f;
                dustTf.Position = new CEVec3(tf.Position.x, tf.Position.y + offsetY, dustTf.Position.z);
            }
            InternalCalls.Particle_SetEmitting(landingDustID, true);
            landingDustTimer = dustBurstDuration;
        }

        private void CheckGround()
        {
            if (jumpCooldown > 0.0f) { IsGrounded = false; return; }
            float halfHeight = Math.Abs(tf.Size.y) * 0.5f;
            float checkBuffer = 5.0f;
            float xOffset = groundCheckWidth * 0.5f;
            CEVec3 origin = tf.Position;
            float rayStartY = origin.y;
            float rayEndY = origin.y - (halfHeight + checkBuffer);
            CEVec3 startL = new CEVec3(origin.x - xOffset, rayStartY, origin.z);
            CEVec3 endL = new CEVec3(origin.x - xOffset, rayEndY, origin.z);
            CEVec3 startR = new CEVec3(origin.x + xOffset, rayStartY, origin.z);
            CEVec3 endR = new CEVec3(origin.x + xOffset, rayEndY, origin.z);
            uint hitID = 0;
            bool hitLeft = InternalCalls.Raycast(ref startL, ref endL, out hitID, Entity.ID);
            bool leftIsGround = hitLeft && hitID != 0 && IsGroundOrPlatform(hitID);
            bool hitRight = InternalCalls.Raycast(ref startR, ref endR, out hitID, Entity.ID);
            bool rightIsGround = hitRight && hitID != 0 && IsGroundOrPlatform(hitID);
            IsGrounded = (leftIsGround || rightIsGround) && rb.LinearVelocity.y <= 0.1f;
        }

        private bool IsGroundOrPlatform(uint entityID)
        {
            Entity hitEnt = new Entity(entityID);
            var tagComp = hitEnt.GetComponent<TagComponent>();
            return tagComp != null && (tagComp.HasTag("Ground") || tagComp.HasTag("Platform") || tagComp.HasTag("Passthrough"));
        }

        private void ApplyMovementPhysics()
        {
            if (swapPauseTimer > 0.0f)
            {
                CEVec3 v = rb.LinearVelocity;
                rb.AddForce(new CEVec3(-v.x * 100.0f, 0f, 0f));
                return;
            }
            CEVec3 velocity = rb.LinearVelocity;
            float targetSpeed = 0.0f;
            if (horizontalInput != 0f && !OnWall && !isDialogueActive)
            {
                float maxSpeed = IsGrounded ? maxRunSpeed : maxAirSpeed;
                targetSpeed = horizontalInput * maxSpeed;
            }
            float speedDif = targetSpeed - velocity.x;
            float accelRate = (Math.Abs(targetSpeed) > 0.01f) ? (IsGrounded ? acceleration : airAcceleration) : (IsGrounded ? deceleration : 0.5f);
            float movementForce = speedDif * accelRate * rb.Mass;
            rb.AddForce(new CEVec3(movementForce, 0f, 0f));
        }

        private void ApplyGravity()
        {
            if (swapPauseTimer > 0.0f && !IsGrounded)
            {
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
                return;
            }
            if (rb.LinearVelocity.y < 0) rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            else if (rb.LinearVelocity.y > 0 && !isJumpHeld) rb.AddForce(new CEVec3(0f, -9.81f * (gravityScale * 0.5f) * rb.Mass * gravityForceMultiplier, 0f));
        }

        public bool OnPreSolve(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Passthrough"))
            {
                if (canPassthrough) isPassthrough = true;
                if (Input.IsKeyPressed(Key.S) || rb.LinearVelocity.y > 0.1f) { canPassthrough = true; return false; }
                if (isPassthrough) return false;
                canPassthrough = false; return true;
            }
            canPassthrough = false; return true;
        }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Wall")) { OnWall = true; rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0); }
            if (Tag != null && (Tag.HasTag("Lava") || Tag.HasTag("Enemy"))) HandleDeath();
        }

        public void OnCollideStay(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Passthrough") && canPassthrough) isPassthrough = true;
        }

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Wall")) OnWall = false;
            if (Tag != null && Tag.HasTag("Passthrough")) { canPassthrough = false; isPassthrough = false; }
        }

        private void HandleFootsteps()
        {
            if (horizontalInput != 0.0f && IsGrounded && swapPauseTimer <= 0.0f)
            {
                footstepTimer -= (float)Time.DT;
                if (footstepTimer <= 0.0f)
                {
                    int index = rng.Next(1, 9);
                    AudioManager.GetInstance().PlayAttached("footstep" + index, Entity.ID);
                    footstepTimer = footstepInterval;
                }
            }
            else footstepTimer = 0.0f;
        }

        private void HandleLanding()
        {
            if (IsGrounded && !wasGroundedLastFrame) AudioManager.GetInstance().PlayAttached("jumpLand", Entity.ID);
            wasGroundedLastFrame = IsGrounded;
        }

        private void UpdateAnimationState(bool isSwitching)
        {
            if (isDialogueActive)
            {
                string idle = isLight ? "playerIdleLight" : "playerIdle";
                if (currentAnimState != idle) { if (anim != null) anim.Play(idle, true); currentAnimState = idle; }
                return;
            }
            string targetState = "playerIdle";
            if (isSwitching) targetState = "playerSwitch";
            else if (!IsGrounded) targetState = (rb.LinearVelocity.y > 1.0f) ? "playerJump" : ((rb.LinearVelocity.y < -12.5f) ? "playerFall" : targetState);
            else if (Math.Abs(horizontalInput) > 0.01f) targetState = "playerWalk";
            if (isLight) targetState += "Light";
            if (targetState != currentAnimState) { if (anim != null) anim.Play(targetState, true); currentAnimState = targetState; }
        }

        private void HandleDeath()
        {
            Debug.Log("Player Died");
            alive = false;
            InternalCalls.SetActive(InternalCalls.FindEntityByName("GameOverScreen"), true);
        }

        public void AddEnergy(float amount) { CurrentEnergy = Math.Min(CurrentEnergy + amount, MaxEnergy); }
        public void RemoveEnergy(float amount) { CurrentEnergy = Math.Max(CurrentEnergy - amount, 0.0f); }
    }
}