/*!
@file       tutorial1door.cs
@author     shahrinidzuan.m@digipen.edu
@date       02/02/2026
@brief      Script to handle player interaction with the tutorial door in the first tutorial level.
            When the player collides with or triggers the door, it loads the next tutorial scene.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class tutorial1door : ScriptBase
    {
        // NEW: Gatekeeper boolean
        private bool hasTriggered = false;

        public override void Start()
        {
            // Register both Trigger and Collision events to ensure we catch the player
            // regardless of whether the door collider is set to 'Trigger' or 'Solid'.
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);
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
            // NEW: If we already triggered the transition, ignore all further collisions
            if (hasTriggered) return;

            Entity other = new Entity(otherEntityID);

            // Check if it's the player
            var tag = other.GetComponent<TagComponent>();
            bool isPlayer = (tag != null && tag.HasTag("Player"));

            if (isPlayer)
            {
                // Lock the door so it can't be triggered again
                hasTriggered = true; 
                
                // Console.WriteLine("[TutorialDoor] Player detected! Loading tutorial2...");
                SceneManager.LoadScene("tutorial2", 2f);
            }
        }
    }
}