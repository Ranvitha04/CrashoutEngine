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
        public CEVec3 Position
        {
            get
            {
                CEVec3 pos;
                InternalCalls.TransformGetPosition(_entityID, out pos);
                return pos;
            }
            set
            {
                InternalCalls.TransformSetPosition(_entityID, ref value);
            }
        }

        // Rotation (degrees)
        public float Rotation
        {
            get
            {
                float rot;
                InternalCalls.TransformGetRotation(_entityID, out rot);
                return rot;
            }
            set
            {
                InternalCalls.TransformSetRotation(_entityID, ref value);
            }
        }

        // Size
        public CEVec2 Size
        {
            get
            {
                CEVec2 size;
                InternalCalls.TransformGetSize(_entityID, out size);
                return size;
            }
            set
            {
                InternalCalls.TransformSetSize(_entityID, ref value);
            }
        }
    }
}
