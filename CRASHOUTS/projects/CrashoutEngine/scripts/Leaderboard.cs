/*!
@file   Leaderboard.cs
@author Renfred Lee (69%), Renfred (RTIS) (31%)
@date   08/03/2025

@brief  Leaderboard interface with C++ leaderboard items

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class Leaderboard
    {
        // Submits a standard integer score.
        public static void SubmitScore(string playerName, int score)
        {
            SubmitScore_Internal(playerName, score);
        }

        // Helper for timing-based leaderboards (e.g., Speedruns).
        // Converts a float time (seconds) into milliseconds (int) for backend sorting.
        public static void SubmitTime(string playerName, float timeInSeconds)
        {
            int timeInMs = (int)(timeInSeconds * 1000f);
            SubmitScore_Internal(playerName, timeInMs);
        }

        public static void ForceRefresh(Entity leaderboardEntity)
        {
            if (leaderboardEntity != null && leaderboardEntity.ID != 0)
            {
                ForceRefresh_Internal(leaderboardEntity.ID);
            }
        }

        // The exact signature matching our mono_add_internal_call string
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SubmitScore_Internal(string name, int score);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void ForceRefresh_Internal(ulong entityID);
    }
}