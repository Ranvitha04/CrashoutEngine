/*!
@file       CEVec2.cs
@author     Renfred Lee
@date       29/11/2025
@brief      CEVec2 struct for 2D vector representation

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
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
