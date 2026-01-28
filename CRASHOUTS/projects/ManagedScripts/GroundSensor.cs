//using System;

//namespace CrashoutsEngine
//{
//    [ScriptBehaviour]
//    public class GroundSensor : ScriptBase
//    {
//        // Public field to assign the parent Entity ID in the Editor (if you have property drawers)
//        // Or we can find it automatically.
//        public uint PTagEntityID = 0;

//        // Internal reference
//        private PTagMovement _pTagScript = null;


//        public void OnTriggerEnter(uint otherEntityID)
//        {
//            // Ignore the pTag itself!
//            Debug.Log("Hi");
//            if (otherEntityID == PTagEntityID) return;
//            Entity other = new Entity(otherEntityID);
//            var Tag = other.GetComponent<TagComponent>();

//            if (Tag != null && Tag.HasTag("Ground"))
//            {
//                NotifyPTag(true);
//            }
//        }

//        public void OnTriggerExit(uint otherEntityID)
//        {
//            if (otherEntityID == PTagEntityID) return;

//            Entity other = new Entity(otherEntityID);
//            var Tag = other.GetComponent<TagComponent>();

//            if (Tag != null && Tag.HasTag("Ground"))
//            {
//                NotifyPTag(false);
//            }
//        }

//        private void NotifyPTag(bool isGrounded)
//        {
//            _pTagScript = PTagMovement.Instance;
//            if (PTagMovement.Instance != null)
//            {
//                PTagMovement.Instance.SetGrounded(isGrounded);
//            }
//        }

//        public override void Start()
//        {
//            // Option A: Auto-find Parent (If your engine supports hierarchical lookup in C#)
//            // If not, you can manually set PTagEntityID in the editor.
//            // For now, let's assume you set PTagEntityID manually or we find it via a tag/name.

//            // If Entity.ParentID is exposed in your C# Entity class:
//            // PTagEntityID = Entity.ParentID; 

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