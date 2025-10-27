using System;
using System.Numerics;

namespace CrashoutsEngine
{
    public class Entity
    {
        public ulong ID { get; private set; }

        public Entity(ulong id)
        {
            ID = id;
        }

        // Check if this entity has a Transform component
        public bool HasTransform => InternalCalls.EntityHasTransform(ID);

        // Lazy-load a Transform object for this entity
        private Transform _transform;
        public Transform Transform => _transform ??= new Transform(ID);
    }
}
