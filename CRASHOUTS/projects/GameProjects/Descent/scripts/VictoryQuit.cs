/*!
@file       VictoryQuit.cs
@brief      Handles the Victory scene Quit button to return to main menu.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class VictoryQuit : ScriptBase
    {
        [SerializeField] private string menuSceneName = "descentmenu";
        [SerializeField] private float fadeDuration = 0.5f;

        public override void Update()
        {
            var btn = Entity.GetComponent<ButtonComponent>();
            if (btn == null) return;

            if (btn.IsClicked())
            {
                SceneManager.LoadScene(menuSceneName, fadeDuration);
            }
        }
    }
}
