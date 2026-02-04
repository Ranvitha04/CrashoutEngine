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
        public float goalNoOfPlates = 1;
        public int currentGoalPlates = 0;

        public float normalMass = 7.0f;
        public float lightMass = 4.5f;

        // --- RAYCAST SETTINGS ---
        public float groundCheckDistance = 26.0f;
        public float groundCheckWidth = 10.0f;

        // --- DIALOGUE VARIABLES ---
        [SerializeField] public float dialogueCooldown = 1.0f; // Time to wait between dialogues
        private Entity[] dialogueEntities;
        private int currentDialogueIndex = 0;
        private bool isDialogueActive = false;
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
        public bool isPaused = false;

        private string currentAnimState = "";
        private bool isLight = false;
        private float swapPauseTimer = 0.0f;

        private float jumpCooldown = 0.0f;
        private bool isPassthrough = false;
        private bool canPassthrough = false;

        public override void Start()
        {
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>();

            rb.Mass = normalMass;
            IsGrounded = false;

            CurrentEnergy = MaxEnergy;
            swapPauseTimer = 0.0f;

            // --- 1. SETUP DIALOGUES ---
            dialogueEntities = new Entity[5];
            for (int i = 0; i < 5; ++i)
            {
                // Find "PlayerDialogue_1", "PlayerDialogue_2", etc.
                dialogueEntities[i] = Entity.Find("PlayerDialogue_" + (i + 1));

                // Ensure they are hidden at start using InternalCalls (Safe method)
                if (dialogueEntities[i] != null)
                {
                    InternalCalls.SetActive(dialogueEntities[i].ID, false);
                }
            }

            // Register Callbacks
            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
        }

        public override void Update()
        {
            if (isPaused || !alive) return;

            // --- 2. DIALOGUE GATEKEEPER ---
            if (!hasDialogueStarted)
            {
                // Wait for scene transition to finish (fade in complete)
                if (!SceneManager.IsTransitioning())
                {
                    hasDialogueStarted = true;
                    isDialogueActive = true;
                    currentDialogueIndex = 0;
                    dialogueTimer = 0.0f;

                    // Activate the first dialogue immediately
                    if (dialogueEntities[0] != null)
                    {
                        InternalCalls.SetActive(dialogueEntities[0].ID, true);
                    }
                }
            }

            // If dialogue is playing, run the sequence and SKIP normal movement
            if (isDialogueActive)
            {
                HandleDialogueSequence();

                // Force physics/anim to idle so player doesn't glide
                horizontalInput = 0f;
                UpdateAnimationState(false);
                return; // STOP EXECUTION HERE
            }

            // 1. HANDLE STATE SWITCH
            if (swapPauseTimer > 0.0f)
            {
                horizontalInput = 0f;
                UpdateAnimationState(true);
                return;
            }

            // 2. POLL INPUT
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

            // Visual Flip
            if (!facingRight && tf.Size.x > 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);
            else if (facingRight && tf.Size.x < 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);

            isJumpHeld = Input.IsKeyPressed(Key.Space);

            // 3. JUMP & DROP DOWN LOGIC
            if (Input.IsKeyTriggered(Key.Space) && IsGrounded && !isPassthrough)
            {
                jumpRequested = true;
            }

            // WEIGHT MECHANIC
            if (Input.IsKeyTriggered(Key.E) && IsGrounded && CurrentEnergy >= energyConsumption)
            {
                RemoveEnergy(energyConsumption);
                isLight = !isLight;
                AudioManager.GetInstance().PlayAttached("switchMassSound", Entity.ID);
                rb.Mass = isLight ? lightMass : normalMass;
                swapPauseTimer = swapPause;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
            }

            // 4. AUDIO & ANIMATION
            HandleFootsteps();
            UpdateAnimationState(false);
        }

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
                return;
            }

            // --- CHECK IF TYPING IS DONE ---
            // Using InternalCalls exactly like CutsceneScript
            bool isTypingDone = InternalCalls.IsTextFinished(currentEnt.ID);

            if (isTypingDone)
            {
                // Start cooldown timer once text is fully revealed
                dialogueTimer += (float)Time.DT;

                if (dialogueTimer >= dialogueCooldown)
                {
                    // Cooldown finished, move to next
                    InternalCalls.SetActive(currentEnt.ID, false); // Hide current

                    currentDialogueIndex++;
                    dialogueTimer = 0.0f;

                    if (currentDialogueIndex < dialogueEntities.Length)
                    {
                        // Activate Next
                        if (dialogueEntities[currentDialogueIndex] != null)
                            InternalCalls.SetActive(dialogueEntities[currentDialogueIndex].ID, true);
                    }
                    else
                    {
                        // All done, return control to player
                        isDialogueActive = false;
                    }
                }
            }
        }

        public override void FixedUpdate()
        {
            if (!alive) return;

            // Handle Timers
            if (swapPauseTimer > 0.0f) swapPauseTimer -= FIXED_TIMESTEP;
            if (jumpCooldown > 0.0f) jumpCooldown -= FIXED_TIMESTEP;

            // 1. GROUND CHECK (RAYCAST)
            CheckGround();

            // 2. HANDLE LANDING SOUND
            HandleLanding();

            // 3. EXECUTE JUMP
            // Ensure we cannot jump if dialogue is active
            if (jumpRequested && !isDialogueActive)
            {
                AudioManager.GetInstance().PlayAttached("JumpUp", Entity.ID);

                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));

                IsGrounded = false;
                jumpRequested = false;
                jumpCooldown = 0.1f;
            }

            // 4. MOVEMENT PHYSICS
            ApplyMovementPhysics();

            // 5. GRAVITY
            ApplyGravity();
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
            bool leftIsGround = false;
            if (hitLeft && hitID != 0) leftIsGround = IsGroundOrPlatform(hitID);

            bool hitRight = InternalCalls.Raycast(ref startR, ref endR, out hitID, Entity.ID);
            bool rightIsGround = false;
            if (hitRight && hitID != 0) rightIsGround = IsGroundOrPlatform(hitID);

            IsGrounded = (leftIsGround || rightIsGround) && rb.LinearVelocity.y <= 0.1f;
        }

        private bool IsGroundOrPlatform(uint entityID)
        {
            Entity hitEnt = new Entity(entityID);
            var tagComp = hitEnt.GetComponent<TagComponent>();
            if (tagComp == null) return false;
            return tagComp.HasTag("Ground") || tagComp.HasTag("Platform") || tagComp.HasTag("Passthrough");
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

            // Only calculate speed if NOT in dialogue
            if (horizontalInput != 0f && !OnWall && !isDialogueActive)
            {
                float maxSpeed = IsGrounded ? maxRunSpeed : maxAirSpeed;
                targetSpeed = horizontalInput * maxSpeed;
            }

            float speedDif = targetSpeed - velocity.x;
            float accelRate;

            if (Math.Abs(targetSpeed) > 0.01f) accelRate = IsGrounded ? acceleration : airAcceleration;
            else accelRate = IsGrounded ? deceleration : 0.5f;

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

            if (rb.LinearVelocity.y < 0)
            {
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            }
            else if (rb.LinearVelocity.y > 0 && !isJumpHeld)
            {
                rb.AddForce(new CEVec3(0f, -9.81f * (gravityScale * 0.5f) * rb.Mass * gravityForceMultiplier, 0f));
            }
        }

        public bool OnPreSolve(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag != null && Tag.HasTag("Passthrough") && canPassthrough) { isPassthrough = true; }

            if (Tag != null && Tag.HasTag("Passthrough"))
            {
                if (Input.IsKeyPressed(Key.S)) { canPassthrough = true; return false; }
                if (rb.LinearVelocity.y > 0.1f) { canPassthrough = true; return false; }

                if (isPassthrough) return false;
                else { canPassthrough = false; return true; }
            }
            canPassthrough = false;
            return true;
        }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag != null && Tag.HasTag("Wall"))
            {
                OnWall = true;
                rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
            }

            if (Tag != null && (Tag.HasTag("Lava") || Tag.HasTag("Enemy")))
            {
                HandleDeath();
            }
        }

        public void OnCollideStay(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Passthrough") && canPassthrough) { isPassthrough = true; }
        }

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag != null && Tag.HasTag("Wall")) { OnWall = false; }
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
            if (IsGrounded && !wasGroundedLastFrame)
            {
                AudioManager.GetInstance().PlayAttached("jumpLand", Entity.ID);
            }
            wasGroundedLastFrame = IsGrounded;
        }

        private void UpdateAnimationState(bool isSwitching)
        {
            // NEW: If dialogue is active, force Idle animation
            if (isDialogueActive)
            {
                string idle = "playerIdle";
                if (isLight) idle += "Light";
                if (currentAnimState != idle)
                {
                    if (anim != null) anim.Play(idle, true);
                    currentAnimState = idle;
                }
                return;
            }

            string targetState = "playerIdle";

            if (isSwitching) targetState = "playerSwitch";
            else if (!IsGrounded)
            {
                if (rb.LinearVelocity.y > 1.0f) targetState = "playerJump";
                else if (rb.LinearVelocity.y < -12.5f) targetState = "playerFall";
            }
            else
            {
                if (Math.Abs(horizontalInput) > 0.01f) targetState = "playerWalk";
            }

            if (isLight) targetState += "Light";

            if (targetState != currentAnimState)
            {
                if (anim != null) anim.Play(targetState, true);
                currentAnimState = targetState;
            }
        }

        private void HandleDeath()
        {
            Debug.Log("Player Died");
            alive = false;
            Entity gameOverEnt = Entity.Find("GameOverScreen");
            if (gameOverEnt != null)
            {
                var gameOverT = gameOverEnt.GetComponent<TransformComponent>();
                gameOverT.Position = new CEVec3(gameOverT.Position.x, gameOverT.Position.y, 0.8f);
            }
        }

        public void AddEnergy(float amount) { CurrentEnergy = Math.Min(CurrentEnergy + amount, MaxEnergy); }
        public void RemoveEnergy(float amount) { CurrentEnergy = Math.Max(CurrentEnergy - amount, 0.0f); }
    }
}
