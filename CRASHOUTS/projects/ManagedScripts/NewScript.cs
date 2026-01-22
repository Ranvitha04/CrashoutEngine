using CrashoutsEngine;

namespace CrashoutsEngine
{
    // Attach this script to an Entity through the ScriptComponent UI.
    // Scripts with [ScriptBehaviour] will appear in the editor's script list.
    [ScriptBehaviour]
    public class NewScript
    {
        // Reference to the entity this script instance is attached to.
        // Automatically assigned by the engine before Start() runs.
        public Entity Entity;

        // Called once when the script is first created.
        // Initialize references or setup values here.
        public void Start() { }

        // Called every frame.
        // Main game logic, movement, inputs, and updates go here.
        public void Update() { }

        // Called at a fixed timestep (ideal for physics logic).
        // Runs independently from the frame rate.
        public void FixedUpdate() { }
    }
}
