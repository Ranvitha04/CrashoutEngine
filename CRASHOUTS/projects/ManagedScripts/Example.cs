using System;
using CrashoutsEngine;  // important so we can see ScriptBase, Entity, etc.

namespace CrashoutsEngine
{
    // This makes the script show up in your ScriptComponent picker
    [ScriptBehaviour]
    public class ExampleClass : ScriptBase
    {
        // Called once when the script instance is created
        public override void Start()
        {
            Console.WriteLine("[C#] ExampleClass Start() called, Entity ID = " + Entity.ID);
        }

        // Called every frame
        public override void Update()
        {
            Console.WriteLine("[C#] ExampleClass Update() running...");

            // You can still keep your old 'Run' idea as a helper:
            Run();
        }

        // Called at a fixed timestep (good for physics)
        public override void FixedUpdate()
        {
            // For now, just log so you know it’s working
            // Console.WriteLine("[C#] ExampleClass FixedUpdate()");
        }

        // Your original idea, now just a normal instance method
        public void Run()
        {
            Console.WriteLine("[C#] Instance method Run() called");
        }
    }
}