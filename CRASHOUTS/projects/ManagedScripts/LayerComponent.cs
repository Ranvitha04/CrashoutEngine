using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddLayer", "RemoveLayer", "EntityHasLayer")]
    public class LayerComponent
    {
        public uint EntityID;

        public LayerComponent() { }

        // 1. Raw Access (The whole mask)
        public uint CurrentMask
        {
            get => InternalGetLayer(EntityID);
            set => InternalSetLayer(EntityID, value);
        }

        // 2. CHECK: Does it have this specific layer?
        public bool HasLayer(string name)
        {
            uint id = InternalGetLayerByName(name);
            // Bitwise AND check
            return (CurrentMask & id) != 0;
        }

        // 3. ADD: Add a layer (without removing others)
        public void AddLayer(string name)
        {
            uint id = InternalGetLayerByName(name);
            // Bitwise OR to combine
            CurrentMask |= id;
        }

        // 4. REMOVE: Remove specific layer
        public void RemoveLayer(string name)
        {
            uint id = InternalGetLayerByName(name);
            // Bitwise AND with Inverse to remove
            CurrentMask &= ~id;
        }

        // 5. SET: Overwrite everything with just this layer
        public void SetExclusive(string name)
        {
            uint id = InternalGetLayerByName(name);
            CurrentMask = id;
        }

        // --- Internal Calls ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static uint InternalGetLayer(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetLayer(uint entityID, uint value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static uint InternalGetLayerByName(string name);
    }
}