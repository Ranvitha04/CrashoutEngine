/*!
@file       SceneManager.cs
@author     Mohamad Amir Akmal
@author     Renfred Lee
@date       26/11/2025
@brief      Scene management functionalities for loading scenes and handling transitions.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class SceneManager
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void InternalLoadScene(string sceneName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void QuitGame();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void TransitionToScene(string sceneName, float fadeOut, float fadeIn);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void FadeEffect(float fadeOut, float fadeIn);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool IsTransitioning();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetActiveSceneName();

        // 1. Default (With Transition)
        public static void LoadScene(string sceneName)
        {
            TransitionToScene(sceneName, 0.3f, 0.3f);
        }

        // 2. Detailed Control
        public static void LoadScene(string sceneName, float fadeOut, float fadeIn)
        {
            TransitionToScene(sceneName, fadeOut, fadeIn);
        }

        // 2. Convenience (Same duration for both)
        public static void LoadScene(string sceneName, float duration)
        {
            TransitionToScene(sceneName, duration, duration);
        }

        // 3. Effect Control
        public static void Fade(float fadeOut, float fadeIn)
        {
            FadeEffect(fadeOut, fadeIn);
        }
    }
}
