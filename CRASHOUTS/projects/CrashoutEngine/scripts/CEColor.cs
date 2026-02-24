using System.Runtime.InteropServices;

namespace CrashoutsEngine
{
    // This tells Mono to pack the memory exactly like a C++ struct
    [StructLayout(LayoutKind.Sequential)]
    public struct CEColor
    {
        public float r;
        public float g;
        public float b;
        public float a;

        // A handy constructor for quick color creation
        public CEColor(float r, float g, float b, float a = 1.0f)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }

        // Some helpful static presets!
        public static CEColor White => new CEColor(1.0f, 1.0f, 1.0f, 1.0f);
        public static CEColor Black => new CEColor(0.0f, 0.0f, 0.0f, 1.0f);
        public static CEColor Red => new CEColor(1.0f, 0.0f, 0.0f, 1.0f);
    }
}