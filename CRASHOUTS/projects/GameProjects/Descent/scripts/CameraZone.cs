/*!
@file       CameraZone.cs
@author     lee.r@digipen.edu
@date       01/03/2026
@brief      Handles camera zone for controlled camera behavior.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class CameraZone : ScriptBase
    {
        // Drag and drop the target entity directly in the inspector!
        public Entity lookAtEntity;
        
        // If true, leaving the zone will return the camera back to the player
        [SerializeField] public bool resetOnExit = false; 

        // NEW: Control how fast the camera transitions when entering THIS specific zone.
        // Lower number = Slower, more cinematic pan. (Your camera default is usually 4.0)
        [SerializeField] public float zoneDampingY = 1.5f; 

        public override void Start()
        {   
            if (lookAtEntity != null && lookAtEntity.ID != 0) lookAtEntity = new Entity(lookAtEntity.ID);
            
            // Register Physics Overlaps
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnZoneEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnZoneExit);
        }

        // --- HELPER METHOD TO CHECK IF ENTITY IS THE PLAYER ---
        private bool IsPlayerEntity(Entity e)
        {
            // 1. Check Tag
            var tag = e.GetComponent<TagComponent>();
            if (tag != null && tag.HasTag("Player")) 
                return true;

            // 2. Check Name (Assuming standard NameComponent C# wrapper)
            var nameComp = e.GetComponent<NameComponent>();
            if (nameComp != null && nameComp.Value == "Player") // (Change .Name to .name if your wrapper uses lowercase!)
                return true;

            // 3. Check for PlayerMovement Script
            var playerScript = e.GetComponent<PlayerMovement>();
            if (playerScript != null) 
                return true;

            return false;
        }

        public void OnZoneEnter(uint otherID)
        {
            Entity otherEntity = new Entity(otherID);
            
            // Replaced the old tag check with our new robust helper method
            if (IsPlayerEntity(otherEntity))
            {
                if (lookAtEntity != null && lookAtEntity.ID != 0)
                {
                    // Set the global target AND the custom speed!
                    CameraController.CurrentLookAtTarget = lookAtEntity;
                    CameraController.ActiveZoneDampingY = zoneDampingY; 
                }
            }
        }

        public void OnZoneExit(uint otherID)
        {
            if (resetOnExit)
            {
                Entity otherEntity = new Entity(otherID);
                
                if (IsPlayerEntity(otherEntity))
                {
                    // Remove the target so the camera goes back to normal
                    if (CameraController.CurrentLookAtTarget != null && 
                        CameraController.CurrentLookAtTarget.ID == lookAtEntity.ID)
                    {
                        CameraController.CurrentLookAtTarget = null;
                        CameraController.ActiveZoneDampingY = -1.0f; // Reset to default speed
                    }
                }
            }
        }
    }
}