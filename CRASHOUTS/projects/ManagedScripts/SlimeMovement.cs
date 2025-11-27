using System;
using System.Numerics; // For math helpers

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class SlimeMovement : ScriptBase
    {
        // --- SETTINGS (Edit in Inspector) ---
        [SerializeField] private float moveSpeed = 5000f;      // Force applied
        [SerializeField] private float maxSpeed = 50.0f;       // Speed cap
        [SerializeField] private float turnTimer = 2.0f;       // How often to turn when prowling (if no walls)
        [SerializeField] private float detectionRange = 300.0f;// Distance to spot player
        [SerializeField] private float jumpForce = 15000.0f;   // For hopping
        
        [SerializeField] private float gravityScale = 5.0f;    // Fall faster
        
        // --- INTERNAL STATE ---
        private RigidBodyComponent rb;
        private TransformComponent tf;
        private PlayerMovement playerScript; // Reference to player singleton

        private bool isGrounded = false;
        private bool moveRight = true;
        private float currentTurnTime = 0.0f;
        private bool isChasing = false;
        private bool isGrabbed = false;
        
        // Helper to prevent rapid flipping
        private float flipCooldown = 0.0f; 

        public override void Start()
        {
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            
            // Get Player reference (assuming PlayerMovement has 'public static PlayerMovement Instance')
            playerScript = PlayerMovement.Instance; 

            // Register Physics Callbacks
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnTriggerExit);
            
            // Ensure we have gravity/mass settings correct
            if (rb != null) {
                rb.LinearVelocity = new CEVec3(0,0,0);
            }
            
            Console.WriteLine("[Slime] AI Initialized.");
        }

        public override void Update()
        {
            if (rb == null || tf == null) return;

            float dt = (float)Time.DT;
            if (flipCooldown > 0) flipCooldown -= dt;

            // 1. CHECK GRAB INPUT (Mouse Interaction)
            HandleGrabLogic();
            if (isGrabbed) return; // Don't move if being held

            // 2. AI DECISION LOGIC
            if (playerScript != null)
            {
                // Calculate distance to player
                TransformComponent playerTf = playerScript.Entity.GetComponent<TransformComponent>();
                if (playerTf != null)
                {
                    float dist = Distance(tf.Position, playerTf.Position);
                    
                    // State Switch: Chase if close, Prowl if far
                    isChasing = (dist < detectionRange);
                    
                    if (isChasing)
                    {
                        // Face the player
                        if (playerTf.Position.x > tf.Position.x) moveRight = true;
                        else moveRight = false;
                    }
                }
            }

            // 3. PROWLING LOGIC (If not chasing)
            if (!isChasing && isGrounded)
            {
                currentTurnTime -= dt;
                if (currentTurnTime <= 0)
                {
                    // Random turn intervals
                    moveRight = !moveRight;
                    currentTurnTime = turnTimer; 
                }
            }

            // 4. VISUALS
            // Flip Sprite Scale based on direction
            float absX = Math.Abs(tf.Size.x);
            if (moveRight) tf.Size = new CEVec2(absX, tf.Size.y);
            else           tf.Size = new CEVec2(-absX, tf.Size.y);
        }

        public override void FixedUpdate()
        {
            if (rb == null || isGrabbed) return;

            CEVec3 velocity = rb.LinearVelocity;
            
            // 1. APPLY MOVEMENT FORCE
            float dir = moveRight ? 1.0f : -1.0f;
            float speedMult = isChasing ? 1.5f : 1.0f; // Run faster when chasing

            // Only accelerate if under max speed
            if (Math.Abs(velocity.x) < maxSpeed * speedMult)
            {
                rb.AddForce(new CEVec3(moveSpeed * dir * speedMult, 0, 0));
            }
            
            // 2. HOPPING (Optional - makes slime feel jumpy)
            /* if (isGrounded && isChasing && Math.Abs(velocity.x) > 5.0f) {
                 // Small chance to hop while chasing
                 if (new Random().Next(0, 50) == 0) {
                     rb.AddForce(new CEVec3(0, jumpForce, 0));
                     isGrounded = false;
                 }
            }
            */

            // 3. CUSTOM GRAVITY (Fall faster for snappy feel)
            if (velocity.y < 0)
            {
                 // Apply extra gravity
                 rb.AddForce(new CEVec3(0f, -9.81f * gravityScale * rb.Mass, 0f));
            }
        }

        private void HandleGrabLogic()
        {
            // Right Click to Grab
            if (Input.IsMouseButtonDown(0)) // 1 = Left Click
            {
                CEVec2 mousePos = Input.GetMousePosition();
                
                // Simple bounding box check for mouse cursor
                float halfW = Math.Abs(tf.Size.x) * 0.5f + 20.0f; // + padding
                float halfH = Math.Abs(tf.Size.y) * 0.5f + 20.0f;

                bool mouseOver = mousePos.x >= tf.Position.x - halfW && 
                                  mousePos.x <= tf.Position.x + halfW &&
                                  mousePos.y >= tf.Position.y - halfH && 
                                  mousePos.y <= tf.Position.y + halfH;

                if (mouseOver)
                {
                    Debug.Log("GRAAAB");
                    isGrabbed = true;
                    rb.LinearVelocity = new CEVec3(0, 0, 0); // Stop physics
                    tf.Position = new CEVec3(mousePos.x, mousePos.y, tf.Position.z);
                }
            }
            else
            {
                if (isGrabbed)
                {
                    isGrabbed = false;
                    // Optional: Throw the slime when releasing?
                }
            }
        }

        // --- PHYSICS EVENTS ---

        public void OnTriggerEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            
            // Check for Ground
            LayerComponent layer = other.GetComponent<LayerComponent>();
            TransformComponent otherTf = other.GetComponent<TransformComponent>();

            if (layer != null && layer.HasLayer("Ground"))
            {
                // Check if we are landing on TOP of the ground
                if (otherTf != null && tf.Position.y > otherTf.Position.y + (otherTf.Size.y * 0.4f))
                {
                    isGrounded = true;
                }
                else
                {
                    // We hit a WALL (Side collision) -> Turn around
                    if (flipCooldown <= 0)
                    {
                        moveRight = !moveRight;
                        // Kill horizontal momentum immediately for snappy turn
                        rb.LinearVelocity = new CEVec3(0, rb.LinearVelocity.y, 0);
                        flipCooldown = 0.2f; // Prevent double-flip glitch
                    }
                }
            }
        }

        public void OnTriggerExit(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            LayerComponent layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Ground"))
            {
                isGrounded = false;
            }
        }
        
        // Unused callbacks required by bindings
        public void OnTriggerStay(uint e) {}
        public void OnCollideEnter(uint e) {}
        public void OnCollideStay(uint e) {}
        public void OnCollideExit(uint e) {}

        // Math Helper
        private float Distance(CEVec3 a, CEVec3 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return (float)Math.Sqrt(dx*dx + dy*dy);
        }
    }
}