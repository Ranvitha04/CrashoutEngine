/*!
@file       Debug.cs
@author     lee.r@digipen.edu
@date       26/11/2025
@brief      Debug class for logging and drawing debug shapes.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class Debug
    {
        // Maps to "CrashoutsEngine.Debug::Log" in C++
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Log(string message);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void DrawLine(ref CEVec3 start, ref CEVec3 end, float thickness, ref CEVec3 color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void DrawTrajectory(ref CEVec3 start, ref CEVec3 end, float thickness, ref CEVec3 color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void DrawCircle(ref CEVec2 size, ref CEVec3 position, ref CEVec3 color);
    }
}
