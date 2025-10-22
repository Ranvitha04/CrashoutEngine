using System;

namespace ExampleNamespace
{
    public class ExampleClass
    {
        public ExampleClass()
        {
            Console.WriteLine("[C#] Constructor called!");
        }

        public void Run()
        {
            Console.WriteLine("[C#] Instance method called");
        }

        public static void Start()
        {
            Console.WriteLine("[C#] HELLO FROM OTHER SIDE");
        }
    }
}
