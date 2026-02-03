using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class tutorial1door : ScriptBase
    {
        public override void Start()
        {
            //Console.WriteLine("[TutorialDoor] Script Initialized on Entity ID: " + Entity.ID);
            
            // Register both Trigger and Collision events to ensure we catch the player
            // regardless of whether the door collider is set to 'Trigger' or 'Solid'.
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            //Console.WriteLine("[TutorialDoor] Trigger Enter detected with Entity ID: " + otherEntityID);
            CheckForPlayer(otherEntityID);
        }

        public void OnCollideEnter(uint otherEntityID)
        {
            //Console.WriteLine("[TutorialDoor] Collision Enter detected with Entity ID: " + otherEntityID);
            CheckForPlayer(otherEntityID);
        }

        private void CheckForPlayer(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);

            // Check if it's the player
            var tag = other.GetComponent<TagComponent>();
            bool isPlayer = (tag != null && tag.HasTag("Player"));

            if (isPlayer)
            {
                //Console.WriteLine("[TutorialDoor] Player detected! Loading tutorial2...");
                SceneManager.LoadScene("tutorial2",2f);
            }
            // else
            // {
            //      Console.WriteLine("[TutorialDoor] Object is not player. Tag: " + (tag != null ? "Found" : "Null"));
            // }
        }
    }
}