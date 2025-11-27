using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerMovement : ScriptBase
    {
        // --- TUNING VARIABLES (Edit in Inspector) ---
        [SerializeField] public float moveSpeed = 200f;       // Force applied to move
        [SerializeField] public float maxRunSpeed = 15.0f;     // Max horizontal speed cap
        [SerializeField] public float jumpPower = 20000f;      // Jump Force

        [SerializeField] public float groundDrag = 0.15f;      // Friction on ground (0.2 = snappy, 0.05 = ice)
        [SerializeField] public float airDrag = 0.02f;         // Friction in air (lower for better air control)

        [SerializeField] public float gravityScale = 2.5f;     // Gravity multiplier when falling (Higher = less floaty)
        [SerializeField] public float turnMultiplier = 2.0f;   // Extra force applied when turning opposite direction

        // "Magic Number" Multipliers (Tweak if mass/units change)
        [SerializeField] public float frictionMultiplier = 500.0f;
        [SerializeField] public float gravityForceMultiplier = 10.0f;

        // Mass Settings (For Weighted Platform interaction)
        public float normalMass = 7.0f;
        public float lightMass = 4.5f;

        // --- STATE VARIABLES ---
        private float horizontalInput;
        private bool isJumpHeld;
        private bool footstepsPlaying = false;

        // Public state for other scripts
        public bool IsGrounded { get; private set; } = false;
        public bool OnWall { get; private set; } = false;

        // Singleton Instance
        public static PlayerMovement Instance;

        // Components
        private RigidBodyComponent rb;
        private TransformComponent tf;

        public override void Start()
        {
            Instance = this;
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();

            // Initial Setup
            rb.Mass = normalMass;
            IsGrounded = false;
            footstepsPlaying = false;
            AudioManager.GetInstance().Play("ambient");

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
            // 1. POLL INPUT (Happens every frame)
            horizontalInput = 0f;
            if (Input.IsKeyPressed(Key.A)) horizontalInput = -1f;
            if (Input.IsKeyPressed(Key.D)) horizontalInput = 1f;

            // Flip Sprite Scale based on direction
            if (horizontalInput < 0 && tf.Size.x > 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);
            else if (horizontalInput > 0 && tf.Size.x < 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);

            // Check Jump Hold State (For variable jump height)
            isJumpHeld = Input.IsKeyPressed(Key.Space);

            // 2. JUMP TRIGGER
            if (Input.IsKeyTriggered(Key.Space) && IsGrounded)
            {
                // Reset vertical velocity for consistent jump height even if moving down slightly
                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));

                // Optional: Add "Stretch" effect or Jump Sound here
            }

            // 3. WEIGHT MECHANIC
            if (Input.IsKeyTriggered(Key.Q))
            {
                // Floating point comparison with epsilon safety
                if (Math.Abs(rb.Mass - normalMass) < 0.1f) rb.Mass = lightMass;
                else rb.Mass = normalMass;
                Debug.Log("Mass toggled to: " + rb.Mass);
            }

            // 4. AUDIO LOGIC
            if (horizontalInput != 0.0f && IsGrounded && !footstepsPlaying)
            {
                AudioManager.GetInstance().Play("footsteps");
                footstepsPlaying = true;
            }
            else if ((!IsGrounded || horizontalInput == 0.0f) && footstepsPlaying)
            {
                AudioManager.GetInstance().Stop("footsteps");
                footstepsPlaying = false;
            }
        }

        public override void FixedUpdate()
        {
            // Get current velocity
            CEVec3 velocity = rb.LinearVelocity;

            // ------------------------------------------------------------
            // A. HORIZONTAL MOVEMENT
            // ------------------------------------------------------------
            if (horizontalInput != 0f && !OnWall)
            {
                // Instant Turn Logic: Are we trying to move opposite to our velocity?
                bool isTurning = (horizontalInput > 0 && velocity.x < 0) ||
                                 (horizontalInput < 0 && velocity.x > 0);

                if (isTurning)
                {
                    // Apply extra force to turn quickly
                    rb.AddForce(new CEVec3(horizontalInput * moveSpeed * turnMultiplier, 0f, 0f));
                }
                // Standard Acceleration
                else if (Math.Abs(velocity.x) < maxRunSpeed)
                {
                    rb.AddForce(new CEVec3(horizontalInput * moveSpeed, 0f, 0f));
                }
            }

            // ------------------------------------------------------------
            // B. DRAG / FRICTION (Stopping Power)
            // ------------------------------------------------------------
            // If no input is pressed, apply force against velocity to stop.
            if (horizontalInput == 0f)
            {
                float drag = IsGrounded ? groundDrag : airDrag;
                rb.AddForce(new CEVec3(-velocity.x * drag * frictionMultiplier, 0f, 0f));
            }

            // ------------------------------------------------------------
            // C. VARIABLE GRAVITY (The "Feel" Fix)
            // ------------------------------------------------------------
            // 1. Falling? Fall Faster.
            if (velocity.y < 0)
            {
                // Apply extra downward force
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            }
            // 2. Jumping but released Space? Fall Faster immediately (Short Hop).
            else if (velocity.y > 0 && !isJumpHeld)
            {
                // Cut the jump short by applying half the fall gravity
                rb.AddForce(new CEVec3(0f, -9.81f * (gravityScale * 0.5f) * rb.Mass * gravityForceMultiplier, 0f));
            }
        }

        // --- COLLISION CALLBACKS ---

        public void OnTriggerEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();
            var otherTf = other.GetComponent<TransformComponent>();

            // Ground / Platform Detection
            if (layer != null && (layer.HasLayer("Ground") || layer.HasLayer("Platform")))
            {
                // Geometry Check: Are we landing on TOP?
                if (otherTf != null && this.tf.Position.y > otherTf.Position.y)
                {
                    IsGrounded = true;

                    if (layer.HasLayer("Platform"))
                    {
                        // Handled by WeightedPlatform script via Instance check, 
                        // but we flag it here if needed later.
                    }
                }
            }

            // Wall Detection
            if (layer != null && layer.HasLayer("Wall"))
            {
                OnWall = true;
                // Kill horizontal momentum on wall hit to prevent sticky physics
                rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
            }

            // Hazards
            if (layer != null && (layer.HasLayer("Lava") || layer.HasLayer("Enemy")))
            {
                Debug.Log("Player Died");
                // Die();
            }
        }

        public void OnTriggerExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();

            // Leaving Ground
            if (layer != null && (layer.HasLayer("Ground") || layer.HasLayer("Platform")))
            {
                IsGrounded = false;
            }

            // Leaving Wall
            if (layer != null && layer.HasLayer("Wall"))
            {
                OnWall = false;
            }
        }

        // Required placeholders for engine bindings
        public void OnTriggerStay(uint otherEntity) { }
        public void OnCollideEnter(uint otherEntity) { }
        public void OnCollideStay(uint otherEntity) { }
        public void OnCollideExit(uint otherEntity) { }

        public override void OnDestroy()
        {
            // Cleanup Singleton to prevent accessing dead object
            if (Instance == this) Instance = null;
        }
    }
}