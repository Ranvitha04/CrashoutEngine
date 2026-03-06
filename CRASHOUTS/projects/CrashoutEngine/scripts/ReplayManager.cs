using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class ReplayManager
    {
        public static void StartRecording() => InternalStartRecording();
        public static void StopRecording() => InternalStopRecording();
        public static void StopPlayback() => InternalStopPlayback();
        public static void StartPlayback() => InternalStartPlayback();
        public static void SaveRecording(string filename) => InternalSaveRecording(filename);
        public static bool LoadRecording(string filename) => InternalLoadRecording(filename);
        public static void RecordTransform(CEVec3 pos, float rot) => InternalRecordTransform(pos.x, pos.y, pos.z, rot);

        // We wrap the C++ pointers into 'out' parameters for clean C# syntax
        public static bool GetGhostTransform(out CEVec3 pos, out float rot)
        {
            float x = 0, y = 0, z = 0, r = 0;
            bool success = InternalGetGhostTransform(ref x, ref y, ref z, ref r);
            pos = new CEVec3(x, y, z);
            rot = r;
            return success;
        }

        // --- EXTERNS ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStartRecording();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStopRecording();
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStopPlayback();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStartPlayback();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSaveRecording(string filename);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalLoadRecording(string filename);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalRecordTransform(float x, float y, float z, float rot);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool InternalGetGhostTransform(ref float x, ref float y, ref float z, ref float rot);
    }
}