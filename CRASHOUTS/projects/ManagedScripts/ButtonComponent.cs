using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public class ButtonComponent
    {
        public uint EntityID;

        internal ButtonComponent() { }

        public bool IsClicked()
        {
            return InternalIsClicked(EntityID);
        }

        public bool IsHovered()
        {
            return InternalIsHovered(EntityID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static bool InternalIsClicked(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static bool InternalIsHovered(uint entityID);
    }
}
