using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    public static class SceneManager
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LoadScene(string sceneName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void QuitGame();

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void TransitionToScene(string sceneName, float duration);
    }
}