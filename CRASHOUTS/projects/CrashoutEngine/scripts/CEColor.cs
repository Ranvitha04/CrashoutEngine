/*!
@file       CEColor.cs
@author     Renfred Lee
@date       13/03/2026
@brief      Managed CEColor struct matching the engine memory layout.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System.Runtime.InteropServices;

namespace CrashoutsEngine
{
    /*!
    @brief Pack the struct to match the native engine layout.
    */
    [StructLayout(LayoutKind.Sequential)]
    /*!
    @brief RGBA color represented as four floats.
    */
    public struct CEColor
    {
        public float r;
        public float g;
        public float b;
        public float a;

        /*!
        @brief Construct a color from RGBA components.
        @param r Red component.
        @param g Green component.
        @param b Blue component.
        @param a Alpha component.
        */
        public CEColor(float r, float g, float b, float a = 1.0f)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }

        /*!
        @brief Common color presets.
        */
        public static CEColor White => new CEColor(1.0f, 1.0f, 1.0f, 1.0f);
        public static CEColor Black => new CEColor(0.0f, 0.0f, 0.0f, 1.0f);
        public static CEColor Red => new CEColor(1.0f, 0.0f, 0.0f, 1.0f);
    }
}
