/*!
@file       TagComponent.cs
@author     lee.r@digipen.edu
@date       29/01/2026
@brief      Tag Component for Entity Tagging System
All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddTag", "RemoveTag", "EntityHasTag")]
    public class TagComponent
    {
        public uint EntityID;

        public TagComponent() { }

        // 1. Raw Access (The whole mask)
        public uint CurrentMask
        {
            get => InternalGetTag(EntityID);
            set => InternalSetTag(EntityID, value);
        }

        // 2. CHECK: Does it have this specific Tag?
        public bool HasTag(string name)
        {
            uint id = InternalGetTagByName(name);
            // Bitwise AND check
            return (CurrentMask & id) != 0;
        }

        // 3. ADD: Add a Tag (without removing others)
        public void AddTag(string name)
        {
            uint id = InternalGetTagByName(name);
            // Bitwise OR to combine
            CurrentMask |= id;
        }

        // 4. REMOVE: Remove specific Tag
        public void RemoveTag(string name)
        {
            uint id = InternalGetTagByName(name);
            // Bitwise AND with Inverse to remove
            CurrentMask &= ~id;
        }

        // 5. SET: Overwrite everything with just this Tag
        public void SetExclusive(string name)
        {
            uint id = InternalGetTagByName(name);
            CurrentMask = id;
        }

        // --- Internal Calls ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static uint InternalGetTag(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetTag(uint entityID, uint value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static uint InternalGetTagByName(string name);
    }
}
