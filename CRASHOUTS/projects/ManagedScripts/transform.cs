using System;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public class Transform
    {
        private readonly ulong _entityID;

        internal Transform(ulong entityID)
        {
            _entityID = entityID;
        }

        // Position
        public Vector3 Position
        {
            get
            {
                InternalCalls.TransformGetPosition(_entityID, out Vector3 position);
                return position;
            }
            set
            {
                InternalCalls.TransformSetPosition(_entityID, ref value);
            }
        }

        // Rotation in degrees
        public float Rotation
        {
            get
            {
                InternalCalls.TransformGetRotation(_entityID, out float rotation);
                return rotation;
            }
            set
            {
                InternalCalls.TransformSetRotation(_entityID, ref value);
            }
        }

        // Size
        public Vector2 Size
        {
            get
            {
                InternalCalls.TransformGetSize(_entityID, out Vector2 size);
                return size;
            }
            set
            {
                InternalCalls.TransformSetSize(_entityID, ref value);
            }
        }
    }
}
