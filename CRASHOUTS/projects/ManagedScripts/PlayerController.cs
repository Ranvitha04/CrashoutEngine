using System;
using System.Numerics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerController : ScriptBase
    {
        // -------- Fields --------
        RigidBodyComponent rb;
        TransformComponent tf;
        Animator animator;

        bool moveLeft;
        bool moveRight;
        bool isMoving;
        bool wasMoving;
        bool jump;

        bool isFacingLeft;     // for flip

        /*SAMPLE TRIGGERS*/
        public void OnTriggerEnter(uint otherEntity)
        {
            var rb = Entity.GetComponent<RigidBodyComponent>();
            // rb.AddForce(new CEVec3(0f, 20000f, 0f)); //* rb.Mass);
        }

        public void OnTriggerExit(uint otherEntity)
        {
            Console.WriteLine("Exited Me");
        }

        public void OnTriggerStay(uint otherEntity)
        {
            Console.WriteLine(otherEntity);
        }

        public override void Start()
        {
            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();

            // IMPORTANT: we don't rely on a native Animator component;
            // we just create a wrapper and tell it which Entity it controls.
            animator = new Animator();
            animator.EntityID = Entity.ID;

            if (rb == null)
                Console.WriteLine("[C#] WARNING: RigidBodyComponent is NULL on Player!");
            if (tf == null)
                Console.WriteLine("[C#] WARNING: TransformComponent is NULL on Player!");

            if (animator == null) // this should never be null now
            {
                Console.WriteLine("[C#] WARNING: Animator wrapper is NULL on Player! (No animation will play)");
            }
            else
            {
                // *** THIS WAS MISSING BEFORE ***
                // Give this entity a SpriteSheetComponent + texture + size
                animator.SetSpriteSheet(
                    "images/Character/Normal Mode/Chibi-Actions-SpriteSheet.png",
                    7,      // columns in your sheet
                    3,      // rows   in your sheet
                    2.0f,   // world width
                    2.0f    // world height
                );

                // Start in idle
                animator.Play("idle_normal");
                isFacingLeft = false;
            }

            Console.WriteLine("[C#] PlayerController Start() called, Entity is " + (Entity == null ? "NULL" : "OK!!"));
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);

            AudioManager.GetInstance().Play("ambient");
        }

        public override void Update()
        {
            if (tf == null)
                tf = Entity.GetComponent<TransformComponent>();

            // --- Input flags for movement ---
            moveLeft = Input.IsKeyPressed(Key.A);
            moveRight = Input.IsKeyPressed(Key.D);
            jump = Input.IsKeyTriggered(Key.Space);

            isMoving = moveLeft || moveRight;

            // --- Choose animation state + facing ---
            string desiredState = "idle_normal";

            if (isMoving)
            {
                desiredState = "walk_normal";

                // update facing based on last horizontal direction
                if (moveLeft && !moveRight)
                    isFacingLeft = true;
                else if (moveRight && !moveLeft)
                    isFacingLeft = false;
            }

            // --- Drive Animator (this is what actually animates!) ---
            if (animator != null)
            {
                animator.Play(desiredState);     // idle_normal / walk_normal
                animator.SetFlip(isFacingLeft);  // flip on A vs D
            }

            // --- Footstep audio ---
            if (isMoving && !wasMoving)
            {
                AudioManager.GetInstance().Play("footstep1");
            }
            else if (!isMoving && wasMoving)
            {
                AudioManager.GetInstance().Stop("footstep1");
            }

            // --- Size & rotation debug controls (same as before) ---
            if (tf != null)
            {
                if (Input.IsKeyPressed(Key.S))
                {
                    tf.Size *= 0.9f;
                }

                if (Input.IsKeyPressed(Key.Q))
                {
                    tf.Size *= 1.1f;
                }

                if (Input.IsKeyPressed(Key.F))
                {
                    tf.Rotation += 3f;

                    if (Entity.Has<ColliderComponent>())
                    {
                        Entity.GetComponent<ColliderComponent>().UpdateRotation(tf.Rotation);
                    }
                }
            }

            wasMoving = isMoving;
        }

        public override void FixedUpdate()
        {
            // do physics logic
            var rb = Entity.GetComponent<RigidBodyComponent>();
            if (rb == null) return;

            if (moveLeft)
            {
                rb.AddForce(new CEVec3(-500f, 0f, 0f));
            }

            if (moveRight)
            {
                rb.AddForce(new CEVec3(500f, 0f, 0f));
            }

            if (jump)
            {
                rb.AddForce(new CEVec3(0f, 20000f, 0f));
                Console.WriteLine("[C#] mass=" + rb.Mass);
            }
        }

        public override void OnDestroy()
        {
            Console.WriteLine("[C#] PlayerController OnDestroy() called");
            AudioManager.GetInstance().Stop("ambient");
        }
    }
}
