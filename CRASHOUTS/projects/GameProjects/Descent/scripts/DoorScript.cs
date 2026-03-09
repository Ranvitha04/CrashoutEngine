/*!
@file       DoorScript.cs
@author     
@date       09/03/2026
@brief      Generic script to handle player interaction with level transition doors.
            When the player collides with or triggers the door, it loads the specified scene.

All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class DoorScript : ScriptBase
    {
        // --- Editor Exposed Variables ---
        
        [SerializeField] public string nextSceneName = "tutorial2";
        [SerializeField] public float transitionDelay = 2.0f;
        [SerializeField] private string doorParticleName = "Door Particles";
        
        // --- Internal State ---
        
        private bool hasTriggered = false;
        private uint doorParticleID = 0;

        public override void Start()
        {
            // Register both Trigger and Collision events to ensure we catch the player
            // regardless of whether the door collider is set to 'Trigger' or 'Solid'.
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);

            Entity dust = Entity.Find(doorParticleName);
            if (dust != null)
            {
                doorParticleID = dust.ID;
                InternalCalls.Particle_SetEmitting(doorParticleID, false);
            }
            else
            {
                Debug.Log($"[DoorScript] Note: No particle system named '{doorParticleName}' found. Skipping particles.");
            }
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            CheckForPlayer(otherEntityID);
        }

        public void OnCollideEnter(uint otherEntityID)
        {
            CheckForPlayer(otherEntityID);
        }

        private void CheckForPlayer(uint otherEntityID)
        {
            // If we already triggered the transition, ignore all further collisions
            if (hasTriggered) return;

            Entity other = new Entity(otherEntityID);

            // Check if it's the player
            var tag = other.GetComponent<TagComponent>();
            bool isPlayer = (tag != null && tag.HasTag("Player"));

            if (isPlayer)
            {
                // Lock the door so it can't be triggered again
                hasTriggered = true; 
                
                // Trigger visual feedback
                if (doorParticleID != 0) 
                {
                    InternalCalls.Particle_TriggerBurst(doorParticleID);
                }
                
                Debug.Log($"[DoorScript] Player detected! Loading scene: {nextSceneName}");
                
                // Load the dynamic scene with the dynamic delay
                SceneManager.LoadScene(nextSceneName, transitionDelay);
            }
        }
    }
}