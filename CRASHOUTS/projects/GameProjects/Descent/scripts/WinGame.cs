/*!
@file       WinGame.cs
@author     Ranvitha
@author     Shahrin Idzuan
@author     Renfred Lee
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
        [SerializeField] private string victorySceneName = "Victory";
        [SerializeField] private float victoryFadeDuration = 1.0f;
        private bool gameWon = false;

        [SerializeField] private string doorParticleName = "Door Particele";
        private uint doorParticleID = 0;

        public override void Start()
        {
            gameWon = false;
            // Register Trigger Event for this "Win Zone" entity
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            // Fallback for solid colliders
            CollisionEvents.RegisterCollideEnter(Entity.ID, OnCollideEnter);
            // Hide the Win Screen at start (just in case it was left visible in editor)
            Entity winScreen = Entity.Find(winScreenName);
            if (winScreen != null)
            {
                InternalCalls.SetActive(winScreen.ID, false);
            }

            Entity dust = Entity.Find(doorParticleName);
            if (dust != null)
            {
                doorParticleID = dust.ID;
                InternalCalls.Particle_SetEmitting(doorParticleID, false);
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

        public void OnCollideEnter(uint otherEntityID)
        {
            OnTriggerEnter(otherEntityID);
        }

        private void Win()
        {
            Console.WriteLine("[WinGame] YOU WIN!");
            gameWon = true;

            if (doorParticleID != 0) InternalCalls.Particle_TriggerBurst(doorParticleID);

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
            // Transition to victory scene
            if (!SceneManager.IsTransitioning())
            {
                SceneManager.LoadScene(victorySceneName, victoryFadeDuration);
            }

            // Optional: Play Win Sound
            // AudioManager.GetInstance().Play("WinMusic");
        }
    }
}
