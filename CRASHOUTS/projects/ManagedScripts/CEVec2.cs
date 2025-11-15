namespace CrashoutsEngine
{
    public struct CEVec2
    {
        public float x;
        public float y;

        public CEVec2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        // Operators
        public static CEVec2 operator +(CEVec2 a, CEVec2 b)
            => new CEVec2(a.x + b.x, a.y + b.y);

        public static CEVec2 operator -(CEVec2 a, CEVec2 b)
            => new CEVec2(a.x - b.x, a.y - b.y);

        public static CEVec2 operator *(CEVec2 a, float s)
            => new CEVec2(a.x * s, a.y * s);

        public static CEVec2 operator *(float s, CEVec2 a)
            => new CEVec2(a.x * s, a.y * s);

        public static CEVec2 operator /(CEVec2 a, float s)
            => new CEVec2(a.x / s, a.y / s);

        public override string ToString()
            => $"CEVec2({x}, {y})";
    }
}
