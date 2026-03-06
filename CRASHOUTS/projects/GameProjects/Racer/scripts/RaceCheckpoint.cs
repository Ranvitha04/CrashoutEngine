using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class RaceCheckpoint : ScriptBase
    {
        // Set this per-entity in your editor!
        public int CheckpointID = 0; 
        
        public override void Start()
        {
            // Optional: Setup audio just like your old script
            AudioManager.SetClip3DDistances(Entity.ID, "checkpointSound", 50.0f, 1000.0f);

            // Register the trigger event using your engine's specific API
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
        }

        public void OnTriggerEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var tag = other.GetComponent<TagComponent>();

            // Make sure only the Player triggers the checkpoint (ignore asteroids/missiles)
            if (tag != null && tag.HasTag("Player"))
            {
                if (LapTracker.Instance != null)
                {
                    Debug.Log("Hit checkpoint " + CheckpointID);
                    // Report the hit to the player's lap tracker
                    LapTracker.Instance.CheckpointHit(CheckpointID);

                    // Visual / Audio feedback
                    var anim = Entity.GetComponent<Animator>();
                    if (anim != null) anim.Play("checkpointFlash", false);
                    
                    AudioManager.GetInstance().PlayAttached("checkpointSound", Entity.ID);
                }
            }
        }
    }
}