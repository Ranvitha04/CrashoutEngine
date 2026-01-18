using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerMovement : ScriptBase
    {
        // --- TUNING VARIABLES ---
        [SerializeField] public float moveSpeed = 200f;
        [SerializeField] public float maxRunSpeed = 100.0f;
        [SerializeField] public float jumpPower = 20000f;

        [SerializeField] public float groundDrag = 0.15f;
        [SerializeField] public float airDrag = 0.02f;

        [SerializeField] public float gravityScale = 2.5f;
        [SerializeField] public float turnMultiplier = 2.0f;

        [SerializeField] public float frictionMultiplier = 500.0f;
        [SerializeField] public float gravityForceMultiplier = 10.0f;
        [SerializeField] public float footstepInterval = 0.075f;
        [SerializeField] public float swapPause = 1f;
        [SerializeField] public float energyConsumption = 12.5f;

        // Mass Settings
        public float normalMass = 7.0f;
        public float lightMass = 4.5f;

        // --- STATE VARIABLES ---
        private float horizontalInput;
        private bool isJumpHeld;
        private bool facingRight = true;

        // Public state for other scripts
        public bool IsGrounded { get; private set; } = false;
        public bool OnWall { get; private set; } = false;

        // Singleton Instance
        public static PlayerMovement Instance;

        // Energy Data
        public float CurrentEnergy = 0.0f;
        public float MaxEnergy = 100.0f;

        // Components
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim;

        private float footstepTimer = 0.0f;
        private Random rng = new Random();

        public bool alive = true;
        public bool isPaused = false;

        // Animation State
        private string currentAnimState = "";
        private bool isLight = false;
        private float swapPauseTimer = 0.0f;

        public override void Start()
        {
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>();

            // Initial Setup
            rb.Mass = normalMass;
            IsGrounded = false;

            // Play music
            AudioManager.GetInstance().Play("ambient");
            AudioManager.GetInstance().Play("bgm1");

            CurrentEnergy = MaxEnergy;
            swapPauseTimer = 0.0f;

            // Register Callbacks
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            CollisionEvents.RegisterTriggerStay(Entity.ID, this.OnTriggerStay);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnTriggerExit);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
        }

        public override void Update()
        {
            if(isPaused) { return; }
            // 0. HANDLE DEATH / PAUSE
            if (!alive) return;

            // 1. HANDLE STATE SWITCH TIMER
            bool isSwitching = false;
            if (swapPauseTimer > 0.0f)
            {
                swapPauseTimer -= (float)Time.DT;
                isSwitching = true;
                
                // FREEZE MOVEMENT INPUT
                horizontalInput = 0f;
            }
            else
            {
                // 2. POLL INPUT (Only if not switching)
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

                // Flip Sprite Scale based on direction
                if (!facingRight && tf.Size.x > 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);
                else if (facingRight && tf.Size.x < 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);

                // Check Jump Hold State
                isJumpHeld = Input.IsKeyPressed(Key.Space);

                // 3. JUMP TRIGGER
                if (Input.IsKeyTriggered(Key.Space) && IsGrounded)
                {
                    // Reset Y velocity for consistent jump height
                    rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                    rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                    IsGrounded = false; // Immediately unground to prevent double jumps before physics updates
                }

                // 4. WEIGHT MECHANIC TRIGGER (Q)
                // "Only allowed to swap state when grounded"
                if (Input.IsKeyTriggered(Key.Q) && IsGrounded && CurrentEnergy >= energyConsumption)
                {
                    RemoveEnergy(energyConsumption);

                    // Toggle State
                    isLight = !isLight;

                    if (isLight) rb.Mass = lightMass;
                    else rb.Mass = normalMass;

                    Debug.Log("Mass toggled to: " + rb.Mass);

                    // Start Cooldown/Freeze
                    swapPauseTimer = swapPause;

                    // Kill velocity so we don't slide while switching
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }
            }

            // 5. AUDIO LOGIC (Footsteps)
            // Only play sound if moving, grounded, and NOT switching
            if (horizontalInput != 0.0f && IsGrounded && !isSwitching)
            {
                footstepTimer -= (float)Time.DT;
                if (footstepTimer <= 0.0f)
                {
                    int index = rng.Next(1, 7);
                    string soundName = "footstep" + index;
                    AudioManager.GetInstance().Play(soundName);
                    footstepTimer = footstepInterval;
                }
            }
            else
            {
                footstepTimer = 0.0f;
            }

            // 6. DETERMINE ANIMATION STATE (The Fix)
            UpdateAnimationState(isSwitching);
        }

        private void UpdateAnimationState(bool isSwitching)
        {
            // Default to Idle so we never have an empty state
            string targetState = "playerIdle";

            // PRIORITY 1: Switching (Overrides everything)
            if (isSwitching)
            {
                targetState = "playerSwitch";
            }
            // PRIORITY 2: Airborne (Jump/Fall)
            else if (!IsGrounded)
            {
                // If moving up noticeably
                if (rb.LinearVelocity.y > 1.0f)
                {
                    targetState = "playerJump";
                }
                // If falling OR hovering near zero (apex of jump), default to Fall
                else if (rb.LinearVelocity.y < -12.5f)
                {
                    targetState = "playerFall";
                }
            }
            // PRIORITY 3: Grounded Movement
            else 
            {
                if (Math.Abs(horizontalInput) > 0.01f)
                {
                    targetState = "playerWalk";
                }
                else
                {
                    targetState = "playerIdle";
                }
            }

            // SUFFIX LOGIC: Add "Light" if isLight is true
            if (isLight)
            {
                targetState += "Light";
            }

            // APPLY LOGIC: Only call Play if state changed
            if (targetState != currentAnimState)
            {
                if (anim != null)
                {
                    anim.Play(targetState, true); // true = restart clip
                }
                currentAnimState = targetState;
            }
        }

        public override void FixedUpdate()
        {
            // If paused/switching, apply drag but NO movement force
            if (swapPauseTimer > 0.0f || !alive)
            {
                // Apply simple ground friction to stop sliding
                CEVec3 v = rb.LinearVelocity;
                float d = groundDrag * frictionMultiplier;
                rb.AddForce(new CEVec3(-v.x * d, 0f, 0f));

                // Apply Gravity so we don't float if we switched mid-air (though we restrict switch to ground)
                if (!IsGrounded)
                    rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));

                return;
            }

            // --- NORMAL PHYSICS ---
            CEVec3 velocity = rb.LinearVelocity;

            // A. HORIZONTAL MOVEMENT
            if (horizontalInput != 0f && !OnWall)
            {
                bool isTurning = (horizontalInput > 0 && velocity.x < 0) ||
                                 (horizontalInput < 0 && velocity.x > 0);

                if (isTurning)
                {
                    rb.AddForce(new CEVec3(horizontalInput * moveSpeed * turnMultiplier, 0f, 0f));
                }
                else if (Math.Abs(velocity.x) < maxRunSpeed)
                {
                    rb.AddForce(new CEVec3(horizontalInput * moveSpeed, 0f, 0f));
                }
            }

            // B. DRAG / FRICTION
            if (horizontalInput == 0f)
            {
                float drag = IsGrounded ? groundDrag : airDrag;
                rb.AddForce(new CEVec3(-velocity.x * drag * frictionMultiplier, 0f, 0f));
            }

            // C. VARIABLE GRAVITY
            if (velocity.y < 0)
            {
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            }
            else if (velocity.y > 0 && !isJumpHeld)
            {
                rb.AddForce(new CEVec3(0f, -9.81f * (gravityScale * 0.5f) * rb.Mass * gravityForceMultiplier, 0f));
            }
        }

        // --- COLLISION CALLBACKS ---

        public void OnTriggerEnter(uint otherEntity)
        {
            
        }
        public void OnTriggerStay(uint otherEntity) { }

        public void OnTriggerExit(uint otherEntity)
        {
            
        }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();
            var otherTf = other.GetComponent<TransformComponent>();

            // Ground Check
            if (layer != null && (layer.HasLayer("Ground") || layer.HasLayer("Platform") || layer.HasLayer("Enemy")))
            {
                // Simple foot check: Are we above the object?
                if (otherTf != null && this.tf.Position.y > otherTf.Position.y)
                {
                    IsGrounded = true;
                }
            }

            // Wall Check
            if (layer != null && layer.HasLayer("Wall"))
            {
                OnWall = true;
                rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
            }

            // Hazard Check
            if (layer != null && (layer.HasLayer("Lava") || layer.HasLayer("Enemy")))
            {
                Debug.Log("Player Died");
                alive = false;
                // TODO: Trigger Death Anim here?

                Entity gameOverEnt = Entity.Find("GameOverScreen");
                if (gameOverEnt != null)
                {
                    var gameOverT = gameOverEnt.GetComponent<TransformComponent>();
                    gameOverT.Position = new CEVec3(gameOverT.Position.x, gameOverT.Position.y, 0.8f);
                }
            }
        }
        public void OnCollideStay(uint otherEntity) { }
        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Ground"))
            {
                IsGrounded = false;
            }

            if (layer != null && layer.HasLayer("Wall"))
            {
                OnWall = false;
            }
        }

        public override void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        public void AddEnergy(float amount)
        {
            CurrentEnergy += amount;
            if (CurrentEnergy > MaxEnergy) CurrentEnergy = MaxEnergy;

            // UI Update logic
            Entity barEnt = Entity.Find("BarUI");
            if (barEnt != null)
            {
                TransformComponent barT = barEnt.GetComponent<TransformComponent>();
                float percentile = CurrentEnergy / MaxEnergy;
                barT.Size = new CEVec2(percentile * 0.5f, barT.Size.y);
            }
        }

        public void RemoveEnergy(float amount)
        {
            CurrentEnergy -= amount;
            if (CurrentEnergy < 0.0f) CurrentEnergy = 0.0f;

            Entity barEnt = Entity.Find("BarUI");
            if (barEnt != null)
            {
                TransformComponent barT = barEnt.GetComponent<TransformComponent>();
                float percentile = CurrentEnergy / MaxEnergy;
                barT.Size = new CEVec2(percentile * 0.5f, barT.Size.y);
            }
        }
    }
}