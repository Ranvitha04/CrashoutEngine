/*!
@file       PlayerMovement.cs
@author     lee.r@digipen.edu
@date       23/02/2026
@brief      Handles player movement, jumping, mass-switching, and animations with deferred state toggling.
*/
using System;

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
        public float fallGravityMultiplier = 1.5f;

        public float footstepInterval = 0.3f;
        public float swapPause = 1.0f;
        public float energyConsumption = 12.5f;
        [SerializeField] private int coyoteFrames = 6;

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
        [SerializeField] public bool playDialogueOnStart = true;
        [SerializeField] public float dialogueCooldown = 1.0f;
        private Entity[] dialogueEntities;
        private int currentDialogueIndex = 0;
        public bool isDialogueActive = false;
        private bool hasDialogueStarted = false;
        private float dialogueTimer = 0.0f;

        // --- STATE VARIABLES ---
        private float horizontalInput;
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
        private CEVec3 respawnPos;

        private string currentAnimState = "";
        private bool isLight = false;
        private float swapPauseTimer = 0.0f;
        private bool stateChangedThisSwap = false; // Prevents double-toggling

        private uint landingDustID = 0;
        private float landingDustTimer = 0.0f;
        private bool landingDustArmed = false;

        private float jumpCooldown = 0.0f;
        private bool isPhasing = false;
        private int passthroughContactCount = 0;
        private int coyoteFramesRemaining = 0;

        private Entity pauseScreenEntity;
        private CEVec3 storedVelocity;
        [SerializeField] private bool reloadOnDeath = false;
        [SerializeField] private string reloadSceneName = "";

        // --- CORE LOGIC ---

        public void SetPauseState(bool p)
        {
            isPaused = p;
            if (isPaused)
            {
                storedVelocity = rb.LinearVelocity;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
                anim.Play("playerIdle" + (isLight ? "Light" : ""));
            }
            else { rb.LinearVelocity = storedVelocity; }
        }

        private void ApplyFlipping()
        {
            if (tf == null) return;
            float absX = Math.Abs(tf.Size.x);
            if (!facingRight && tf.Size.x > 0) tf.Size = new CEVec2(-absX, tf.Size.y);
            else if (facingRight && tf.Size.x < 0) tf.Size = new CEVec2(absX, tf.Size.y);
        }

        public override void Start()
        {
            currentGoalPlates = 0;
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>();

            if (rb == null || tf == null) return;

            pauseScreenEntity = Entity.Find("Pause");
            if (pauseScreenEntity != null) InternalCalls.SetActive(pauseScreenEntity.ID, false);

            if (!string.IsNullOrEmpty(landingDustName))
            {
                Entity e = Entity.Find(landingDustName);
                if (e != null) { landingDustID = e.ID; InternalCalls.Particle_SetEmitting(landingDustID, false); }
            }

            rb.Mass = normalMass;
            IsGrounded = false;
            CurrentEnergy = MaxEnergy;
            respawnPos = tf.Position;

            // Audio Setup
            for (int i = 1; i <= 8; i++) AudioManager.SetClip3DDistances(Entity.ID, "footstep" + i, 10.0f, 500.0f);
            AudioManager.SetClip3DDistances(Entity.ID, "jumpLand", 20.0f, 600.0f);
            AudioManager.SetClip3DDistances(Entity.ID, "JumpUp", 20.0f, 600.0f);
            AudioManager.SetClip3DDistances(Entity.ID, "switchMassSound", 20.0f, 600.0f);

            // Add an effect to the mass switching to make it sound "muffled" or "heavy"
            AudioManager.AddClipDSP(Entity.ID, "switchMassSound", AudioManager.FMOD_DSP_TYPE.LOWPASS);

            dialogueEntities = new Entity[5];
            for (int i = 0; i < 5; ++i)
            {
                dialogueEntities[i] = Entity.Find("PlayerDialogue_" + (i + 1));
                if (dialogueEntities[i] != null) InternalCalls.SetActive(dialogueEntities[i].ID, false);
            }

            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
        }

        public override void Update()
        {
            if (tf == null || rb == null) return;

            // Update Audio Listener to follow the player
            AudioManager.SetListenerPosition(tf.Position);

            if (isPaused || !alive) return;

            // 1. Facing/Flipping (Always runs)
            if (Input.IsKeyPressed(Key.A)) facingRight = false;
            else if (Input.IsKeyPressed(Key.D)) facingRight = true;
            ApplyFlipping();

            // 2. Dialogue Block
            if (playDialogueOnStart && isDialogueActive)
            {
                if (!hasDialogueStarted && !SceneManager.IsTransitioning())
                {
                    hasDialogueStarted = true; isDialogueActive = true; currentDialogueIndex = 0;
                    if (dialogueEntities[0] != null) InternalCalls.SetActive(dialogueEntities[0].ID, true);
                }
                HandleDialogueSequence();
                horizontalInput = 0f;
                UpdateAnimationState(false);
                return;
            }

            // 3. State Switch Check (Input E)
            if (Input.IsKeyTriggered(Key.E) && IsGrounded && CurrentEnergy >= energyConsumption && swapPauseTimer <= 0.0f)
            {
                RemoveEnergy(energyConsumption);
                swapPauseTimer = swapPause;
                stateChangedThisSwap = false;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
                AudioManager.GetInstance().PlayAttached("switchMassSound", Entity.ID);
            }

            // 4. Block Movement Input if Switching
            if (swapPauseTimer > 0.0f)
            {
                horizontalInput = 0f;
                UpdateAnimationState(true); // Signal switching state
                return;
            }

            // 5. Movement Input
            horizontalInput = 0f;
            if (Input.IsKeyPressed(Key.A)) horizontalInput = -1f;
            else if (Input.IsKeyPressed(Key.D)) horizontalInput = 1f;

            if (Input.IsKeyTriggered(Key.Space) && (IsGrounded || coyoteFramesRemaining > 0) && !isPhasing) jumpRequested = true;

            // 6. Visual Logic
            if (landingDustTimer > 0.0f)
            {
                landingDustTimer -= (float)Time.DT;
                if (landingDustTimer <= 0.0f && landingDustID != 0) InternalCalls.Particle_SetEmitting(landingDustID, false);
            }

            HandleFootsteps();
            UpdateAnimationState(false);
        }

        public override void FixedUpdate()
        {
            if (tf == null || rb == null || !alive || isPaused) return;

            // Handle Switch Timer and Deferred State Toggle
            if (swapPauseTimer > 0.0f)
            {
                swapPauseTimer -= FIXED_TIMESTEP;
                
                // Toggle mode halfway through the timer (0.5s of a 1.0s pause)
                if (swapPauseTimer < (swapPause * 0.5f) && !stateChangedThisSwap)
                {
                    isLight = !isLight;
                    rb.Mass = isLight ? lightMass : normalMass;
                    stateChangedThisSwap = true;
                }
            }

            if (jumpCooldown > 0.0f) jumpCooldown -= FIXED_TIMESTEP;

            bool wasGrounded = IsGrounded;
            CheckGround();
            if (IsGrounded) coyoteFramesRemaining = coyoteFrames;
            else if (coyoteFramesRemaining > 0) coyoteFramesRemaining--;

            if (!wasGrounded && IsGrounded && landingDustArmed && !isPhasing)
            {
                TriggerLandingDust();
                landingDustArmed = false;
            }

            HandleLanding();

            if (jumpRequested && !isDialogueActive)
            {
                AudioManager.GetInstance().PlayAttached("JumpUp", Entity.ID);
                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                IsGrounded = false; jumpRequested = false; jumpCooldown = 0.1f; landingDustArmed = true; coyoteFramesRemaining = 0;
            }

            ApplyMovementPhysics();
            ApplyGravity();
        }

        private void UpdateAnimationState(bool isSwitching)
        {
            if (anim == null) return;

            if (isDialogueActive)
            {
                string idle = isLight ? "playerIdleLight" : "playerIdle";
                if (currentAnimState != idle) { anim.Play(idle, true); currentAnimState = idle; }
                return;
            }

            string targetState = "playerIdle";

            if (isSwitching)
            {
                // CRITICAL: We choose the animation based on what we are BECOMING.
                // If state has NOT changed yet, we play the "out" animation for current mode.
                // If it HAS changed, we play the same animation to finish the transition.
                if (!stateChangedThisSwap)
                {
                    targetState = isLight ? "playerSwitch" : "playerSwitchLight";
                }
                else
                {
                    // After the toggle, we are now technically in the target mode
                    // so we keep playing the transition that matches our new mode's "entry".
                    targetState = isLight ? "playerSwitchLight" : "playerSwitch";
                }
            }
            else
            {
                if (rb.LinearVelocity.y < -12.5f) targetState = "playerFall";
                else if (!IsGrounded) targetState = (rb.LinearVelocity.y > 1.0f) ? "playerJump" : "playerFall";
                else if (Math.Abs(horizontalInput) > 0.01f) targetState = "playerWalk";
                
                if (isLight) targetState += "Light";
            }

            if (targetState != currentAnimState) { anim.Play(targetState, true); currentAnimState = targetState; }
        }

        // --- PHYSICS HELPERS ---

        private void ApplyMovementPhysics()
        {
            if (swapPauseTimer > 0.0f) { rb.AddForce(new CEVec3(-rb.LinearVelocity.x * 100.0f, 0f, 0f)); return; }
            CEVec3 v = rb.LinearVelocity;
            float targetSpeed = (horizontalInput != 0f && !OnWall && !isDialogueActive) ? 
                (horizontalInput * (IsGrounded ? maxRunSpeed : maxAirSpeed)) : 0.0f;
            
            float speedDif = targetSpeed - v.x;
            float accelRate = (Math.Abs(targetSpeed) > 0.01f) ? (IsGrounded ? acceleration : airAcceleration) : (IsGrounded ? deceleration : 0.5f);
            rb.AddForce(new CEVec3(speedDif * accelRate * rb.Mass, 0f, 0f));
        }

        private void ApplyGravity()
        {
            if (!IsGrounded)
            {
                float baseGrav = -9.81f * gravityScale * rb.Mass * gravityForceMultiplier;
                if (rb.LinearVelocity.y < 0) rb.AddForce(new CEVec3(0f, baseGrav * fallGravityMultiplier, 0f));
                else rb.AddForce(new CEVec3(0f, baseGrav, 0f));
            }
        }

        private void CheckGround()
        {
            if (jumpCooldown > 0.0f) { IsGrounded = false; return; }
            float halfH = Math.Abs(tf.Size.y) * 0.5f;
            float buffer = 5.0f;
            float xOff = groundCheckWidth * 0.5f;
            CEVec3 origin = tf.Position;
            CEVec3 startL = new CEVec3(origin.x - xOff, origin.y, origin.z);
            CEVec3 endL = new CEVec3(origin.x - xOff, origin.y - (halfH + buffer), origin.z);
            CEVec3 startR = new CEVec3(origin.x + xOff, origin.y, origin.z);
            CEVec3 endR = new CEVec3(origin.x + xOff, origin.y - (halfH + buffer), origin.z);
            uint hitID = 0;
            bool hitL = InternalCalls.Raycast(ref startL, ref endL, out hitID, Entity.ID);
            bool leftG = hitL && hitID != 0 && IsGroundOrPlatform(hitID);
            bool hitR = InternalCalls.Raycast(ref startR, ref endR, out hitID, Entity.ID);
            bool rightG = hitR && hitID != 0 && IsGroundOrPlatform(hitID);
            IsGrounded = (leftG || rightG) && rb.LinearVelocity.y <= 0.1f;
        }

        private bool IsGroundOrPlatform(uint id)
        {
            var tag = new Entity(id).GetComponent<TagComponent>();
            if (tag == null) return false;
            if (tag.HasTag("Ground") || tag.HasTag("Platform")) return true;
            if (tag.HasTag("Passthrough") && !isPhasing) return true;
            return false;
        }

        // --- COLLISION HANDLERS ---

        public bool OnPreSolve(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();
            if (tag != null && tag.HasTag("Passthrough"))
            {
                if (rb.LinearVelocity.y > 0.1f || Input.IsKeyHeld(Key.S)) isPhasing = true;
                return !isPhasing;
            }
            return true;
        }

        public void OnCollideEnter(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();
            if (tag == null) return;
            if (tag.HasTag("Wall")) { OnWall = true; rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0); }
            if (tag.HasTag("Lava") || tag.HasTag("Enemy")) HandleDeath();
            if (tag.HasTag("Passthrough")) passthroughContactCount++;
        }

        public void OnCollideExit(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();
            if (tag == null) return;
            if (tag.HasTag("Wall")) OnWall = false;
            if (tag.HasTag("Passthrough")) { passthroughContactCount--; if (passthroughContactCount <= 0) { passthroughContactCount = 0; isPhasing = false; } }
        }

        // --- UTILS ---

        private void HandleDialogueSequence()
        {
            if (currentDialogueIndex >= dialogueEntities.Length) { isDialogueActive = false; return; }
            Entity ent = dialogueEntities[currentDialogueIndex];
            if (ent == null) { isDialogueActive = false; return; }
            if (Input.IsKeyTriggered(Key.Space) || (InternalCalls.IsTextFinished(ent.ID) && (dialogueTimer += (float)Time.DT) >= dialogueCooldown))
            {
                InternalCalls.SetActive(ent.ID, false); currentDialogueIndex++; dialogueTimer = 0.0f;
                if (currentDialogueIndex < dialogueEntities.Length && dialogueEntities[currentDialogueIndex] != null)
                    InternalCalls.SetActive(dialogueEntities[currentDialogueIndex].ID, true);
                else isDialogueActive = false;
            }
        }

        private void HandleFootsteps()
        {
            if (horizontalInput != 0.0f && IsGrounded && swapPauseTimer <= 0.0f)
            {
                if ((footstepTimer -= (float)Time.DT) <= 0.0f)
                {
                    AudioManager.GetInstance().PlayAttached("footstep" + rng.Next(1, 9), Entity.ID);
                    footstepTimer = footstepInterval;
                }
            } else footstepTimer = 0.0f;
        }

        private void HandleLanding() { if (IsGrounded && !wasGroundedLastFrame && !isPhasing) AudioManager.GetInstance().PlayAttached("jumpLand", Entity.ID); wasGroundedLastFrame = IsGrounded; }
        private void TriggerLandingDust() { if (landingDustID == 0 || tf == null) return; var dustTf = new Entity(landingDustID).GetComponent<TransformComponent>(); if (dustTf != null) dustTf.Position = new CEVec3(tf.Position.x, tf.Position.y - Math.Abs(tf.Size.y) * 0.5f, dustTf.Position.z); InternalCalls.Particle_SetEmitting(landingDustID, true); landingDustTimer = dustBurstDuration; }
        private void HandleDeath()
        {
            alive = false;
            if (!reloadOnDeath)
            {
                Respawn();
                return;
            }

            if (!SceneManager.IsTransitioning())
            {
                string target = string.IsNullOrEmpty(reloadSceneName) ? "tutorial2" : reloadSceneName;
                SceneManager.LoadScene(target, 1.0f);
            }
        }
        public void Respawn() { alive = true; tf.Position = respawnPos; rb.LinearVelocity = new CEVec3(0, 0, 0); CurrentEnergy = MaxEnergy; isPhasing = false; passthroughContactCount = 0; }
        public void AddEnergy(float amount) { CurrentEnergy = Math.Min(CurrentEnergy + amount, MaxEnergy); }
        public void RemoveEnergy(float amount) { CurrentEnergy = Math.Max(CurrentEnergy - amount, 0.0f); }
        public void UpdateCheckpoint(CEVec3 newPos) { respawnPos = newPos; }
    }
}
