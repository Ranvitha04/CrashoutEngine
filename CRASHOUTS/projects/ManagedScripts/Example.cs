using System;
using CrashoutsEngine;  // important so we can see ScriptBase, Entity, etc.

namespace CrashoutsEngine
{

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
            Run();
        }

        // Called at a fixed timestep (good for physics)
        public override void FixedUpdate()
        {
            Console.WriteLine("[C#] ExampleClass FixedUpdate()");
        }

        // Called when the script instance is being destroyed
        public override void OnDestroy()
        {
            Console.WriteLine("[C#] ExampleClass OnDestroy() called for Entity ID = " + Entity.ID);
        }

        // A custom method defined in this class
        public void Run()
        {
            Console.WriteLine("[C#] Instance method Run() called");
        }
    }
}