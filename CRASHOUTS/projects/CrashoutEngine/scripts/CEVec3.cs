/*!
@file       CEVec3.cs
@author     Renfred Lee
@date       29/11/2025
@brief      CEVec3 struct for 3D vector representation

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
namespace CrashoutsEngine
{
    public struct CEVec3
    {
        public float x;
        public float y;
        public float z;

        public CEVec3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        // Operators
        public static CEVec3 operator +(CEVec3 a, CEVec3 b)
            => new CEVec3(a.x + b.x, a.y + b.y, a.z + b.z);

        public static CEVec3 operator -(CEVec3 a, CEVec3 b)
            => new CEVec3(a.x - b.x, a.y - b.y, a.z - b.z);

        public static CEVec3 operator *(CEVec3 a, float s)
            => new CEVec3(a.x * s, a.y * s, a.z * s);

        public static CEVec3 operator *(float s, CEVec3 a)
            => new CEVec3(a.x * s, a.y * s, a.z * s);

        public static CEVec3 operator /(CEVec3 a, float s)
            => new CEVec3(a.x / s, a.y / s, a.z / s);

        public override string ToString()
            => $"CEVec3({x}, {y}, {z})";
    }
}
