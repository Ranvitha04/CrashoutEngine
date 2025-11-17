using System;

namespace CrashoutsEngine
{
    [NativeComponent("AddName", "RemoveName", "EntityHasName")]
    public class NameComponent
    {
        public uint EntityID;

        internal NameComponent() { }

        public string Value
        {
            get => InternalCalls.GetName(EntityID);
            set => InternalCalls.SetName(EntityID, value);
        }

        public override string ToString() => Value;
    }
}
