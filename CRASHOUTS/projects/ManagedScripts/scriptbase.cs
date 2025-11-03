using System;

namespace CrashoutsEngine
{
    public abstract class ScriptBase
    {
        // Set by the engine before Start() is called
        public Entity Entity;

        // Called once when the script instance is created
        public virtual void Start() { }

        // Called once per frame with deltaTime (in seconds)
        public virtual void Update(float dt) { }
    }
}
