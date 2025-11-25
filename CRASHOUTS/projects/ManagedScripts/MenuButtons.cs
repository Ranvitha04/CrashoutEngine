using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class MenuButtons : ScriptBase
    {
        public override void Start()
        {

        }

        public override void Update()
        {
            Console.WriteLine("Test2");
            var btn = Entity.GetComponent<ButtonComponent>();
            if (btn.IsClicked())
            {
                Console.WriteLine("Clicked");
            }

        }

        public override void FixedUpdate()
        {
        }
    }
}