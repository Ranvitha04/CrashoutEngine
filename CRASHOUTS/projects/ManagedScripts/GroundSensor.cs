//using System;

//namespace CrashoutsEngine
//{
//    [ScriptBehaviour]
//    public class GroundSensor : ScriptBase
//    {
//        // Public field to assign the parent Entity ID in the Editor (if you have property drawers)
//        // Or we can find it automatically.
//        public uint playerEntityID = 0;

//        // Internal reference
//        private PlayerMovement _playerScript = null;


//        public void OnTriggerEnter(uint otherEntityID)
//        {
//            // Ignore the player itself!
//            Debug.Log("Hi");
//            if (otherEntityID == playerEntityID) return;
//            Entity other = new Entity(otherEntityID);
//            var Tag = other.GetComponent<TagComponent>();

//            if (Tag != null && Tag.HasTag("Ground"))
//            {
//                Notifyplayer(true);
//            }
//        }

//        public void OnTriggerExit(uint otherEntityID)
//        {
//            if (otherEntityID == playerEntityID) return;

//            Entity other = new Entity(otherEntityID);
//            var Tag = other.GetComponent<TagComponent>();

//            if (Tag != null && Tag.HasTag("Ground"))
//            {
//                Notifyplayer(false);
//            }
//        }

//        private void Notifyplayer(bool isGrounded)
//        {
//            _playerScript = PlayerMovement.Instance;
//            if (PlayerMovement.Instance != null)
//            {
//                PlayerMovement.Instance.SetGrounded(isGrounded);
//            }
//        }

//        public override void Start()
//        {
//            // Option A: Auto-find Parent (If your engine supports hierarchical lookup in C#)
//            // If not, you can manually set playerEntityID in the editor.
//            // For now, let's assume you set playerEntityID manually or we find it via a tag/name.

//            // If Entity.ParentID is exposed in your C# Entity class:
//            // playerEntityID = Entity.ParentID; 

//            // Register Callbacks for THIS entity (the sensor)
//            CollisionEvents.RegisterTriggerEnter(Entity.ID, OnTriggerEnter);
//            CollisionEvents.RegisterTriggerExit(Entity.ID, OnTriggerExit);

//            Console.WriteLine("[C#] GroundSensor Started on Entity " + Entity.ID);
//        }

//        public override void Update()
//        {
//            base.Update();
//            //Debug.Log("test");
//        }

//        public override void FixedUpdate()
//        {
//            base.FixedUpdate();
//        }

//        public override void OnDestroy()
//        {


//        }
//    }
//}