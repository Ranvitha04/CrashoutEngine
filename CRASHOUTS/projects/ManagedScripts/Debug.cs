using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class Debug
    {
        // Maps to "CrashoutsEngine.Debug::Log" in C++
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Log(string message);
    }
}