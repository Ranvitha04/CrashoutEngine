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

        // Components
        private RigidBodyComponent _rb;
        private TransformComponent _tf;
        //private ShapeComponent _shape; // To hide visuals

        public override void Start()
        {
            _rb = Entity.GetComponent<RigidBodyComponent>();
            _tf = Entity.GetComponent<TransformComponent>();
            //_shape = Entity.GetComponent<ShapeComponent>();

            // Register Trigger Event
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);

            Console.WriteLine("[EnergyBall] Ready.");
        }

        public void OnTriggerEnter(uint otherID)
        {
            if (!_isActive) return;

            Entity other = new Entity(otherID);

            // Check Layer "Player"
            LayerComponent layer = other.GetComponent<LayerComponent>();
            if (layer != null && layer.HasLayer("Player"))
            {
                Collect();
            }
        }

        private void Collect()
        {
            Console.WriteLine("[EnergyBall] Collected!");

            // 1. Give Energy to Player
            if (PlayerMovement.Instance != null)
            {
                PlayerMovement.Instance.AddEnergy(energyAmount);
            }

            // 2. Disable Interaction
            _isActive = false;
            _timer = respawnTime;
            _rb.SetDynamic(_isActive);

            // 3. Disable Visuals (Hide)
            // if (_shape != null) 
            // {
            //     // Hack: Move z-index far away or set alpha 0 if you have color control
            //     // Better: Implement ShapeComponent.Enabled in C++
            //     // For now, let's just shrink it to 0 or move it away?
            //     // Let's move it to Z = -1000 (behind camera)
            //     if (_tf != null) _tf.Position = new CEVec3(_tf.Position.x, _tf.Position.y, -1000.0f);
            // }

            // 4. Disable Physics (if it has a body)
            // If it's a static trigger, we don't need to do much.
            // But if it's dynamic, we might want to sleep it.
        }

        private void Respawn()
        {
            Console.WriteLine("[EnergyBall] Respawned!");
            _isActive = true;
            _rb.SetDynamic(_isActive);
            // Restore Visuals
            //if (_tf != null) _tf.Position = new CEVec3(_tf.Position.x, _tf.Position.y, 0.1f); // Restore Z
        }

        public override void Update()
        {
            // Respawn Timer
            if (!_isActive)
            {
                _timer -= (float)Time.DT;
                if (_timer <= 0.0f)
                {
                    Respawn();
                }
            }
            time += (float)Time.DT;
            // Optional: Bobbing animation while active
            if (_isActive && _tf != null)
            {
               // Simple Sine wave bob
               float posY = _tf.Position.y;
               posY += (float)Math.Sin(time * 4.0f) * 0.15f;
               _tf.Position = new CEVec3(_tf.Position.x, posY, _tf.Position.z);

            }
        }
    }
}