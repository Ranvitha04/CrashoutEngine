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