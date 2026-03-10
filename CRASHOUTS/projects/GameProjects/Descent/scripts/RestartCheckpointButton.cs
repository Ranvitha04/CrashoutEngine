/*!
@file       RestartCheckpointButton.cs
@brief      Handles the Restart on Checkpoint button in the GameOver scene.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class RestartCheckpointButton : ScriptBase
    {
        [SerializeField] private string gameSceneName = ""; // Leave empty to reload current scene
        [SerializeField] private float fadeDuration = 0.5f;

        public override void Update()
        {
            var btn = Entity.GetComponent<ButtonComponent>();
            if (btn == null) return;

            if (btn.IsClicked())
            {
                // Set the flag so PlayerMovement respawns at checkpoint on start
                PlayerMovement.RespawnNextLoad = true;

                // FIX: If blank, use the last known game level name instead of the current UI scene name
                string targetScene = string.IsNullOrEmpty(gameSceneName) ? PlayerMovement.lastLevelName : gameSceneName;
                
                if (!string.IsNullOrEmpty(targetScene))
                {
                    SceneManager.LoadScene(targetScene, fadeDuration);
                }
                else
                {
                    // Fallback if somehow everything is empty
                    SceneManager.LoadScene("tutorial", fadeDuration);
                }
            }
        }
    }
}
