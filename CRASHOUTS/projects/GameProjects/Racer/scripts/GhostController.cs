using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class GhostController : ScriptBase
    {
        // Make sure these match your PlayerController exactly!
        public float ThrustForce = 500.0f; // <-- Matched to your 500.0f in PlayerController
        public float RotationSpeed = 150.0f;
        public float SpaceDrag = 0.98f;

        // The name of the file to load (must match what LapTracker saves!)
        public string TrackName = "Level1";

        private TransformComponent _transform;
        private RigidBodyComponent _rigidBody;

        public override void Start()
        {
            _transform = Entity.GetComponent<TransformComponent>();
            _rigidBody = Entity.GetComponent<RigidBodyComponent>();

            if (_rigidBody != null)
            {
                if (_rigidBody.Mass <= 0.0f) _rigidBody.Mass = 1.0f;
                _rigidBody.GravityScale = 0.0f; 
            }

            // 1. Try to load the recording
            bool ghostLoaded = ReplayManager.LoadRecording($"{TrackName}_Ghost");

            // 2. If it failed (file doesn't exist), turn off the ghost and stop!
            if (!ghostLoaded)
            {
                InternalCalls.SetActive(Entity.ID, false);
                return; 
            }

            // 3. If it succeeded, start playback!
            ReplayManager.StartPlayback();
        }

        public override void FixedUpdate()
        {
            if (_transform == null || _rigidBody == null) return;

            // 1. ROTATION (Using Virtual Keys)
            if (ReplayManager.IsVirtualKeyPressed((int)Key.A) || ReplayManager.IsVirtualKeyPressed((int)Key.Left)) 
            {
                _rigidBody.AngularVelocity = RotationSpeed * (1.0f / 60.0f); 
            }
            else if (ReplayManager.IsVirtualKeyPressed((int)Key.D) || ReplayManager.IsVirtualKeyPressed((int)Key.Right))
            {
                _rigidBody.AngularVelocity = -RotationSpeed * (1.0f / 60.0f);
            }
            else
            {
                _rigidBody.AngularVelocity = 0f; 
            }

            // 2. DIRECTION MATH 
            float rotationRadians = _transform.Rotation * (float)(Math.PI / 180.0);
            float dirX = -(float)Math.Sin(rotationRadians);
            float dirY = (float)Math.Cos(rotationRadians);

            // 3. MOVEMENT (Using Virtual Keys)
            if (ReplayManager.IsVirtualKeyPressed((int)Key.W) || ReplayManager.IsVirtualKeyPressed((int)Key.Up))
            {
                CEVec3 force = new CEVec3(dirX * ThrustForce, dirY * ThrustForce, 0f);
                _rigidBody.AddForce(force);
            }
            else if (ReplayManager.IsVirtualKeyPressed((int)Key.S) || ReplayManager.IsVirtualKeyPressed((int)Key.Down))
            {
                CEVec3 force = new CEVec3(-dirX * ThrustForce, -dirY * ThrustForce, 0f);
                _rigidBody.AddForce(force);
            }
            
            // 4. APPLY SPACE DRAG
            _rigidBody.LinearVelocity = new CEVec3(
                _rigidBody.LinearVelocity.x * SpaceDrag, 
                _rigidBody.LinearVelocity.y * SpaceDrag, 
                0f
            );
        }
    }
}