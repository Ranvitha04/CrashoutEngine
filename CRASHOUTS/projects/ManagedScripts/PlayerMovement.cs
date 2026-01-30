using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerMovement : ScriptBase
    {
        // --- TUNING VARIABLES ---

        // NEW: Target Velocity Settings
        [SerializeField] public float maxRunSpeed = 150.0f;   // Target speed on ground
        [SerializeField] public float maxAirSpeed = 120.0f;   // Target speed in air
        [SerializeField] public float acceleration = 10.0f;   // Snappiness (How fast we reach max speed)
        [SerializeField] public float deceleration = 10.0f;   // Friction (How fast we stop)
        [SerializeField] public float airAcceleration = 2.0f; // Air Control (Lower = more momentum)

        [SerializeField] public float jumpPower = 20000f;
        [SerializeField] public float gravityScale = 2.5f;
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
        private bool jumpRequested; // INPUT BUFFER for FixedUpdate
        private bool facingRight = true;

        // Public state for other scripts
        public bool IsGrounded { get; private set; } = false;
        public bool OnWall { get; private set; } = false;

        public static PlayerMovement Instance;

        public float CurrentEnergy = 0.0f;
        public float MaxEnergy = 100.0f;

        // Components
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator anim;

        private float footstepTimer = 0.0f;
        private Random rng = new Random();

        // FIX: Counter instead of boolean to prevent infinite fall
        private int groundContactCount = 0;

        // Constant for manual timer stepping in FixedUpdate
        private const float FIXED_TIMESTEP = 0.02f;

        public bool alive = true;
        public bool isPaused = false;

        private string currentAnimState = "";
        private bool isLight = false;
        private float swapPauseTimer = 0.0f;

        public override void Start()
        {
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            anim = Entity.GetComponent<Animator>();

            rb.Mass = normalMass;
            IsGrounded = false;
            groundContactCount = 0;

            // Play music - Clean up later
            // AudioManager.GetInstance().PlayAttached("ambient", Entity.ID);
            // AudioManager.GetInstance().PlayAttached("bgm1", Entity.ID);

            CurrentEnergy = MaxEnergy;
            swapPauseTimer = 0.0f;

            // Register Callbacks
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            CollisionEvents.RegisterTriggerStay(Entity.ID, this.OnTriggerStay);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnTriggerExit);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
        }

        public override void Update()
        {
            if (isPaused) { return; }
            if (!alive) return;

            // Safety Reset
            if (groundContactCount < 0) groundContactCount = 0;

            // 1. HANDLE STATE SWITCH (Only Input Blocking)
            // We decrement the timer in FixedUpdate for consistency
            bool isSwitching = (swapPauseTimer > 0.0f);

            if (isSwitching)
            {
                horizontalInput = 0f;
            }
            else
            {
                // 2. POLL INPUT
                horizontalInput = 0f;

                // Allow input in air (Air Control)
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

                // JUMP REQUEST (Buffer Input)
                // We do NOT add force here. We ask FixedUpdate to do it.
                if (Input.IsKeyTriggered(Key.Space) && IsGrounded)
                {
                    jumpRequested = true;
                }

                // WEIGHT MECHANIC
                if (Input.IsKeyTriggered(Key.Q) && IsGrounded && CurrentEnergy >= energyConsumption)
                {
                    RemoveEnergy(energyConsumption);
                    isLight = !isLight;
                    if (isLight) rb.Mass = lightMass;
                    else rb.Mass = normalMass;

                    Debug.Log("Mass toggled to: " + rb.Mass);
                    swapPauseTimer = swapPause;

                    // Kill velocity immediately visually
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }
            }

            // 3. AUDIO LOGIC
            if (horizontalInput != 0.0f && IsGrounded && !isSwitching)
            {
                footstepTimer -= (float)Time.DT;
                if (footstepTimer <= 0.0f)
                {
                    int index = rng.Next(1, 7);
                    AudioManager.GetInstance().PlayAttached("footstep" + index, Entity.ID);
                    footstepTimer = footstepInterval;
                }
            }
            else
            {
                footstepTimer = 0.0f;
            }

            // 4. ANIMATION
            UpdateAnimationState(isSwitching);
        }

        public override void FixedUpdate()
        {
            // 1. HANDLE PAUSE TIMER (Physics Step Consistent)
            if (swapPauseTimer > 0.0f)
            {
                swapPauseTimer -= FIXED_TIMESTEP;

                // Apply massive drag to stop player while switching
                CEVec3 v = rb.LinearVelocity;
                rb.AddForce(new CEVec3(-v.x * 100.0f, 0f, 0f));

                // Keep gravity if in air
                if (!IsGrounded)
                    rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));

                return;
            }

            if (!alive) return;

            // 2. EXECUTE JUMP (Physics Step Consistent)
            if (jumpRequested)
            {
                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                IsGrounded = false;
                jumpRequested = false; // Reset request
            }

            // 3. MOVEMENT PHYSICS (Target Velocity Method)
            CEVec3 velocity = rb.LinearVelocity;

            // A. Calculate Desired Target Speed
            float targetSpeed = 0.0f;
            if (horizontalInput != 0f && !OnWall)
            {
                float maxSpeed = IsGrounded ? maxRunSpeed : maxAirSpeed;
                targetSpeed = horizontalInput * maxSpeed;
            }

            // B. Calculate Difference (Target - Current)
            float speedDif = targetSpeed - velocity.x;

            // C. Determine Acceleration Rate
            float accelRate;

            // If we are trying to move (Input != 0)
            if (Math.Abs(targetSpeed) > 0.01f)
            {
                accelRate = IsGrounded ? acceleration : airAcceleration;
            }
            // If we are trying to stop (Input == 0)
            else
            {
                accelRate = IsGrounded ? deceleration : 0.5f; // Very low friction in air
            }

            // D. Apply Force
            // Force = Mass * Acceleration
            // By scaling by SpeedDif, the force naturally decreases as we get closer to top speed.
            float movementForce = speedDif * accelRate * rb.Mass;

            rb.AddForce(new CEVec3(movementForce, 0f, 0f));

            // 4. GRAVITY
            if (velocity.y < 0)
            {
                // Fall faster
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            }
            else if (velocity.y > 0 && !isJumpHeld)
            {
                // Short Hop
                rb.AddForce(new CEVec3(0f, -9.81f * (gravityScale * 0.5f) * rb.Mass * gravityForceMultiplier, 0f));
            }
        }

        // --- COLLISION CALLBACKS ---

        public bool OnPreSolve(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            // Passthrough Check
            if (Tag != null && Tag.HasTag("Passthrough"))
            {
                if (Input.IsKeyPressed(Key.S)) return false;
                else if (rb.LinearVelocity.y > 0.0f) return false;

                var otherTf = other.GetComponent<TransformComponent>();
                if (otherTf != null)
                {
                    float playerBottom = tf.Position.y - (Math.Abs(tf.Size.y) * 0.5f);
                    float platTop = otherTf.Position.y + (Math.Abs(otherTf.Size.y) * 0.5f);
                    // Buffer for floating point errors
                    if (playerBottom > (platTop - 5.0f)) return false;
                }
            }
            return true;
        }

        public void OnTriggerEnter(uint otherEntity) { }
        public void OnTriggerStay(uint otherEntity) { }
        public void OnTriggerExit(uint otherEntity) { }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();
            var otherTf = other.GetComponent<TransformComponent>();

            // Ground Check (Counter)
            if (Tag != null && (Tag.HasTag("Ground") || Tag.HasTag("Platform") || Tag.HasTag("Enemy")))
            {
                if (otherTf != null && this.tf.Position.y > otherTf.Position.y)
                {
                    groundContactCount++;
                    IsGrounded = true;
                }
            }

            // Wall Check
            if (Tag != null && Tag.HasTag("Wall"))
            {
                OnWall = true;
                rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
            }

            // Hazard Check
            if (Tag != null && (Tag.HasTag("Lava") || Tag.HasTag("Enemy")))
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
        }

        public void OnCollideStay(uint otherEntity) { }

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            // Ground Exit (Counter)
            if (Tag != null && (Tag.HasTag("Ground") || Tag.HasTag("Platform") || Tag.HasTag("Enemy")))
            {
                groundContactCount--;
                if (groundContactCount < 0) groundContactCount = 0;

                if (groundContactCount == 0)
                {
                    IsGrounded = false;
                }
            }

            if (Tag != null && Tag.HasTag("Wall"))
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
            UpdateBarUI();
        }

        public void RemoveEnergy(float amount)
        {
            CurrentEnergy -= amount;
            if (CurrentEnergy < 0.0f) CurrentEnergy = 0.0f;
            UpdateBarUI();
        }

        private void UpdateBarUI()
        {
            Entity barEnt = Entity.Find("BarUI");
            if (barEnt != null)
            {
                TransformComponent barT = barEnt.GetComponent<TransformComponent>();
                float percentile = CurrentEnergy / MaxEnergy;
                barT.Size = new CEVec2(percentile * 0.5f, barT.Size.y);
            }
        }

        private void UpdateAnimationState(bool isSwitching)
        {
            string targetState = "playerIdle";

            if (isSwitching)
            {
                targetState = "playerSwitch";
            }
            else if (!IsGrounded)
            {
                if (rb.LinearVelocity.y > 1.0f) targetState = "playerJump";
                else if (rb.LinearVelocity.y < -12.5f) targetState = "playerFall";
            }
            else
            {
                if (Math.Abs(horizontalInput) > 0.01f) targetState = "playerWalk";
                else targetState = "playerIdle";
            }

            if (isLight) targetState += "Light";

            if (targetState != currentAnimState)
            {
                if (anim != null) anim.Play(targetState, true);
                currentAnimState = targetState;
            }
        }
    }
}