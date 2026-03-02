using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class Checkpoint : ScriptBase
    {
        private CheckpointComponent cp;
        private TriggerMetadataComponent tm;

        public override void Start()
        {
            cp = Entity.GetComponent<CheckpointComponent>();
            tm = Entity.GetComponent<TriggerMetadataComponent>();

            AudioManager.SetClip3DDistances(Entity.ID, "checkpointSound", 50.0f, 1000.0f);

            // If it doesn't have metadata, add it for tracking
            if (tm == null)
            {
                Entity.Add<TriggerMetadataComponent>();
                tm = Entity.GetComponent<TriggerMetadataComponent>();
                tm.OneTimeUse = true;
            }

            // Register trigger events
            CollisionEvents.RegisterTriggerEnter(Entity.ID, this.OnTriggerEnter);
        }

        public void OnTriggerEnter(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var tag = other.GetComponent<TagComponent>();

            if (tag != null && tag.HasTag("Player"))
            {
                if (cp != null && !cp.Activated)
                {
                    // Update player's respawn point
                    if (PlayerMovement.Instance != null)
                    {
                        CEVec3 finalRespawnPos = cp.RespawnPosition;

                        // Robust Lookup Logic:
                        //Try finding by Name
                        if (!string.IsNullOrEmpty(cp.TargetEntityName))
                        {
                            Entity target = Entity.Find(cp.TargetEntityName);
                            if (target != null)
                            {
                                var targetTf = target.GetComponent<TransformComponent>();
                                if (targetTf != null) finalRespawnPos = targetTf.Position;
                            }
                        }
                        //Fallback to Runtime ID
                        else if (cp.TargetRespawnEntity != 0)
                        {
                            Entity target = new Entity(cp.TargetRespawnEntity);
                            var targetTf = target.GetComponent<TransformComponent>();
                            if (targetTf != null)
                            {
                                finalRespawnPos = targetTf.Position;
                            }
                        }

                        PlayerMovement.Instance.UpdateCheckpoint(finalRespawnPos);
                        cp.Activated = true;
                        
                        // Visual feedback (optional)
                        var anim = Entity.GetComponent<Animator>();
                        if (anim != null) anim.Play("checkpointActivated", false);
                        
                        AudioManager.GetInstance().PlayAttached("checkpointSound", Entity.ID);
                        //Console.WriteLine("[Checkpoint] Activated at " + cp.RespawnPosition);
                    }
                }
            }
        }
    }
}
