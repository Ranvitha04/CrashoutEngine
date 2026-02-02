using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerMovement : ScriptBase
    {
        // --- TUNING VARIABLES ---
        [SerializeField] public float maxRunSpeed = 150.0f;
        [SerializeField] public float maxAirSpeed = 120.0f;
        [SerializeField] public float acceleration = 10.0f;
        [SerializeField] public float deceleration = 10.0f;
        [SerializeField] public float airAcceleration = 2.0f;

        [SerializeField] public float jumpPower = 20000f;
        [SerializeField] public float gravityScale = 2.5f;
        [SerializeField] public float gravityForceMultiplier = 10.0f;

        [SerializeField] public float footstepInterval = 0.075f;
        [SerializeField] public float swapPause = 1f;
        [SerializeField] public float energyConsumption = 12.5f;

        public float normalMass = 7.0f;
        public float lightMass = 4.5f;

        // --- NEW: RAYCAST SETTINGS ---
        // How far down to check for ground (BoxHeight/2 + Buffer)
        // Adjust this if detection is spotty. Should be slightly longer than half your sprite height.
        public float groundCheckDistance = 26.0f;
        public float groundCheckWidth = 10.0f; // Width between the two raycasts (feet)

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
        private Random rng = new Random();

        private const float FIXED_TIMESTEP = 0.02f;

        public bool alive = true;
        public bool isPaused = false;

        private string currentAnimState = "";
        private bool isLight = false;
        private float swapPauseTimer = 0.0f;

        // NEW: Fall Through Logic
        private float fallThroughTimer = 0.0f;
        private float jumpCooldown = 0.0f; // Prevents grounding immediately after jumping

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

            // Register Callbacks
            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
            // We keep Enter/Exit for Wall logic, but remove Ground logic
            CollisionEvents.RegisterCollideEnter(Entity.ID, this.OnCollideEnter);
            CollisionEvents.RegisterCollideExit(Entity.ID, this.OnCollideExit);
        }

        public override void Update()
        {
            if (isPaused || !alive) return;

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
            bool downPressed = Input.IsKeyPressed(Key.S);

            if (Input.IsKeyTriggered(Key.Space) && IsGrounded)
            {
                if (downPressed)
                {
                    // DROP DOWN REQUEST
                    // Set a timer where we ignore platform collisions
                    fallThroughTimer = 0.25f;
                    // Nudge player down slightly so they don't get stuck in the physics solver
                    rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, -50.0f, 0.0f);
                }
                else
                {
                    // NORMAL JUMP REQUEST
                    jumpRequested = true;
                }
            }

            // WEIGHT MECHANIC
            if (Input.IsKeyTriggered(Key.Q) && IsGrounded && CurrentEnergy >= energyConsumption)
            {
                RemoveEnergy(energyConsumption);
                isLight = !isLight;
                rb.Mass = isLight ? lightMass : normalMass;
                swapPauseTimer = swapPause;
                rb.LinearVelocity = new CEVec3(0, 0, 0);
            }

            // 4. AUDIO & ANIMATION
            HandleFootsteps();
            UpdateAnimationState(false);
        }

        public override void FixedUpdate()
        {
            if (!alive) return;

            // Handle Timers
            if (swapPauseTimer > 0.0f) swapPauseTimer -= FIXED_TIMESTEP;
            if (jumpCooldown > 0.0f) jumpCooldown -= FIXED_TIMESTEP;
            if (fallThroughTimer > 0.0f) fallThroughTimer -= FIXED_TIMESTEP;

            // 1. GROUND CHECK (RAYCAST)
            CheckGround();

            // 2. EXECUTE JUMP
            if (jumpRequested)
            {
                rb.LinearVelocity = new CEVec3(rb.LinearVelocity.x, 0f, 0f);
                rb.AddForce(new CEVec3(0f, jumpPower, 0f));

                IsGrounded = false;
                jumpRequested = false;

                // Add cooldown so Raycast doesn't immediately snap us back to "Grounded"
                // while we are still accelerating upwards
                jumpCooldown = 0.1f;
            }

            // 3. MOVEMENT PHYSICS
            ApplyMovementPhysics();

            // 4. GRAVITY
            ApplyGravity();
        }

        // --- NEW: ROBUST GROUND CHECK ---
        private void CheckGround()
        {
            if (jumpCooldown > 0.0f)
            {
                IsGrounded = false;
                return;
            }

            // We cast two rays: one from left foot, one from right foot.
            // This prevents falling off ledges too early.

            float xOffset = groundCheckWidth * 0.5f;
            CEVec3 origin = tf.Position;

            // Ray 1: Left Foot
            CEVec3 startL = new CEVec3(origin.x - xOffset, origin.y, origin.z);
            CEVec3 endL = new CEVec3(origin.x - xOffset, origin.y - groundCheckDistance, origin.z);

            // Ray 2: Right Foot
            CEVec3 startR = new CEVec3(origin.x + xOffset, origin.y, origin.z);
            CEVec3 endR = new CEVec3(origin.x + xOffset, origin.y - groundCheckDistance, origin.z);

            uint hitID = 0;

            // Check Left
            bool hitLeft = InternalCalls.Raycast(ref startL, ref endL, out hitID, Entity.ID);
            bool leftIsGround = false;
            if (hitLeft && hitID != 0) leftIsGround = IsGroundOrPlatform(hitID);

            // Check Right
            bool hitRight = InternalCalls.Raycast(ref startR, ref endR, out hitID, Entity.ID);
            bool rightIsGround = false;
            if (hitRight && hitID != 0) rightIsGround = IsGroundOrPlatform(hitID);

            // If either foot hit ground, we are grounded
            bool wasGrounded = IsGrounded;
            IsGrounded = (leftIsGround || rightIsGround) && rb.LinearVelocity.y <= 0.1f; // Only grounded if falling or still
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
                // Stop player during swap
                CEVec3 v = rb.LinearVelocity;
                rb.AddForce(new CEVec3(-v.x * 100.0f, 0f, 0f));
                return;
            }

            CEVec3 velocity = rb.LinearVelocity;
            float targetSpeed = 0.0f;

            if (horizontalInput != 0f && !OnWall)
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
            // If switching, manual gravity
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

        // --- COLLISION CALLBACKS ---

        public bool OnPreSolve(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            // Passthrough Check
            if (Tag != null && Tag.HasTag("Passthrough"))
            {
                // 1. If dropping down timer is active, DISABLE collision
                if (fallThroughTimer > 0.0f) return false;

                // 2. If moving UP, DISABLE collision (jump through from bottom)
                if (rb.LinearVelocity.y > 0.0f) return false;

                // 3. Position Check: If we are not fully above the platform, disable collision
                // This prevents snapping to the top if we clip the side
                var otherTf = other.GetComponent<TransformComponent>();
                if (otherTf != null)
                {
                    float playerBottom = tf.Position.y - (Math.Abs(tf.Size.y) * 0.5f);
                    float platTop = otherTf.Position.y + (Math.Abs(otherTf.Size.y) * 0.5f);

                    // Allow a small buffer (5.0f)
                    if (playerBottom < (platTop - 5.0f)) return false;
                }
            }
            return true;
        }

        public void OnCollideEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            // Only using this for Wall / Hazard logic now. Ground logic is Raycast.
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

        public void OnCollideExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag != null && Tag.HasTag("Wall"))
            {
                OnWall = false;
            }
        }

        private void HandleFootsteps()
        {
            if (horizontalInput != 0.0f && IsGrounded && swapPauseTimer <= 0.0f)
            {
                footstepTimer -= (float)Time.DT;
                if (footstepTimer <= 0.0f)
                {
                    int index = rng.Next(1, 7);
                    AudioManager.GetInstance().PlayAttached("footstep" + index, Entity.ID);
                    footstepTimer = footstepInterval;
                }
            }
            else footstepTimer = 0.0f;
        }

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

        // ... [Existing Energy/UI Helpers remain the same] ...
        public void AddEnergy(float amount) { CurrentEnergy = Math.Min(CurrentEnergy + amount, MaxEnergy); }
        public void RemoveEnergy(float amount) { CurrentEnergy = Math.Max(CurrentEnergy - amount, 0.0f); }

    }
}