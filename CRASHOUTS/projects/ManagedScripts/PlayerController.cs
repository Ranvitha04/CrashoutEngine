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
        Animator animator;
        /*SAMPLE TRIGGERS*/
        public void OnTriggerEnter(uint otherEntity)
        {
            //Console.WriteLine("Entered Inside Me BOUNCE");
            var rb = Entity.GetComponent<RigidBodyComponent>();
            // rb.AddForce(new CEVec3(0f, 20000f, 0f)); //* rb.GetMass());
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
            // Initialization logic for the player
            Console.WriteLine("[C#] PlayerController Start() called, Entity is " + (Entity == null ? "NULL" : "OK!!"));
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            //Entity.Add<RigidBodyComponent>();
            //Entity.Add<TransformComponent>();
            // CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);
            // CollisionEvents.RegisterTriggerStay(Entity.ID, OnTriggerStay);

            AudioManager.GetInstance().Play("ambient");

                              
            animator = Entity.GetComponent<Animator>(); 

          //  if (animator == null)
          //  {
          //      Console.WriteLine("[C#] PlayerController: FAILED to get Animator on entity " + Entity.ID);
           //     return;
           // }

           // animator.SetSpriteSheet(
            //    "images/spritesheet/ducky_2_spritesheet.png",
            //    6,   // cols
             //   4,   // rows
             //   10.0f,
             //   10.0f
           // );
           
        }

        public override void Update()
        {
            //Console.WriteLine("Has RB? " + Entity.Has<RigidBodyComponent>());

            //Console.WriteLine("[C#] PlayerController Update() called with dt: " + dt);
            // Access physics and transform components from your entity
            //var rb = Entity.GetComponent<RigidBodyComponent>();
            var tf = Entity.GetComponent<TransformComponent>();
            //if (rb == null) Console.WriteLine("[C#] rb is null");
            if (tf == null) Console.WriteLine("[C#] tf is null");
            isMoving = moveLeft || moveRight;
            if (isMoving && !wasMoving)
            {
                //Console.WriteLine("[C#] Player started moving -> Play footsteps");
                AudioManager.GetInstance().Play("footstep1");
            }
            else if (!isMoving && wasMoving)
            {
                //Console.WriteLine("[C#] Player stopped moving -> Stop footsteps_loop");
                AudioManager.GetInstance().Stop("footstep1");
            }
            //if (Input.IsKeyPressed(Key.A))
            //{
            //    //Console.WriteLine($"[DEBUG] A pressed, rb ID = {rb.EntityID}, mass = {rb.GetMass()}");
            //    //Console.WriteLine("A key is pressed");
            //    //Console.WriteLine("[C#] mass=" + rb.GetMass());
            //    rb.AddForce(new CEVec3(-500f, 0f, 0f));
            //}

            //if (Input.IsKeyPressed(Key.D))
            //{
            //    rb.AddForce(new CEVec3(500f, 0f, 0f));
            //}

            //if (Input.IsKeyTriggered(Key.Space))
            //{
            //    rb.AddForce(new CEVec3(0f, 20000f, 0f)); //* rb.GetMass());
            //    Console.WriteLine("[C#] mass=" + rb.GetMass());
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
            moveLeft = Input.IsKeyPressed(Key.A);
            moveRight = Input.IsKeyPressed(Key.D);

           // if (animator != null)
           // {
           //     bool moving = moveLeft || moveRight;
           //     if (!moving)
            //        animator.Play("idle_normal");
            //    else
            //    {
              //      animator.Play("walk_normal");
              //      if (moveLeft && !moveRight)
               //         animator.SetFlip(true);
                //    else if (moveRight && !moveLeft)
               //         animator.SetFlip(false);
               // }
           // }
           // wasMoving = isMoving;
            //jump = Input.IsKeyTriggered(Key.Space);

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