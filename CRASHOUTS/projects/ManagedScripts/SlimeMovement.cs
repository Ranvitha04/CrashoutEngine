using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SlimeMovement : ScriptBase
    {
        // --- TUNING VARIABLES (Edit in Inspector) ---
        [SerializeField] private float moveSpeed = 200f;         // Force applied to move
        [SerializeField] private float maxRunSpeed = 100.0f;     // Max horizontal speed cap
        [SerializeField] private float minX = 0.0f;              // min X position (Prowling) 
        [SerializeField] private float maxX = 0.0f;              // max X position (Prowling)

        [SerializeField] private float groundDrag = 0.15f;       // Friction on ground (0.2 = snappy, 0.05 = ice)
        [SerializeField] private float airDrag = 0.02f;          // Friction in air (lower for better air control)

        [SerializeField] private float gravityScale = 2.5f;      // Gravity multiplier when falling (Higher = less floaty)
        [SerializeField] private float turnMultiplier = 2.0f;    // Extra force applied when turning opposite direction

        // "Magic Number" Multipliers (Tweak if mass/units change)
        [SerializeField] private float frictionMultiplier = 500.0f;
        [SerializeField] private float gravityForceMultiplier = 10.0f;
        [SerializeField] private float activationAreaX = 0.0f;   // min/max X for activation
        [SerializeField] private float activationAreaY = 0.0f;   // min/max Y for activation
        [SerializeField] private float activationDist = 15.0f;
        private float activationDistSquared;

        // --- STATE VARIABLES ---
        private bool isProwling;
        private bool isChasing;
        private bool footstepsPlaying = false;

        bool IsGrounded = false;
        bool MoveRight = false;
        bool isGrabbed = false;

        // Components
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private PlayerMovement playerMovement;

        public override void Start()
        {
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            playerMovement = PlayerMovement.Instance;
            // Initial Setup
            IsGrounded = false;
            footstepsPlaying = false;

            activationDistSquared = activationDist * activationDist;

            // Register Callbacks
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            CollisionEvents.RegisterTriggerStay(Entity.ID, this.OnTriggerStay);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnTriggerExit);
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideStay(Entity.ID, this.OnCollideStay);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
        }

        private float distSquared(CEVec3 p1, CEVec3 p2)
        {
            float sqrtX = (p2.x - p1.x) * (p2.x - p1.x);
            float sqrtY = (p2.y - p1.y) * (p2.y - p1.y);
            return (sqrtX - sqrtY);
        }

        public override void Update()
        {
            // Check states and determine movement

            // Flip Sprite Scale based on direction
            if (!MoveRight && tf.Size.x > 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);
            else if (MoveRight && tf.Size.x < 0) tf.Size = new CEVec2(-tf.Size.x, tf.Size.y);

            // 4. AUDIO LOGIC
            if (IsGrounded && !footstepsPlaying)
            {
                //AudioManager.GetInstance().Play("footstep1");
                footstepsPlaying = true;
            }
            else if (!IsGrounded && footstepsPlaying)
            {
                //AudioManager.GetInstance().Stop("footstep1");
                footstepsPlaying = false;
            }

            // Check position of player with respect to Player
            TransformComponent playerTf = playerMovement.Entity.GetComponent<TransformComponent>();
            if(distSquared(playerTf.Position, tf.Position) <= activationDistSquared)
            {
                // In activation area
                if (Input.IsMouseButtonDown(1))
                {
                    Debug.Log("Near enough button down");
                    CEVec2 mousePos = Input.GetMousePosition();
                    // X LIMITS
                    if(mousePos.x <= (tf.Position.x + activationAreaX) && mousePos.x >= (tf.Position.x - activationAreaX) &&
                    // Y LIMITS
                    mousePos.y <= (tf.Position.y + activationAreaY) && mousePos.y >= (tf.Position.y - activationAreaY)){
                        Debug.Log("GRAB THE SLIME!!!");
                        isGrabbed = true;
                    }
                    else isGrabbed = false;
                }
                else isGrabbed = false;
            }
        }

        public override void FixedUpdate()
        {
            // Get current velocity
            CEVec3 velocity = rb.LinearVelocity;

            // ------------------------------------------------------------
            // HORIZONTAL MOVEMENT ( CHECK IF HIT & FACING WALL OR SOMETHING )
            // ------------------------------------------------------------
            if (IsGrounded)
            {
                // Instant Turn Logic: Are we trying to move opposite to our velocity?
                bool isTurning = (velocity.x < 0) ||
                                 (velocity.x > 0);
                if (isTurning)
                {
                    // Apply extra force to turn quickly
                    rb.AddForce(new CEVec3(moveSpeed * turnMultiplier, 0f, 0f));
                }
                // Standard Acceleration
                else if (Math.Abs(velocity.x) < maxRunSpeed)
                {
                    rb.AddForce(new CEVec3(moveSpeed, 0f, 0f));
                }
            }

            // ------------------------------------------------------------
            // VARIABLE GRAVITY (The "Feel" Fix)
            // ------------------------------------------------------------
            // 1. Falling? Fall Faster.
            if (velocity.y < 0)
            {
                // Apply extra downward force
                rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass * gravityForceMultiplier, 0f));
            }
            // 2. Jumping but released Space? Fall Faster immediately (Short Hop).
            else if (velocity.y > 0)
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
            if (layer != null && layer.HasLayer("Ground"))
            {
                // Geometry Check: Are we landing on TOP?
                if (otherTf != null && this.tf.Position.y > otherTf.Position.y)
                {
                    IsGrounded = true;
                }
                else
                {
                    // On the sides? WALL DETECTION
                    MoveRight = !MoveRight;
                    rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
                }
            }
        }

        public void OnTriggerExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var layer = other.GetComponent<LayerComponent>();

            // Leaving Ground
            if (layer != null && layer.HasLayer("Ground"))
            {
                IsGrounded = false;
            }
        }

        // Required placeholders for engine bindings
        public void OnTriggerStay(uint otherEntity) { }
        public void OnCollideEnter(uint otherEntity) { }
        public void OnCollideStay(uint otherEntity) { }
        public void OnCollideExit(uint otherEntity) { }
    }
}