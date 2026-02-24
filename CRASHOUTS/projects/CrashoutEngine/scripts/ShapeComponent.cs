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

        public CEColor Color
        {
            get
            {
                InternalGetColor(EntityID, out CEColor c);
                return c;
            }
            set
            {
                InternalSetColor(EntityID, ref value);
            }
        }

        // Properties
        public ShapeType Type
        {
            get => (ShapeType)InternalGetType(EntityID);
            set => InternalSetType(EntityID, (int)value);
        }

        // Wraps the new struct so old scripts don't break
        public void GetColor(out float r, out float g, out float b, out float a)
        {
            CEColor c = this.Color; 
            r = c.r;
            g = c.g;
            b = c.b;
            a = c.a;
        }

        // Wraps the new struct so old scripts don't break
        public void SetColor(float r, float g, float b, float a)
        {
            this.Color = new CEColor(r, g, b, a);
        }

        // Public Property for easy access
        public float Alpha
        {
            set => InternalSetAlpha(EntityID, value);
        }

        // --- Internal Calls ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalGetColor(uint entityID, out CEColor color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetColor(uint entityID, ref CEColor color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetAlpha(uint entityID, float alpha);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static int InternalGetType(uint entityID);   

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern static void InternalSetType(uint entityID, int type);
    }
}
