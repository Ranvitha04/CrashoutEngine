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
        // Type the name of the empty entity you placed in the level for the camera to look at
        [SerializeField] public string lookAtObjectName = "Zone1_Target";
        
        // If true, leaving the zone will return the camera back to the player
        [SerializeField] public bool resetOnExit = false; 

        private Entity lookAtEntity;

        public override void Start()
        {
            lookAtEntity = Entity.Find(lookAtObjectName);
            
            // Register Physics Overlaps
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnZoneEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, this.OnZoneExit);
        }

        public void OnZoneEnter(uint otherID)
        {
            var tag = new Entity(otherID).GetComponent<TagComponent>();
            if (tag != null && tag.HasTag("Player"))
            {
                // Set the global target!
                CameraController.CurrentLookAtTarget = lookAtEntity;
            }
        }

        public void OnZoneExit(uint otherID)
        {
            if (resetOnExit)
            {
                var tag = new Entity(otherID).GetComponent<TagComponent>();
                if (tag != null && tag.HasTag("Player"))
                {
                    // Remove the target so the camera goes back to normal
                    if (CameraController.CurrentLookAtTarget == lookAtEntity)
                    {
                        CameraController.CurrentLookAtTarget = null;
                    }
                }
            }
        }
    }
}