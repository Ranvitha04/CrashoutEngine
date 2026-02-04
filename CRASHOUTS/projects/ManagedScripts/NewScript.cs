using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class NewScript : ScriptBase
    {
        public Entity Entity;

        public override void Start() 
        { 
            // Called once on start
        }

        public override void Update() 
        { 
            // Called every frame
        }

        public override void FixedUpdate() 
        { 
            // Called every physics step
        }
    }
}
