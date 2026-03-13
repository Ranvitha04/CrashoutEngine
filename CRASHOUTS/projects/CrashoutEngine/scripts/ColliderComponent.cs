/*!
@file       collidercomponent.cs
@author     Renfred Lee
@author     Shahrin Idzuan
@date       29/11/2025
@brief      collider component for Crashouts Engine

All content ? 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddCollider", "RemoveCollider", "EntityHasCollider")]
    public class ColliderComponent
    {
        public uint EntityID;

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalUpdateRotation(uint entityID, float rot);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalGetEnabled(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetEnabled(uint entityID, bool enabled);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalGetAABB(uint entityID, out CEVec3 min, out CEVec3 max);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetAABB(uint entityID, ref CEVec3 min, ref CEVec3 max);

        public void UpdateRotation(float rotation)
        {
            InternalUpdateRotation(EntityID, rotation);
        }

        public bool Enabled
        {
            get => InternalGetEnabled(EntityID);
            set => InternalSetEnabled(EntityID, value);
        }

        public void GetAABB(out CEVec3 min, out CEVec3 max)
        {
            InternalGetAABB(EntityID, out min, out max);
        }

        public void SetAABB(CEVec3 min, CEVec3 max)
        {
            InternalSetAABB(EntityID, ref min, ref max);
        }
    }
}
