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
                var tf = winScreen.GetComponent<TransformComponent>();
                if (tf != null)
                {
                    // Move it far away or set scale to 0 to hide
                    // Assuming Z = -100 hides it behind camera
                    tf.Position = new CEVec3(tf.Position.x, tf.Position.y, -100.0f);
                }
            }
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            if (gameWon) return;

            Entity other = new Entity(otherEntityID);

            // Check if it's the PTag
            // Option A: Check Tag
            var Tag = other.GetComponent<TagComponent>();
            bool isPTag = (Tag != null && Tag.HasTag("PTag"));

            // Option B: Check Name
            // var name = other.GetComponent<NameComponent>();
            // bool isPTag = (name != null && name.ToString() == "PTag");

            if (isPTag)
            {
                Win();
            }
        }

        private void Win()
        {
            Console.WriteLine("[WinGame] YOU WIN!");
            gameWon = true;

            // 1. Disable PTag Control
            if (PTagMovement.Instance != null)
            {
                // If you have an 'Alive' or 'CanMove' flag
                // PTagMovement.Instance.CanMove = false; 

                // Or just stop them
                var rb = PTagMovement.Instance.Entity.GetComponent<RigidBodyComponent>();
                if (rb != null) rb.LinearVelocity = new CEVec3(0, 0, 0);
            }

            // 2. Show Win UI
            Entity winScreen = Entity.Find(winScreenName);
            if (winScreen != null)
            {
                var tf = winScreen.GetComponent<TransformComponent>();
                if (tf != null)
                {
                    // Bring to front
                    tf.Position = new CEVec3(tf.Position.x, tf.Position.y, 0.5f);
                }
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