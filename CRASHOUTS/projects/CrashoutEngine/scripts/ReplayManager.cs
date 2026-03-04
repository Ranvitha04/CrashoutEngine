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
        public static bool IsVirtualKeyPressed(int keycode) => InternalIsVirtualKeyPressed(keycode);

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
        private static extern bool InternalIsVirtualKeyPressed(int keycode);
    }
}