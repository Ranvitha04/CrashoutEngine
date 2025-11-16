namespace CrashoutsEngine
{
    public static class Time
    {
        public static double dt => InternalCalls.TimeGetDT();
        public static double fixedDT => InternalCalls.TimeGetFixedDT();
        public static double fps => InternalCalls.TimeGetFPS();
        public static int steps => InternalCalls.TimeGetSteps();
    }
}
