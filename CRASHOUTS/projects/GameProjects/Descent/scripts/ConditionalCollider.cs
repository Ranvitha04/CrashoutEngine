/*!
@file       ConditionalCollider.cs
@author     Shahrin Idzuan
@date       13/03/2026
@brief      Script that conditionally allows or blocks collisions in PreSolve.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Uses collision tags to decide whether a collision should occur.
    */
    public class ConditionalCollider : ScriptBase
    {
        /*!
        @brief Register the PreSolve callback for this entity.
        */
        public override void Start()
        {
            // Register the PreSolve event to conditionally handle collisions.
            CollisionEvents.RegisterPreSolve(Entity.ID, this.OnPreSolve);
        }

        /*!
        @brief PreSolve callback to accept or reject collision resolution.
        @param otherEntity Entity id for the other collider.
        @return True to collide, false to ignore collision.
        */
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

        /*!
        @brief Update tick (unused for this behaviour).
        */
        public override void Update()
        {
            // Not used for this logic.
        }

        /*!
        @brief Fixed update tick (unused for this behaviour).
        */
        public override void FixedUpdate()
        {
            // Not used for this logic.
        }
    }
}
