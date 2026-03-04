using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class ReplayManager
    {
        public static void StartRecording() => InternalStartRecording();
        public static void Stop() => InternalStop();
        public static void StartPlayback() => InternalStartPlayback();
        public static void SaveRecording(string filename) => InternalSaveRecording(filename);
        public static void LoadRecording(string filename) => InternalLoadRecording(filename);

        // --- EXTERNS ---
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStartRecording();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStop();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalStartPlayback();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalSaveRecording(string filename);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalLoadRecording(string filename);
    }
}