using System;

namespace ExampleNamespace
{
    public class PlayerScript
    {
        private float timer = 0f;

        // Called once when the script instance is created
        public void Start()
        {
            Console.WriteLine("[C#] PlayerScript Start() called");
        }

        // Called every frame by your engine
        public void Update(double deltaTime)
        {
            {
                Console.WriteLine($"[C#] PlayerScript Update() - elapsed {deltaTime:F2}s");
            }
        }
    }
}
