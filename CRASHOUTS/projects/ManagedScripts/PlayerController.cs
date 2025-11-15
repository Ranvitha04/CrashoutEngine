using System;
using System.Numerics;
namespace CrashoutsEngine
{
    public class PlayerController : ScriptBase
    {
        /*SAMPLE TRIGGERS*/
        public void OnTriggerEnter(uint otherEntity) {
            Console.WriteLine("Entered Inside Me BOUNCE");
            var rb = Entity.RigidBody;
            rb.AddForce(new Vector3(0f, 20000f, 0f)); //* rb.GetMass());
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
            Console.WriteLine("[C#] PlayerController Start() called, Entity is " + (Entity == null ? "NULL" : "OK!"));
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
            // CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);
            // CollisionEvents.RegisterTriggerStay(Entity.ID, OnTriggerStay);
        }

        public override void Update()
        {
            //Console.WriteLine("[C#] PlayerController Update() called with dt: " + dt);
            // Access physics and transform components from your entity
            var rb = Entity.RigidBody;
            var tf = Entity.Transform;
            if (rb == null) Console.WriteLine("[C#] rb is null");
            if (tf == null) Console.WriteLine("[C#] tf is null");


            if (Input.IsKeyPressed(Key.A))
            {
                //Console.WriteLine("A key is pressed");
                //Console.WriteLine("[C#] mass=" + rb.GetMass());
                rb.AddForce(new Vector3(-500f, 0f, 0f));
            }

            if (Input.IsKeyPressed(Key.D))
            {
                rb.AddForce(new Vector3(500f, 0f, 0f));
            }

            if (Input.IsKeyTriggered(Key.Space))
            {
                rb.AddForce(new Vector3(0f, 40000f, 0f)); //* rb.GetMass());
                Console.WriteLine("[C#] mass=" + rb.GetMass());
            }

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
                if (Entity.HasCollider)
                {
                    Entity.Collider.UpdateRotation(tf.Rotation);
                }
            }
        }
    }
}