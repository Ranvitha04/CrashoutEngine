using System;

namespace CrashoutsEngine
{
    public abstract class ScriptBase
    {
        // Set by the engine before Start() is called
        public Entity Entity;

        // Called once when the script instance is created
        public virtual void Start() { }

        // Called once per frame
        public virtual void Update() { }

        // Called at a fixed timestep
        public virtual void FixedUpdate() { }
    }
}
