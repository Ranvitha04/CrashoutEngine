/*!
@file       ReplayManager.cs
@author     Renfred Lee
@date       13/03/2026
@brief      Managed API for replay recording and playback.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    /*!
    @brief Static API for recording and playing back replay data via native calls.
    */
    public static class ReplayManager
    {
        /*!
        @brief Start recording a replay.
        */
        public static void StartRecording() => InternalStartRecording();

        /*!
        @brief Stop recording the current replay.
        */
        public static void StopRecording() => InternalStopRecording();

        /*!
        @brief Stop replay playback.
        */
        public static void StopPlayback() => InternalStopPlayback();

        /*!
        @brief Start replay playback.
        */
        public static void StartPlayback() => InternalStartPlayback();

        /*!
        @brief Save the current recording to a file.
        @param filename Output file name/path.
        */
        public static void SaveRecording(string filename) => InternalSaveRecording(filename);

        /*!
        @brief Load a recording from a file.
        @param filename Input file name/path.
        @return True if load succeeded, otherwise false.
        */
        public static bool LoadRecording(string filename) => InternalLoadRecording(filename);

        /*!
        @brief Record a transform sample for the active recording.
        @param pos Position to record.
        @param rot Rotation to record.
        */
        public static void RecordTransform(CEVec3 pos, float rot) => InternalRecordTransform(pos.x, pos.y, pos.z, rot);

        /*!
        @brief Get the current ghost transform during playback.
        @param pos Output ghost position.
        @param rot Output ghost rotation.
        @return True if a transform was returned, otherwise false.
        */
        public static bool GetGhostTransform(out CEVec3 pos, out float rot)
        {
            float x = 0, y = 0, z = 0, r = 0;
            bool success = InternalGetGhostTransform(ref x, ref y, ref z, ref r);
            pos = new CEVec3(x, y, z);
            rot = r;
            return success;
        }

        /*!
        @brief Native calls for replay functionality.
        */
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
