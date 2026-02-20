/*!
@file       ShapeComponent.cs
@author     lee.r@digipen.edu
@date       28/11/2025
@brief      C# Script for Shape Component

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public enum ShapeType
    {
        Rectangle = 0,
        Line = 1,
        Triangle = 2,
        Circle = 3
    }

    // Attribute required for Entity.GetComponent
    [NativeComponent("AddShape", "RemoveShape", "EntityHasShape")]
    public class ShapeComponent
    {
        public uint EntityID;

        public ShapeComponent() { }

        // Properties
        public ShapeType Type
        {
            get => (ShapeType)InternalGetType(EntityID);
            set => InternalSetType(EntityID, (int)value);
        }

        // Using a helper struct/vector for color would be nice, 
        // but passing r,g,b,a explicitly is simplest.
        public void SetColor(float r, float g, float b, float a)
        {
            InternalSetColor(EntityID, r, g, b, a);
        }

        // Public Property for easy access
        public float Alpha
        {
            set => InternalSetAlpha(EntityID, value);
        }

        // --- Internal Calls ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalGetColor(uint entityID, out float r, out float g, out float b, out float a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetColor(uint entityID, float r, float g, float b, float a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetAlpha(uint entityID, float alpha);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static int InternalGetType(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetType(uint entityID, int type);
    }
}
