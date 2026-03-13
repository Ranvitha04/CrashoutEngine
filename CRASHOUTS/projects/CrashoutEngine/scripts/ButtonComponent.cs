/*!
@file       ButtonComponent.cs
@author     Renfred Lee
@date       26/11/2025
@brief      Button Component for Crashouts Engine.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
// ButtonComponent.cs
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    // The attribute points to InternalCalls for Add/Remove/Has
    [NativeComponent("AddButton", "RemoveButton", "EntityHasButton")]
    public class ButtonComponent
    {
        public uint EntityID;

        // Internal constructor used by Entity.GetComponent<T>
        public ButtonComponent() { }

        // Public API
        public bool IsClicked()
        {
            return InternalIsClicked(EntityID);
        }

        public bool IsHovered()
        {
            return InternalIsHovered(EntityID);
        }

        // Internal Calls (Private to this class)
        // Matches "CrashoutsEngine.ButtonComponent::InternalIsClicked" in C++
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static bool InternalIsClicked(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static bool InternalIsHovered(uint entityID);
    }
}
