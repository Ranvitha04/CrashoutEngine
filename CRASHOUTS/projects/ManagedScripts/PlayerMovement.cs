using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerMovement : ScriptBase
    {
        // --- TUNING VARIABLES ---
        [SerializeField] public float moveSpeed = 400f; // Increased force for snappier start
        [SerializeField] public float maxRunSpeed = 150.0f; // Hard Cap on ground speed
        [SerializeField] public float maxAirSpeed = 120.0f; // Hard Cap on air speed
        [SerializeField] public float airControlFactor = 0.5f; // Multiplier for air movement (0.5 = half acceleration in air)

        [SerializeField] public float jumpPower = 20000f;

        [SerializeField] public float groundDrag = 0.2f;
        [SerializeField] public float airDrag = 0.01f; // Very low drag in air so we glide

        [SerializeField] public float gravityScale = 2.5f;
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

        // Fix for infinite fall: Count contacts instead of boolean toggle
        private int groundContactCount = 0;

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
            groundContactCount = 0;

            // Play music
            AudioManager.GetInstance().PlayAttached("ambient", Entity.ID);
            AudioManager.GetInstance().PlayAttached("bgm1", Entity.ID);

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

            // 1. SAFETY RESET
            if (groundContactCount < 0) groundContactCount = 0;

            // 2. HANDLE STATE SWITCH TIMER
            bool isSwitching = false;
            if (swapPauseTimer > 0.0f)
            {
                swapPauseTimer -= (float)Time.DT;
                isSwitching = true;
                horizontalInput = 0f;
            }
            else
            {
                // 3. POLL INPUT (Improved Logic)
                horizontalInput = 0f;

                // Allow input even in air (Air Control), but apply logic in FixedUpdate
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

                // Flip Sprite
                if (!facingRight && tf.Size.x > 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);
                else if (facingRight && tf.Size.x < 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);

                isJumpHeld = Input.IsKeyPressed(Key.Space);

                // JUMP TRIGGER
                if (Input.IsKeyTriggered(Key.Space) && IsGrounded)
                {
                    rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                    rb.AddForce(new CEVec3(0f, jumpPower, 0f));
                    IsGrounded = false;
                    // Note: We do not set horizontalInput to 0 here. Momentum is preserved.
                }

                // WEIGHT MECHANIC
                if (Input.IsKeyTriggered(Key.Q) && IsGrounded && CurrentEnergy >= energyConsumption)
                {
                    RemoveEnergy(energyConsumption);
                    isLight = !isLight;
                    if (isLight) rb.Mass = lightMass;
                    else rb.Mass = normalMass;

                    swapPauseTimer = swapPause;
                    rb.LinearVelocity = new CEVec3(0, 0, 0);
                }
            }

            // 4. AUDIO LOGIC
            if (horizontalInput != 0.0f && IsGrounded && !isSwitching)
            {
                footstepTimer -= (float)Time.DT;
                if (footstepTimer <= 0.0f)
                {
                    int index = rng.Next(1, 7);
                    string clipKey = "footstep" + index;
                    AudioManager.GetInstance().PlayAttached(clipKey, Entity.ID);
                    footstepTimer = footstepInterval;
                }
            }
            else
            {
                footstepTimer = 0.0f;
            }

            // 5. UPDATE ANIMATION
            UpdateAnimationState(isSwitching);
        }

        public override void FixedUpdate()
        {
            if (swapPauseTimer > 0.0f || !alive)
            {
                // Simple drag to stop player when switching
                CEVec3 v = rb.LinearVelocity;
                rb.AddForce(new CEVec3(-v.x * 100.0f, 0f, 0f));
                if (!IsGrounded) rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
                return;
            }

            CEVec3 velocity = rb.LinearVelocity;

            // --- A. HORIZONTAL MOVEMENT WITH SPEED CAP ---

            // 1. Determine Max Speed for this frame
            float currentMaxSpeed = IsGrounded ? maxRunSpeed : maxAirSpeed;

            // 2. Determine Acceleration Force for this frame
            float currentMoveForce = moveSpeed;
            if (!IsGrounded) currentMoveForce *= airControlFactor; // Less force in air

            // 3. Apply Force ONLY if we haven't hit the speed limit
            if (horizontalInput != 0f && !OnWall)
            {
                // Turning around feels snappier if we multiply force
                bool isTurning = (horizontalInput > 0 && velocity.x < 0) || (horizontalInput < 0 && velocity.x > 0);
                float turnBonus = isTurning ? 2.0f : 1.0f;

                // CHECK: Only add force if we are below max speed in that direction
                bool canAccelerate = false;
                if (horizontalInput > 0 && velocity.x < currentMaxSpeed) canAccelerate = true;
                if (horizontalInput < 0 && velocity.x > -currentMaxSpeed) canAccelerate = true;

                if (canAccelerate)
                {
                    rb.AddForce(new CEVec3(horizontalInput * currentMoveForce * turnBonus, 0f, 0f));
                }
            }

            // --- B. DRAG / FRICTION ---
            // If no input, OR if we are moving faster than max speed (e.g. knocked back), apply drag
            if (horizontalInput == 0f || Math.Abs(velocity.x) > currentMaxSpeed)
            {
                float drag = IsGrounded ? groundDrag : airDrag;
                // We clamp the drag force to avoid jittering near 0
                rb.AddForce(new CEVec3(-velocity.x * drag * frictionMultiplier, 0f, 0f));
            }

            // --- C. GRAVITY ---
            if (velocity.y < 0)
            {
                // Fall faster
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            }
            else if (velocity.y > 0 && !isJumpHeld)
            {
                // Short hop (let go of space to fall earlier)
                rb.AddForce(new CEVec3(0f, -9.81f * (gravityScale * 0.5f) * rb.Mass * gravityForceMultiplier, 0f));
            }
        }

        // --- COLLISION CALLBACKS ---

        public bool OnPreSolve(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Passthrough"))
            {
                if (Input.IsKeyPressed(Key.S)) return false;
                else if (rb.LinearVelocity.y > 0.0f) return false;

                var otherTf = other.GetComponent<TransformComponent>();
                if (otherTf != null)
                {
                    float playerBottom = tf.Position.y - (Math.Abs(tf.Size.y) * 0.5f);
                    float platTop = otherTf.Position.y + (Math.Abs(otherTf.Size.y) * 0.5f);
                    if (playerBottom > platTop) return false;
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
            var layer = other.GetComponent<LayerComponent>();
            var otherTf = other.GetComponent<TransformComponent>();

            // Ground Check using COUNTER
            if (layer != null && (layer.HasLayer("Ground") || layer.HasLayer("Platform") || layer.HasLayer("Enemy")))
            {
                if (otherTf != null && this.tf.Position.y > otherTf.Position.y)
                {
                    groundContactCount++;
                    IsGrounded = true;
                }
            }

            // Hazard Check
            if (layer != null && (layer.HasLayer("Lava") || layer.HasLayer("Enemy")))
            {
                Debug.Log("Player Died");
                alive = false;
                // Game Over logic...
            }
        }

        public void OnCollideStay(uint otherEntity)
        {
            // Usually redundant if using counters, but good for safety
            // ...
        }

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();

            // Ground Exit using COUNTER
            if (layer != null && (layer.HasLayer("Ground") || layer.HasLayer("Platform") || layer.HasLayer("Enemy")))
            {
                groundContactCount--;
                if (groundContactCount < 0) groundContactCount = 0;

                if (groundContactCount == 0)
                {
                    IsGrounded = false;
                }
            }
        }

        public override void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        // Energy Logic Helpers
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


        // Animation Logic
        private void UpdateAnimationState(bool isSwitching)
        {
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
