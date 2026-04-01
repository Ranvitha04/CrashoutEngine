/*!
@file       PlayerMovement.cs
@author     Renfred Lee
@author     Renfred (RTIS)
@author     Afiq
@date       23/02/2026
@brief      Handles player movement, jumping, mass-switching, and animations.
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
        
        [SerializeField] private float coyoteTime = 0.15f;
        public int goalNoOfPlates = 1;
        public int currentGoalPlates = 0;

        [SerializeField] private string landingDustName = "LandingDust";
        [SerializeField] private float dustBurstDuration = 0.15f;
        [SerializeField] private float landingRumbleMinFallSpeed = 60.0f;
        [SerializeField] private float landingRumbleMaxFallSpeed = 260.0f;
        [SerializeField] private float landingRumbleMinStrength = 0.15f;
        [SerializeField] private float landingRumbleMaxStrength = 0.65f;
        [SerializeField] private float landingRumbleDuration = 0.12f;

        public float normalMass = 7.0f;
        public float lightMass = 4.5f;

        // --- RAYCAST SETTINGS ---
        public float groundCheckDistance = 26.0f;
        public float groundCheckWidth = 10.0f;

        // --- DIALOGUE VARIABLES (UPDATED) ---
        [SerializeField] public bool playDialogueOnStart = true;
        [SerializeField] public float dialogueCooldown = 1.0f;
        
        [SerializeField] public string dialogueParentName = "DialogueContainer";
        [SerializeField] public string dialoguePrefix = "PlayerDialogue_";
        [SerializeField] public int numberOfDialogues = 5;
        
        private Entity dialogueParentEntity;
        private Entity[] dialogueEntities;
        private int currentDialogueIndex = 0;
        public bool isDialogueActive = false;
        private bool hasDialogueStarted = false;
        private float dialogueTimer = 0.0f;

        // --- STATE VARIABLES ---
        public bool facingRight = true;
        private float horizontalInput;
        private bool jumpRequested;
        public bool IsGrounded { get; private set; } = false;
        public bool OnWall { get; private set; } = false;

        public static PlayerMovement Instance;
        public bool lockBookGlowToPlayer = false;
        public static bool RespawnNextLoad = false;
        public float CurrentEnergy = 0.0f;
        public float MaxEnergy = 100.0f;

        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim;

        private float footstepTimer = 0.0f;
        private Random rng = new Random();
        private const float FIXED_TIMESTEP = 0.02f;

        public bool alive = true;
        private bool isDying = false;
        private float deathTimer = 0.0f;
        [SerializeField] private string normalDeathAnimName = "playerDeathNoBook";
        [SerializeField] private string changedMassDeathAnimName = "playerDeath";
        [SerializeField] private bool unlimitedEnergy = false;
        [SerializeField] private bool godMode = false;
        [SerializeField] private float normalDeathDelay = 2.0f;
        [SerializeField] private float changedMassDeathDelay = 2.0f;
        public bool isPaused = false; 
        private static CEVec3 respawnPos;
        private static float checkpointEnergy = 100.0f;
        public static string lastLevelName = "";

        private string currentAnimState = "";
        private bool isLight = false;
        public bool IsLightMode { get { return isLight; } }
        private float swapPauseTimer = 0.0f;
        private bool stateChangedThisSwap = false;
        private bool switchStartIsLight = false;
        private bool switchTargetIsLight = false;

        public bool IsSwitching { get { return swapPauseTimer > 0.0f; } }
        public bool SwitchStartIsLight { get { return switchStartIsLight; } }
        public bool SwitchTargetIsLight { get { return switchTargetIsLight; } }
        public float SwitchProgress01
        {
            get
            {
                if (swapPause <= 0.0001f) return 1.0f;
                float t = 1.0f - (swapPauseTimer / swapPause);
                if (t < 0.0f) return 0.0f;
                if (t > 1.0f) return 1.0f;
                return t;
            }
        }

        private uint landingDustID = 0;
        private float landingDustTimer = 0.0f;
        private bool landingDustArmed = false;
        private float strongestAirborneFallSpeed = 0.0f;

        private float jumpCooldown = 0.0f;
        private float coyoteTimeTimer = 0.0f;
        
        private bool jumpReady = true;

        // --- PASSTHROUGH LOGIC (HEARTBEAT TIMER) ---
        private bool isPhasing = false;
        private float passthroughOverlapTimer = 0.0f;

        private Entity pauseScreenEntity;
        private Entity gameOverScreen;
        private CEVec3 storedVelocity;
        private float storedAngularVelocity = 0.0f;
        [SerializeField] private bool reloadOnDeath = false;
        [SerializeField] private string reloadSceneName = "";
        [SerializeField] private string gameOverSceneName = "Defeat";

        // --- CORE LOGIC ---

        public void SetPauseState(bool p)
        {
            isPaused = p;
            if (isPaused)
            {
                storedVelocity = rb.LinearVelocity;
                storedAngularVelocity = rb.AngularVelocity;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
                rb.AngularVelocity = 0.0f;
                rb.SetDynamic(false);
                anim.Play("playerIdle" + (isLight ? "Light" : ""));
            }
            else
            {
                rb.SetDynamic(true);
                rb.LinearVelocity = storedVelocity;
                rb.AngularVelocity = storedAngularVelocity;
            }
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
            string currentLevel = SceneManager.GetActiveSceneName();
            unlimitedEnergy = false;
            godMode = false;
            currentGoalPlates = 0;
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>();

            if (rb == null || tf == null) return;

            pauseScreenEntity = Entity.Find("Pause");
            if (pauseScreenEntity != null) InternalCalls.SetActive(pauseScreenEntity.ID, false);

            gameOverScreen = Entity.Find("GameOverScreen");

            if (!string.IsNullOrEmpty(landingDustName))
            {
                Entity e = Entity.Find(landingDustName);
                if (e != null) { landingDustID = e.ID; InternalCalls.Particle_SetEmitting(landingDustID, false); }
            }

            rb.Mass = normalMass;
            IsGrounded = false;
            
            if (string.IsNullOrEmpty(lastLevelName) || lastLevelName != currentLevel || !RespawnNextLoad)
            {
                respawnPos = tf.Position;
                checkpointEnergy = MaxEnergy;
                lastLevelName = currentLevel;
                CurrentEnergy = MaxEnergy;
            }
            
            jumpReady = true;

            if (RespawnNextLoad)
            {
                // Disable dialogue flags since we are respawning from a Game Over
                playDialogueOnStart = false;
                isDialogueActive = false;

                Respawn();
                RespawnNextLoad = false;
            }

            for (int i = 1; i <= 8; i++) AudioManager.SetClip3DDistances(Entity.ID, "footstep" + i, 25.0f, 250.0f);
            AudioManager.SetClip3DDistances(Entity.ID, "jumpLand", 20.0f, 600.0f);
            AudioManager.SetClip3DDistances(Entity.ID, "JumpUp", 20.0f, 600.0f);
            AudioManager.SetClip3DDistances(Entity.ID, "switchMassSound", 20.0f, 600.0f);
            AudioManager.AddClipDSP(Entity.ID, "switchMassSound", AudioManager.FMOD_DSP_TYPE.LOWPASS);

            // --- DYNAMIC DIALOGUE SETUP ---
            if (!string.IsNullOrEmpty(dialogueParentName))
            {
                dialogueParentEntity = Entity.Find(dialogueParentName);
                if (dialogueParentEntity != null) InternalCalls.SetActive(dialogueParentEntity.ID, false);
            }

            dialogueEntities = new Entity[numberOfDialogues];
            for (int i = 0; i < numberOfDialogues; ++i)
            {
                dialogueEntities[i] = Entity.Find(dialoguePrefix + (i + 1));
                if (dialogueEntities[i] != null) InternalCalls.SetActive(dialogueEntities[i].ID, false);
            }

            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
        }

        public override void Update()
        {
            if (tf == null || rb == null) return;
            if (unlimitedEnergy)
                CurrentEnergy = MaxEnergy;
                
            if (Input.IsKeyTriggered(Key.Num1))
            {
                unlimitedEnergy = !unlimitedEnergy;
                Debug.Log("Unlimited Energy: " + (unlimitedEnergy ? "ON" : "OFF"));
            }

            if (Input.IsKeyTriggered(Key.Num2))
            {
                godMode = !godMode;
                Debug.Log("God Mode: " + (godMode ? "ON" : "OFF"));
            }

            if (godMode)
            {
                IsGrounded = true;
                coyoteTimeTimer = coyoteTime;
            }
            
            if (Input.IsKeyTriggered(Key.Num3))
            {
                TeleportToTutorialDoor();
            }
            
            AudioManager.SetListenerPosition(tf.Position);
            if (isDying)
            {
                deathTimer -= (float)Time.DT;
                if (deathTimer <= 0.0f)
                {
                    FinishDeathTransition();
                }
                return;
            }

            if (isPaused || !alive) return;

            float moveInputX = isDialogueActive ? 0.0f : Input.GetAxis("MoveX");
            if (moveInputX < -0.01f) facingRight = false;
            else if (moveInputX > 0.01f) facingRight = true;
            ApplyFlipping();

            if (playDialogueOnStart && isDialogueActive)
            {
                if (!hasDialogueStarted && !SceneManager.IsTransitioning())
                {
                    hasDialogueStarted = true; 
                    isDialogueActive = true; 
                    currentDialogueIndex = 0;
                    
                    // Activate the parent container
                    if (dialogueParentEntity != null) InternalCalls.SetActive(dialogueParentEntity.ID, true);

                    // Activate the first dialogue
                    if (dialogueEntities.Length > 0 && dialogueEntities[0] != null) 
                        InternalCalls.SetActive(dialogueEntities[0].ID, true);
                }
                HandleDialogueSequence();
                horizontalInput = 0f;
                UpdateAnimationState(false);
                return;
            }

            if (Input.GetActionTriggered("SwapMass") && IsGrounded && CurrentEnergy >= energyConsumption && swapPauseTimer <= 0.0f)
            {
                RemoveEnergy(energyConsumption);
                switchStartIsLight = isLight;
                switchTargetIsLight = !isLight;
                swapPauseTimer = swapPause;
                stateChangedThisSwap = false;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
                AudioManager.GetInstance().PlayAttached("switchMassSound", Entity.ID);
                AudioManager.GetInstance().PlayAttached("speak", Entity.ID);
            }

            if (swapPauseTimer > 0.0f)
            {
                horizontalInput = 0f;
                UpdateAnimationState(true); 
                return;
            }

            horizontalInput = moveInputX;

            bool wasGroundedThisFrame = IsGrounded;
            if (!IsGrounded && rb.LinearVelocity.y < 0.0f)
            {
                strongestAirborneFallSpeed = Math.Max(strongestAirborneFallSpeed, -rb.LinearVelocity.y);
            }
            CheckGround();

            if (IsGrounded) coyoteTimeTimer = coyoteTime;
            else if (coyoteTimeTimer > 0.0f) coyoteTimeTimer -= (float)Time.DT;

            if (passthroughOverlapTimer > 0.0f)
            {
                passthroughOverlapTimer -= (float)Time.DT;
                if (passthroughOverlapTimer <= 0.0f)
                {
                    isPhasing = false; 
                }
            }

            if (Input.GetAction("DropDown") && IsGrounded && !isPhasing)
            {
                isPhasing = true;
                passthroughOverlapTimer = 0.1f; 
            }

            if (Input.GetActionTriggered("Jump") && (IsGrounded || coyoteTimeTimer > 0.0f) && jumpReady && !isPhasing) 
            {
                jumpRequested = true;
                jumpReady = false; 
                coyoteTimeTimer = 0.0f;
            }

            if (!IsGrounded && rb.LinearVelocity.y < -1.0f) landingDustArmed = true;

            if (!wasGroundedThisFrame && IsGrounded && landingDustArmed)
            {
                TriggerLandingDust();
                HandleLanding();
                landingDustArmed = false;
                strongestAirborneFallSpeed = 0.0f;
            }
            else if (IsGrounded)
            {
                strongestAirborneFallSpeed = 0.0f;
            }

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

            if (swapPauseTimer > 0.0f)
            {
                swapPauseTimer -= FIXED_TIMESTEP;
                if (swapPauseTimer < (swapPause * 0.5f) && !stateChangedThisSwap)
                {
                    isLight = !isLight;
                    rb.Mass = isLight ? lightMass : normalMass;
                    stateChangedThisSwap = true;
                }
            }

            if (jumpCooldown > 0.0f) jumpCooldown -= FIXED_TIMESTEP;

            if (rb.LinearVelocity.y <= 15.0f && jumpCooldown <= 0.0f && !jumpRequested)
            {
                jumpReady = true;
            }

            if (jumpRequested && !isDialogueActive)
            {
                AudioManager.GetInstance().PlayAttached("JumpUp", Entity.ID);
                AudioManager.GetInstance().PlayAttached("jumpVoice", Entity.ID);
                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                
                IsGrounded = false; 
                jumpRequested = false; 
                jumpCooldown = 0.1f; 
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
                if (!stateChangedThisSwap) targetState = isLight ? "playerSwitch" : "playerSwitchLight";
                else targetState = isLight ? "playerSwitchLight" : "playerSwitch";
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
        private void FinishDeathTransition()
        {
            if (!string.IsNullOrEmpty(gameOverSceneName))
            {
                if (!SceneManager.IsTransitioning())
                {
                    SceneManager.LoadScene(gameOverSceneName, 1.0f);
                }
                return;
            }

            if (reloadOnDeath)
            {
                if (!SceneManager.IsTransitioning())
                {
                    RespawnNextLoad = true;
                    string target = string.IsNullOrEmpty(reloadSceneName)
                        ? SceneManager.GetActiveSceneName()
                        : reloadSceneName;

                    SceneManager.LoadScene(target, 1.0f);
                }
            }
        }
        private void CheckGround()
        {
            if (jumpCooldown > 0.0f) { IsGrounded = false; return; }
            float halfH = Math.Abs(tf.Size.y) * 0.5f;
            float buffer = 10.0f; 
            
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
            
            IsGrounded = (leftG || rightG) && rb.LinearVelocity.y <= 15.0f; 
        }

        private bool IsGroundOrPlatform(uint id)
        {
            var tag = new Entity(id).GetComponent<TagComponent>();
            if (tag == null) return false;
            if (tag.HasTag("Ground") || tag.HasTag("Platform")) return true;
            if (tag.HasTag("Passthrough"))
            {
                if (isPhasing) return false;
                return true;
            }
            return false;
        }

        // --- COLLISION HANDLERS ---
        public bool OnPreSolve(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();

            if (godMode && tag != null && tag.HasTag("Lava"))
                return false;

            if (tag != null && tag.HasTag("Passthrough"))
            {
                passthroughOverlapTimer = 0.1f;

                if (rb.LinearVelocity.y > 0.1f)
                {
                    isPhasing = true;
                }

                if (isPhasing) return false;

                return true;
            }

            return true;
        }
        
        public void OnCollideEnter(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();
            if (tag == null) return;
            if (tag.HasTag("Wall")) { OnWall = true; rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0); }
            if (tag.HasTag("Lava") || tag.HasTag("Enemy")) HandleDeath();
        }

        public void OnCollideExit(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();
            if (tag == null) return;
            if (tag.HasTag("Wall")) OnWall = false;
        }

        // --- UTILS ---

        private void HandleDialogueSequence()
        {
            if (currentDialogueIndex >= dialogueEntities.Length) { EndDialogue(); return; }
            Entity ent = dialogueEntities[currentDialogueIndex];
            if (ent == null) { EndDialogue(); return; }
            
            if (Input.GetActionTriggered("Jump") || (InternalCalls.IsTextFinished(ent.ID) && (dialogueTimer += (float)Time.DT) >= dialogueCooldown))
            {
                InternalCalls.SetActive(ent.ID, false); 
                currentDialogueIndex++; 
                dialogueTimer = 0.0f;
                
                if (currentDialogueIndex < dialogueEntities.Length && dialogueEntities[currentDialogueIndex] != null)
                {
                    InternalCalls.SetActive(dialogueEntities[currentDialogueIndex].ID, true);
                }
                else 
                {
                    EndDialogue();
                }
            }
        }
        
        private void EndDialogue()
        {
            isDialogueActive = false;
            if (dialogueParentEntity != null) InternalCalls.SetActive(dialogueParentEntity.ID, false);
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

        private void HandleLanding()
        {
            AudioManager.GetInstance().PlayAttached("jumpLand", Entity.ID);
            AudioManager.GetInstance().PlayAttached("landVoice", Entity.ID);

            if (strongestAirborneFallSpeed < landingRumbleMinFallSpeed || landingRumbleMaxFallSpeed <= landingRumbleMinFallSpeed)
            {
                return;
            }

            float t = (strongestAirborneFallSpeed - landingRumbleMinFallSpeed) / (landingRumbleMaxFallSpeed - landingRumbleMinFallSpeed);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float strength = landingRumbleMinStrength + ((landingRumbleMaxStrength - landingRumbleMinStrength) * t);
            Input.SetControllerVibration(strength * 0.7f, strength, landingRumbleDuration);
        }
        
        private void TriggerLandingDust() 
        { 
            if (landingDustID == 0 || tf == null) return; 
            var dustTf = new Entity(landingDustID).GetComponent<TransformComponent>(); 
            if (dustTf != null) dustTf.Position = new CEVec3(tf.Position.x, tf.Position.y - Math.Abs(tf.Size.y) * 0.5f, dustTf.Position.z); 
            InternalCalls.Particle_SetEmitting(landingDustID, true); 
            landingDustTimer = dustBurstDuration; 
        }

        public void VS_RemoveEnergy25()
        {
            RemoveEnergy(25.0f);
            Debug.Log("Removed 25 energy. CurrentEnergy = " + CurrentEnergy);
        }

        public void VS_AddEnergy25()
        {
            AddEnergy(25.0f);
            Debug.Log("Added 25 energy. CurrentEnergy = " + CurrentEnergy);
        }

        public void VS_PrintEnergy()
        {
            Debug.Log("CurrentEnergy = " + CurrentEnergy);
        }
        
        private void HandleDeath()
        {
            if (godMode) return;
            if (isDying) return;

            alive = false;
            isDying = true;
            rb.LinearVelocity = new CEVec3(0, 0, 0);

            string deathClip = isLight ? normalDeathAnimName : changedMassDeathAnimName;
            deathTimer = isLight ? changedMassDeathDelay : normalDeathDelay;

            if (anim != null)
            {
                anim.Play(deathClip, true);
                currentAnimState = deathClip;
            }

            AudioManager.GetInstance().PlayAttached("deathSound", Entity.ID);
        }

        private void TeleportToTutorialDoor()
        {
            Entity door = Entity.Find("nextleveldoor");
            if (door == null) return;

            var doorTf = door.GetComponent<TransformComponent>();
            if (doorTf == null) return;

            tf.Position = new CEVec3(
                doorTf.Position.x,
                doorTf.Position.y + 60.0f,
                doorTf.Position.z
            );

            if (rb != null)
                rb.LinearVelocity = new CEVec3(0, 0, 0);
        }

        public void VS_Grounded()
        {
            Debug.Log("Branch TRUE: Player is grounded");
        }

        public void VS_Airborne()
        {
            Debug.Log("Branch FALSE: Player is airborne");
        }
        
        public void VS_TwoPlusTwoTrue()
        {
            Console.WriteLine("2 + 2 is 4");
        }

        public void VS_TwoPlusTwoFalse()
        {
            Console.WriteLine("2 + 2 is NOT 4");
        }
        
        public void Respawn() 
        { 
            alive = true; 
            tf.Position = respawnPos; 
            rb.LinearVelocity = new CEVec3(0, 0, 0); 
            CurrentEnergy = checkpointEnergy; 
            isPhasing = false; 
            passthroughOverlapTimer = 0.0f;
            jumpReady = true;
            isDying = false;
            deathTimer = 0.0f;
            isLight = false;
            rb.Mass = normalMass;
            swapPauseTimer = 0.0f;

            if (anim != null)
            {
                anim.Play("playerIdle", true);
                currentAnimState = "playerIdle";
            }
        }
        
        private void TeleportToCheckpoint()
        {
            tf.Position = respawnPos;
            rb.LinearVelocity = new CEVec3(0, 0, 0);
            isPhasing = false;
            passthroughOverlapTimer = 0.0f;
        }
        
        public void AddEnergy(float amount) { CurrentEnergy = Math.Min(CurrentEnergy + amount, MaxEnergy); }
        public void RemoveEnergy(float amount)
        {
            if (unlimitedEnergy)
            {
                CurrentEnergy = MaxEnergy;
                return;
            }

            CurrentEnergy = Math.Max(CurrentEnergy - amount, 0.0f);
        }
        public void UpdateCheckpoint(CEVec3 newPos) 
        { 
            respawnPos = newPos; 
            checkpointEnergy = CurrentEnergy;
        }
    }
}