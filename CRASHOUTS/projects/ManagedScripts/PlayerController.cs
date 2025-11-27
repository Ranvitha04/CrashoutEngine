using System;
using System.Numerics;
namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerController : ScriptBase
    {
        bool moveLeft;
        bool moveRight;
        bool isMoving;
        bool wasMoving;
        bool jump;

        private RigidBodyComponent rb;
        private TransformComponent tf;
        private Animator animator;

        private string currentAnimState = "";
        private bool isAlt = false;
        /*SAMPLE TRIGGERS*/
        public void OnTriggerEnter(uint otherEntity) {
            //Console.WriteLine("Entered Inside Me BOUNCE");
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

            Console.WriteLine("[C#] PlayerController Start() called, Entity is " + (Entity == null ? "NULL" : "OK!!"));


            rb = Entity.GetComponent<RigidBodyComponent>();
            tf = Entity.GetComponent<TransformComponent>();
            animator = Entity.GetComponent<Animator>();

            if (rb == null)
                Console.WriteLine("[C#] WARNING: RigidBodyComponent is NULL on Player!");

            if (tf == null)
                Console.WriteLine("[C#] WARNING: TransformComponent is NULL on Player!");

            if (animator == null)
                Console.WriteLine("[C#] WARNING: Animator component is NULL on Player! (No animation will play)");

            // Register trigger callbacks
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            //CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnTriggerExit);
            //CollisionEvents.RegisterTriggerStay(Entity.ID, this.OnTriggerStay);

            // Start ambient audio
            AudioManager.GetInstance().Play("ambient");
        }

        public override void Update()
        {
            moveLeft = Input.IsKeyPressed(Key.A);
            moveRight = Input.IsKeyPressed(Key.D);
            jump = Input.IsKeyTriggered(Key.Space);

            isMoving = moveLeft || moveRight;
            //Console.WriteLine("Has RB? " + Entity.Has<RigidBodyComponent>());

            //Console.WriteLine("[C#] PlayerController Update() called with dt: " + dt);
            // Access physics and transform components from your entity
            //var rb = Entity.GetComponent<RigidBodyComponent>();
            var tf = Entity.GetComponent<TransformComponent>();
            //if (rb == null) Console.WriteLine("[C#] rb is null");
            if (tf == null) Console.WriteLine("[C#] tf is null");
            if (Input.IsKeyTriggered(Key.Z))
            {
                isAlt=!isAlt;
            }
            if (isMoving && !wasMoving)
            {
                //Console.WriteLine("[C#] Player started moving -> Play footsteps");
                AudioManager.GetInstance().Play("footsteps");
            }
            else if (!isMoving && wasMoving)
            {
                //Console.WriteLine("[C#] Player stopped moving -> Stop footsteps_loop");
                AudioManager.GetInstance().Stop("footsteps");
            }


            if (animator != null)
            {
                // flip
                if (moveLeft && !moveRight)
                    animator.SetFlip(true);
                else if (moveRight && !moveLeft)
                    animator.SetFlip(false);

                // pick animation based on isAlt + movement
                string desiredState;
                if (isAlt)
                    desiredState = isMoving ? "walk_light" : "idle_light";  // <- your light sprites
                else
                    desiredState = isMoving ? "walk_normal" : "idle_normal";

                if (desiredState != currentAnimState)
                {
                    Console.WriteLine("[C#] Animator.Play -> " + desiredState);
                    animator.Play(desiredState);
                    currentAnimState = desiredState;
                }
            }

            wasMoving = isMoving;
            //if (Input.IsKeyPressed(Key.A))
            //{
            //    //Console.WriteLine($"[DEBUG] A pressed, rb ID = {rb.EntityID}, mass = {rb.Mass}");
            //    //Console.WriteLine("A key is pressed");
            //    //Console.WriteLine("[C#] mass=" + rb.Mass);
            //    rb.AddForce(new CEVec3(-500f, 0f, 0f));
            //}

            //if (Input.IsKeyPressed(Key.D))
            //{
            //    rb.AddForce(new CEVec3(500f, 0f, 0f));
            //}

            //if (Input.IsKeyTriggered(Key.Space))
            //{
            //    rb.AddForce(new CEVec3(0f, 20000f, 0f)); //* rb.Mass);
            //    Console.WriteLine("[C#] mass=" + rb.Mass);
            //}

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
            /*
            */
         

        }

        public override void FixedUpdate()
        {
            // do physics logic
            var rb = Entity.GetComponent<RigidBodyComponent>();
            if (rb == null) return;

            // Retrieve current horizontal speed
            //double xSpeed = rb.getVelocity().x; 
            //Console.WriteLine("[C#] speedx=" + xSpeed);
            // Apply physics here (fixed 60hz)
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
            // Cleanup logic for the player
            Console.WriteLine("[C#] PlayerController OnDestroy() called");
            AudioManager.GetInstance().Stop("ambient");
        }
    }
}

