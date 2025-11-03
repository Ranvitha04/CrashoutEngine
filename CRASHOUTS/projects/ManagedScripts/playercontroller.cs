using System;
using System.Numerics;
namespace CrashoutsEngine
{
    public class PlayerController : ScriptBase
    {
        public override void Start()
        {
            // Initialization logic for the player
            Console.WriteLine("[C#] PlayerController Start() called, Entity is " + (Entity == null ? "NULL" : "OK!"));

        }

        public override void Update(float dt)
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
                rb.AddForce(new Vector3(-100f, 0f, 0f));
            }

            if (Input.IsKeyPressed(Key.D))
            {
                rb.AddForce(new Vector3(100f, 0f, 0f));
            }

            if (Input.IsKeyPressed(Key.W))
            {
                rb.AddForce(new Vector3(0f, 300f, 0f) * rb.GetMass());
            }

            if (Input.IsKeyPressed(Key.S))
            {
                tf.Size *= 0.9f;
            }

            if (Input.IsKeyPressed(Key.Q))
            {
                tf.Size *= 1.1f;
            }

            if (Input.IsKeyPressed(Key.R))
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