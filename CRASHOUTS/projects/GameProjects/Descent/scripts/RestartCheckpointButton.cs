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
        [SerializeField] private string gameSceneName = "descentgame";
        [SerializeField] private float fadeDuration = 0.5f;

        public override void Update()
        {
            var btn = Entity.GetComponent<ButtonComponent>();
            if (btn == null) return;

            if (btn.IsClicked())
            {
                // Set the flag so PlayerMovement respawns at checkpoint on start
                PlayerMovement.RespawnNextLoad = true;
                SceneManager.LoadScene(gameSceneName, fadeDuration);
            }
        }
    }
}
