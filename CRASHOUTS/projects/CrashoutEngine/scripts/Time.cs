/*!
@file       time.cs
@author     Renfred Lee
@date       29/11/2025
@brief      time class for accessing delta time, fixed delta time, FPS, and steps

All content � 2025 Digipen Institute of Technology Singapore.All rights reserved.

//__________________________________________________________________________*/
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class Time
    {
        public static double DT => InternalCalls.TimeGetDT();
        public static double FixedDT => InternalCalls.TimeGetFixedDT();
        public static double FPS => InternalCalls.TimeGetFPS();
        public static int Steps => InternalCalls.TimeGetSteps();
    }
}
