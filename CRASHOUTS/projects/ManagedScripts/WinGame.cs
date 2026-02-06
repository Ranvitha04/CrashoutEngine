/*!
@file       WinGame.cs
@author     lee.r@digipen.edu
@date       28/11/2025
@brief      Script to handle winning the game when the player enters a designated win zone.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class WinGame : ScriptBase
    {
        // The name of the UI entity to show when winning
        private string winScreenName = "WinScreen";
        private bool gameWon = false;

        public override void Start()
        {
            gameWon = false;
            // Register Trigger Event for this "Win Zone" entity
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);

            // Hide the Win Screen at start (just in case it was left visible in editor)
            Entity winScreen = Entity.Find(winScreenName);
            if (winScreen != null)
            {
                InternalCalls.SetActive(winScreen.ID, false);
            }
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            if (gameWon) return;

            Entity other = new Entity(otherEntityID);

            // Check if it's the player
            // Option A: Check Tag
            var Tag = other.GetComponent<TagComponent>();
            bool isplayer = (Tag != null && Tag.HasTag("Player"));

            // Option B: Check Name
            // var name = other.GetComponent<NameComponent>();
            // bool isplayer = (name != null && name.ToString() == "player");

            if (isplayer)
            {
                Win();
            }
        }

        private void Win()
        {
            Console.WriteLine("[WinGame] YOU WIN!");
            gameWon = true;

            // 1. Disable player Control
            if (PlayerMovement.Instance != null)
            {
                // If you have an 'Alive' or 'CanMove' flag
                // PlayerMovement.Instance.CanMove = false; 

                // Or just stop them
                var rb = PlayerMovement.Instance.Entity.GetComponent<RigidBodyComponent>();
                if (rb != null) rb.LinearVelocity = new CEVec3(0, 0, 0);
            }

            // 2. Show Win UI
            Entity winScreen = Entity.Find(winScreenName);
            if (winScreen != null)
            {
                InternalCalls.SetActive(winScreen.ID, true);
            }
            else
            {
                Console.WriteLine("[Error] WinGame could not find entity: " + winScreenName);
            }

            // Optional: Play Win Sound
            // AudioManager.GetInstance().Play("WinMusic");
        }
    }
}
