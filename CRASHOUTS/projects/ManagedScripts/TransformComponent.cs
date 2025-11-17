using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddTransform", "RemoveTransform", "EntityHasTransform")]
    public class TransformComponent
    {
        // Must be named exactly what GetComponent<T>() expects
        public uint EntityID;

        public CEVec3 Position
        {
            get
            {
                InternalCalls.TransformGetPosition(EntityID, out CEVec3 pos);
                return pos;
            }
            set
            {
                InternalCalls.TransformSetPosition(EntityID, ref value);
            }
        }

        public float Rotation
        {
            get
            {
                InternalCalls.TransformGetRotation(EntityID, out float rot);
                return rot;
            }
            set
            {
                InternalCalls.TransformSetRotation(EntityID, ref value);
            }
        }

        public CEVec2 Size
        {
            get
            {
                InternalCalls.TransformGetSize(EntityID, out CEVec2 size);
                return size;
            }
            set
            {
                InternalCalls.TransformSetSize(EntityID, ref value);
            }
        }
    }
}
