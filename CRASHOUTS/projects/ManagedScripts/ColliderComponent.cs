/*!
@file       collidercomponent.cs
@author     m.biniswandi@digipen.edu
@date       29/11/2025
@brief      collider component for Crashouts Engine

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.

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

        public void UpdateRotation(float rotation)
        {
            InternalUpdateRotation(EntityID, rotation);
        }
    }
}
