using System;

namespace CrashoutsEngine
{
    public class NameComponent
    {
        public uint EntityID { get; internal set; }

        internal NameComponent() { }

        public string Value
        {
            get => InternalCalls.GetName(EntityID);
            set => InternalCalls.SetName(EntityID, value);
        }

        public override string ToString() => Value;
    }
}
