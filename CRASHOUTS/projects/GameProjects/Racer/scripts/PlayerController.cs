using System;
using CrashoutsEngine; // Adjust to match your C# namespace

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PlayerController : ScriptBase
    {
        // Tweak these in the Editor Inspector
        public float ThrustForce = 500.0f;
        public float RotationSpeed = 150.0f; // Degrees per second

        private TransformComponent _transform;
        private RigidBodyComponent _rigidBody;

        public override void Start()
        {
            // Cache the components so we don't have to search for them every frame
            _transform = Entity.GetComponent<TransformComponent>();
            _rigidBody = Entity.GetComponent<RigidBodyComponent>();
        }

        public override void Update() { }

        public override void FixedUpdate()
        {
            if (_transform == null || _rigidBody == null) return;

            // ==========================================
            // 1. ROTATION LOGIC (A and D)
            // ==========================================
            // Removed the (int) cast since your Input system handles the Key enum natively!
            if (Input.IsKeyHeld(Key.A)) 
            {
                // Assuming standard timestep of 1/60th of a second for FixedUpdate
                _transform.Rotation += RotationSpeed * (1.0f / 60.0f); 
            }
            else if (Input.IsKeyHeld(Key.D))
            {
                _transform.Rotation -= RotationSpeed * (1.0f / 60.0f);
            }

            // ==========================================
            // 2. DIRECTION MATH (Trigonometry)
            // ==========================================
            // C#'s Math.Sin and Cos expect Radians, so we convert your degrees
            float rotationRadians = _transform.Rotation * (float)(Math.PI / 180.0);

            // CALCULATE DIRECTION VECTOR:
            // * NOTE: This assumes your sprite points RIGHT (1, 0) at 0 degrees.
            // * If your sprite points UP (0, 1) at 0 degrees, swap these to:
            // * dirX = (float)-Math.Sin(rotationRadians);
            // * dirY = (float)Math.Cos(rotationRadians);
            float dirX = (float)Math.Cos(rotationRadians);
            float dirY = (float)Math.Sin(rotationRadians);

            // ==========================================
            // 3. MOVEMENT LOGIC (W and S)
            // ==========================================
            if (Input.IsKeyHeld(Key.W))
            {
                // Push Forward (Using CEVec3 with Z = 0f to match your physics engine)
                CEVec3 force = new CEVec3(dirX * ThrustForce, dirY * ThrustForce, 0f);
                _rigidBody.AddForce(force);
            }
            else if (Input.IsKeyHeld(Key.S))
            {
                // Push Backward (Reverse the vector)
                CEVec3 force = new CEVec3(-dirX * ThrustForce, -dirY * ThrustForce, 0f);
                _rigidBody.AddForce(force);
            }
        }
        
        public override void OnDestroy() { }
    }
}