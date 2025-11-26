using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PressurePlate : ScriptBase
    {

        bool stepOn = false;
        float oldY, newY;
        const float decrease = 5.0f;
        TransformComponent transform;

        void MoveWallsOnSuccess()
        {
            // Find the wall (SORRY THIS IS HARDCODED, ALL WALLS MUST BE NAMED "wall_{pressure plate name}"
            // Example, this pressure plate is named "plate_1", the respective wall is named "wall_plate_1"
            string wallName = "wall_" + Entity.GetComponent<NameComponent>().ToString();
            Entity wall = Entity.Find(wallName);
        }

        public void OnTriggerEnter(uint otherEntityID)
        {
            Entity other = new Entity(otherEntityID);
            var layer = other.GetComponent<LayerComponent>();

            if (layer != null && layer.HasLayer("Player") && !stepOn)
            {
                transform.Position = new CEVec3(transform.Position.x, newY, transform.Position.z);
                Debug.Log("Stepped on Pressure Plate!");
                AudioManager.GetInstance().Play("pressureplate");
                Debug.Log("Played Sound");
                stepOn = true;

                MoveWallsOnSuccess();
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

            transform = Entity.GetComponent<TransformComponent>();
            oldY = transform.Position.y;
            newY = oldY - decrease;
            stepOn = false;
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