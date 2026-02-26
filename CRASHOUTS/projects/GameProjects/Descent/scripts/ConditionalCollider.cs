using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class ConditionalCollider : ScriptBase
    {
        public override void Start()
        {
            // Register the PreSolve event to conditionally handle collisions.
            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
        }

        public bool OnPreSolve(uint otherEntity)
        {
            Entity other = new Entity(otherEntity);
            var tag = other.GetComponent<TagComponent>();

            if (tag != null)
            {
                // Block collision with the "Player".
                if (tag.HasTag("Player"))
                {
                    return true; // true = collide
                }

                // Allow "Enemy" (like the Slime) to pass through.
                if (tag.HasTag("Enemy"))
                {
                    return false; // false = ignore collision
                }
            }

            // By default, collide with any other objects.
            return true;
        }

        public override void Update()
        {
            // Not used for this logic.
        }

        public override void FixedUpdate()
        {
            // Not used for this logic.
        }
    }
}
