/*!
@file       LightComponent.cs
@author
@date       11/03/2026
@brief      C# wrapper for the engine's LightComponent.
*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [NativeComponent("AddLight", "RemoveLight", "EntityHasLight")]
    public class LightComponent
    {
        public uint EntityID;

        public bool Active
        {
            get => InternalGetActive(EntityID);
            set => InternalSetActive(EntityID, value);
        }

        public float Intensity
        {
            get => InternalGetIntensity(EntityID);
            set => InternalSetIntensity(EntityID, value);
        }

        public CEVec3 Color
        {
            get
            {
                InternalGetColor(EntityID, out CEVec3 c);
                return c;
            }
            set => InternalSetColor(EntityID, ref value);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float InternalGetIntensity(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetIntensity(uint entityID, float intensity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalGetColor(uint entityID, out CEVec3 color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetColor(uint entityID, ref CEVec3 color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalGetActive(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSetActive(uint entityID, bool active);
    }
}

