/*!
@file       PressurePlate.cs
@author     lee.r@digipen.edu
@date       26/11/2025
@brief      Script behaviour for a pressure plate that can be activated by the player or enemies.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PressurePlate : ScriptBase
    {
        bool stepOn = false;
        float oldY, newY;
        [SerializeField] float decrease = 5.0f;
        [SerializeField] string goalTextName = null;
        TransformComponent transform;
        RigidBodyComponent rb; // The plate's RB

        public string wallName = "MovingWall";
        private MovingWall _targetScript;

        void GetWall()
        {
            if (wallName.Length <= 0) return;
            Entity wallEnt = Entity.Find(wallName);

            if (wallEnt != null)
            {
                _targetScript = wallEnt.GetScript<MovingWall>();
            }
            else
            {
                Console.WriteLine($"[Error] Plate could not find entity named: {wallName}");
            }
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            //Debug.Log("wweird");
            // If already pressed, don't calculate again
            if (stepOn) return;

            Entity other = new Entity(otherEntityID);
            var Tag = other.GetComponent<TagComponent>();

            if (Tag == null) return;

            bool isValidTrigger = false;

            // --- CHECK 1: IS IT AN ENEMY? ---
            if (Tag.HasTag("Enemy"))
            {
                Debug.Log("Enemy stepped on plate.");
                isValidTrigger = true;
            }
            // --- CHECK 2: IS IT THE player? ---
            else if (Tag.HasTag("Player"))
            {
                var otherRb = other.GetComponent<RigidBodyComponent>();

                // We need to access the player Singleton to know what "Normal" mass is.
                // We also check if the colliding object actually has a Rigidbody to read mass from.
                if (otherRb != null && PlayerMovement.Instance != null)
                {
                    // Check if current mass is greater than or equal to the Normal Mass defined in PlayerMovement
                    // (Using -0.1f buffer for floating point safety)
                    if (otherRb.Mass >= (PlayerMovement.Instance.normalMass - 0.1f))
                    {
                        Debug.Log("player (Heavy) stepped on plate.");
                        isValidTrigger = true;
                    }
                    else
                    {
                        Debug.Log("player (Light) stepped on plate - Not heavy enough!");
                    }
                }
            }

            // --- EXECUTE ACTION ---
            if (isValidTrigger)
            {
                if(goalTextName != null)
                {
                    // Get reference
                    // Add to player thing
                    PlayerMovement.Instance.currentGoalPlates++;
                    InternalCalls.SetText(InternalCalls.FindEntityByName(goalTextName), 
                        "Activated Pressure Plates: [" +
                        PlayerMovement.Instance.currentGoalPlates + "/" +
                        PlayerMovement.Instance.goalNoOfPlates + "]");
                } 

                transform.Position = new CEVec3(transform.Position.x, newY, transform.Position.z);
                Debug.Log("Stepped on Pressure Plate!");

                AudioManager.GetInstance().PlayAttached("Switch", Entity.ID);

                stepOn = true;

                if (_targetScript != null)
                {
                    _targetScript.PlatesActive++;
                }

                // Stop the plate from moving via physics since we hard-set position
                if (rb != null) rb.SetDynamic(false);
            }
        }

        public void OnTriggerExit(uint otherEntityID)
        {
            // Note: Currently your plate logic is "One Way" (it stays down).
            // If you want it to rise up when the player/enemy leaves, 
            // you would add logic here to decrement _targetScript.PlatesActive
            // and reset stepOn = false.
        }

        public override void Start()
        {
            GetWall();

            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);

            transform = Entity.GetComponent<TransformComponent>();

            if (transform != null)
            {
                oldY = transform.Position.y;
                newY = oldY - decrease;
            }

            stepOn = false;

            rb = Entity.GetComponent<RigidBodyComponent>();
            if (rb != null) rb.SetDynamic(true);
        }

        public override void Update()
        {
            base.Update();
        }

        public override void FixedUpdate()
        {
            base.FixedUpdate();
        }

        public override void OnDestroy()
        {

        }
    }
}
