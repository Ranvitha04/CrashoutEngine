using System;
using System.Numerics;

namespace CrashoutsEngine
{
    public class PlayerController : ScriptBase
    {
        private float speed = 5.0f;

        public override void Start()
        {
            Console.WriteLine("[C#] PlayerController Start()");
        }

        public override void Update(float dt)
        {
            if (!Entity.HasTransform)
            {
                return;
            }
            var pos = Entity.Transform.Position;
            pos.X += speed * dt;
            Entity.Transform.Position = pos;
        }
    }
}
