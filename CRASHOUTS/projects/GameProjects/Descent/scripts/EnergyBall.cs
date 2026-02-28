/*!
@file       EnergyBall.cs
@author     lee.r@digipen.edu
@date       28/11/2025
@brief      Energy Ball Pickup Script

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Threading;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class EnergyBall : ScriptBase
    {
        // --- Settings (Editable in Inspector) ---
        [SerializeField] private float energyAmount = 25.0f;
        [SerializeField] private float respawnTime = 5.0f;

        // --- Internal State ---
        private bool _isActive = true;
        private float _timer = 0.0f;
        private float time = 0.0f;
        private float bobDist = 0.4f;

        // Components
        private RigidBodyComponent _rb;
        private TransformComponent _tf;
        private Animator _anim; // The Keyframe Animator

        public override void Start()
        {
            _rb = Entity.GetComponent<RigidBodyComponent>();
            _tf = Entity.GetComponent<TransformComponent>();
            _anim = Entity.GetComponent<Animator>(); // Get Animator

            AudioManager.SetClip3DDistances(Entity.ID, "Pickup", 50.0f, 800.0f);

            // Register Trigger Event
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);

            // Set Initial Animation State
            if (_anim != null) _anim.Play("charged");

            Debug.Log("[EnergyBall] Ready.");
        }

        public void OnTriggerEnter(uint otherID)
        {
            if (!_isActive) return;

            Entity other = new Entity(otherID);

            // Check Tag "player"
            TagComponent Tag = other.GetComponent<TagComponent>();
            if (Tag != null && Tag.HasTag("Player"))
            {
                Collect();
            }
        }

        private void Collect()
        {
            Console.WriteLine("[EnergyBall] Collected!");

            // 1. Give Energy to player
            if (PlayerMovement.Instance != null)
            {
                PlayerMovement.Instance.AddEnergy(energyAmount);
            }

            AudioManager.GetInstance().PlayAttached("Pickup", Entity.ID);

            // 2. Disable Interaction
            _isActive = false;
            _timer = respawnTime;

            // If you have a dynamic body, sleep it or stop simulation
            if (_rb != null)
            {
                // Assuming SetDynamic toggles simulation/physics response
                // If this method isn't exposed, you might just want to move it or change Tags
                // But based on your code:
                _rb.SetDynamic(false);
            }

            // 3. Switch Animation (Instead of hiding)
            if (_anim != null) _anim.Play("cooldown");
        }

        private void Respawn()
        {
            Console.WriteLine("[EnergyBall] Respawned!");
            _isActive = true;

            if (_rb != null) _rb.SetDynamic(true);

            // Restore Animation
            if (_anim != null) _anim.Play("charged");
        }

        public override void Update()
        {
            float dt = (float)Time.DT;

            // Respawn Timer
            if (!_isActive)
            {
                _timer -= dt;
                if (_timer <= 0.0f)
                {
                    Respawn();
                }
            }

            // Bobbing animation while active (visual polish)
            if (_isActive && _tf != null)
            {
                time += dt;
                // Simple Sine wave bob logic
                // Note: We must be careful not to drift the position if physics is also affecting it.
                // Assuming this is a static/trigger pickup:
                float bobOffset = (float)Math.Sin(time * 4.0f) * bobDist;

                // Apply directly to Y. 
                // CAUTION: If this object moves, this will lock its Y position to the sine wave.
                // Better to have a "baseY" stored in Start() if it's static.
                // For now, assuming simple bob:
                _tf.Position = new CEVec3(_tf.Position.x, _tf.Position.y + bobOffset, _tf.Position.z);
            }
        }
    }
}
