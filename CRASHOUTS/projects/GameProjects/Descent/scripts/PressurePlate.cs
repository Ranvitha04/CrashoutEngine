/*!
@file       PressurePlate.cs
@author     Renfred Lee
@author     Renfred (RTIS)
@author     Mohamad Amir Akmal
@date       26/11/2025
@brief      Script behaviour for a pressure plate that can be activated by the player or enemies.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PressurePlate : ScriptBase
    {
        private bool stepOn = false;
        private float oldY, newY;
        [SerializeField] private float decrease = 5.0f;
        [SerializeField] private string goalTextName = null;
        
        private TransformComponent transform;
        private RigidBodyComponent rb;

        public string wallName = "MovingWall";
        private MovingWall _targetScript;

        // NEW: Thread-safety lock to prevent multithreaded double-triggers
        private readonly object activationLock = new object();

        void GetWall()
        {
            if (string.IsNullOrEmpty(wallName)) return;
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
            // Lock ensures only one collision event can process this block at a time
            lock (activationLock)
            {
                // If already pressed, don't calculate again
                if (stepOn) return;

                Entity other = new Entity(otherEntityID);
                var Tag = other.GetComponent<TagComponent>();

                if (Tag == null) return;

                bool isValidTrigger = false;

                // --- CHECK 1: IS IT AN ENEMY? ---
                if (Tag.HasTag("Enemy"))
                {
                    Debug.Log($"Enemy (ID: {otherEntityID}) stepped on plate.");
                    isValidTrigger = true;
                }
                // --- CHECK 2: IS IT THE PLAYER? ---
                else if (Tag.HasTag("Player"))
                {
                    var otherRb = other.GetComponent<RigidBodyComponent>();

                    if (otherRb != null && PlayerMovement.Instance != null)
                    {
                        if (otherRb.Mass >= (PlayerMovement.Instance.normalMass - 0.1f))
                        {
                            Debug.Log($"Player (ID: {otherEntityID} | Heavy) stepped on plate.");
                            isValidTrigger = true;
                        }
                        else
                        {
                            Debug.Log("Player (Light) stepped on plate - Not heavy enough!");
                        }
                    }
                }

                // --- EXECUTE ACTION ---
                if (isValidTrigger && !stepOn)
                {
                    // SET THIS IMMEDIATELY to prevent any recursive/subsequent triggers
                    stepOn = true; 

                    if(goalTextName != null && goalTextName.Length > 0)
                    {
                        PlayerMovement.Instance.currentGoalPlates += 1;
                        
                        // FIXED: Capture the uint ID directly instead of trying to cast to Entity
                        uint textEntityID = InternalCalls.FindEntityByName(goalTextName);
                        
                        InternalCalls.SetText(textEntityID, 
                            "Activated Pressure Plates: [" +
                            PlayerMovement.Instance.currentGoalPlates + "/" +
                            PlayerMovement.Instance.goalNoOfPlates + "]");
                    } 

                    transform.Position = new CEVec3(transform.Position.x, newY, transform.Position.z);
                    Debug.Log("Stepped on Pressure Plate!");

                    AudioManager.GetInstance().PlayAttached("Switch", Entity.ID);

                    if (_targetScript != null)
                    {
                        _targetScript.PlatesActive += 1;
                    }

                    if (rb != null) rb.SetDynamic(false);
                }
            }
        }

        public void OnTriggerExit(uint otherEntityID)
        {
        }

        public override void Start()
        {
            GetWall();

            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);

            AudioManager.SetClip3DDistances(Entity.ID, "Switch", 20.0f, 1000.0f);

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