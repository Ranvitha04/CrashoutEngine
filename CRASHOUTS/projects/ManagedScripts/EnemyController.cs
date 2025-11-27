using System;
using System.Numerics;
namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class EnemyController : ScriptBase
    {

        public static void OnCollideEnter(uint otherEntity) {
            Console.WriteLine("Entered Inside Me");
        }

        public static void OnCollideExit(uint otherEntity)
        {
            Console.WriteLine("Exited Me");
        }

        public static void OnCollideStay(uint otherEntity)
        {
            //Console.WriteLine("Staying in Me");
        }

        /*SAMPLE TRIGGERS*/
        public static void OnTriggerEnter(uint otherEntity) {
            Console.WriteLine("Entered Inside Me");
        }

        public static void OnTriggerExit(uint otherEntity)
        {
            Console.WriteLine("Exited Me");
        }

        public static void OnTriggerStay(uint otherEntity)
        {
            // Console.WriteLine(otherEntity);
        }

        public override void Start()
        {
            // Initialization logic for the enemy
            Console.WriteLine("[C#] EnemyController Start() called, Entity is " + (Entity == null ? "NULL" : "OK!"));
            
            // CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);
            // CollisionEvents.RegisterCollideExit(Entity.ID, OnCollideExit);
            // CollisionEvents.RegisterCollideStay(Entity.ID, OnCollideStay);

            // CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            // CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);
            // CollisionEvents.RegisterTriggerStay(Entity.ID, OnTriggerStay);
        }

        public override void Update()
        {
            //Console.WriteLine("[C#] PlayerController Update() called with dt: " + dt);
            // Access physics and transform components from your entity
            var rb = Entity.GetComponent<RigidBodyComponent>();
            var tf = Entity.GetComponent<TransformComponent>();
            if (rb == null) Console.WriteLine("[C#] rb is null");
            if (tf == null) Console.WriteLine("[C#] tf is null");


            if (Input.IsKeyPressed(Key.Left))
            {
                //Console.WriteLine("A key is pressed");
                //Console.WriteLine("[C#] mass=" + rb.Mass);
                rb.AddForce(new CEVec3(-100f, 0f, 0f));
            }

            if (Input.IsKeyPressed(Key.Right))
            {
                rb.AddForce(new CEVec3(100f, 0f, 0f));
            }

            if (Input.IsKeyPressed(Key.Up))
            {
                rb.AddForce(new CEVec3(0f, 300f, 0f) * rb.Mass);
            }
        }

        public override void FixedUpdate()
        {
            // Physics update logic for the enemy
        }   

        public override void OnDestroy()
        {
            // Cleanup logic for the enemy
        }
    }
}