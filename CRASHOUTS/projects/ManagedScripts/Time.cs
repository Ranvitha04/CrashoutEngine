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
