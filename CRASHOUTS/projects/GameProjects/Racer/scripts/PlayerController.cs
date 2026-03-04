using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerController : ScriptBase
    {
        public float ThrustForce = 500.0f;
        public float RotationSpeed = 150.0f; 
        
        // Add a drag variable so they don't glide forever
        public float SpaceDrag = 0.98f; 
        private RigidBodyComponent _rigidBody;

        public override void Start()
        {
            _rigidBody = Entity.GetComponent<RigidBodyComponent>();

            if (_rigidBody != null)
            {
                // Make sure the player has mass to move!
                if (_rigidBody.Mass <= 0.0f) _rigidBody.Mass = 1.0f;
                
                // Turn off gravity for the top-down Asteroids feel
                // (Assuming you added the C# binding for GravityScale)
                _rigidBody.GravityScale = 0.0f; 
            }
        }

        public override void Update() { }

        public override void FixedUpdate()
        {
            if (_rigidBody == null) return;

            // GET THE LIVE TRANSFORM EVERY FRAME!
            var liveTransform = Entity.GetComponent<TransformComponent>();

            // 1. ROTATION (Using Physics)
            if (Input.IsKeyHeld(Key.A) || Input.IsKeyHeld(Key.Left)) 
                _rigidBody.AngularVelocity = RotationSpeed * (1.0f / 60.0f); 
            else if (Input.IsKeyHeld(Key.D) || Input.IsKeyHeld(Key.Right))
                _rigidBody.AngularVelocity = -RotationSpeed * (1.0f / 60.0f);
            else
                _rigidBody.AngularVelocity = 0f; 

            // 2. DIRECTION MATH (Using live rotation!)
            float rotationRadians = liveTransform.Rotation * (float)(Math.PI / 180.0);
            float dirX = -(float)Math.Sin(rotationRadians);
            float dirY = (float)Math.Cos(rotationRadians);

            // 3. MOVEMENT
            if (Input.IsKeyHeld(Key.W) || Input.IsKeyHeld(Key.Up))
                _rigidBody.AddForce(new CEVec3(dirX * ThrustForce, dirY * ThrustForce, 0f));
            else if (Input.IsKeyHeld(Key.S) || Input.IsKeyHeld(Key.Down))
                _rigidBody.AddForce(new CEVec3(-dirX * ThrustForce, -dirY * ThrustForce, 0f));
            
            // 4. APPLY SPACE DRAG
            _rigidBody.LinearVelocity = new CEVec3(
                _rigidBody.LinearVelocity.x * SpaceDrag, 
                _rigidBody.LinearVelocity.y * SpaceDrag, 
                0f
            );

            // 5. RECORD THE LIVE DATA
            ReplayManager.RecordTransform(liveTransform.Position, liveTransform.Rotation);
        }
        
        public override void OnDestroy() { }
    }
}