using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PressurePlate : ScriptBase
    {

        public void OnTriggerEnter(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Player"))
            {
                Debug.Log("Hi");
            }
        }

        public void OnTriggerExit(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Player"))
            {
            }
        }

        public override void Start()
        {
            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
            CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);
        }

        public override void Update()
        {
            base.Update();
            //Debug.Log("test");
        }

        public override void FixedUpdate()
        {
            base.FixedUpdate();
        }
    }
}