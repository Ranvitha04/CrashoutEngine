using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class GameOverUI : ScriptBase
    {
        // Name of the scene to reload when restarting
        // You can hardcode this or maybe get the current scene name if exposed
        private string currentSceneName = "descentgame";

        public override void Start()
        {
            Console.WriteLine("[GameOverUI] Script Started.");
        }

        public override void Update()
        {
            // Check for button interactions
            var btn = Entity.GetComponent<ButtonComponent>();
            var nameComp = Entity.GetComponent<NameComponent>();

            if (btn != null && btn.IsClicked())
            {
                Entity gameOverEnt = Entity.Find("GameOverScreen");
                Entity winEnt = Entity.Find("WinScreen");
                //Debug.Log("Pressed Game OVer");
                if (gameOverEnt != null)
                {
                    var gameOverT = gameOverEnt.GetComponent<TransformComponent>();
                    string btnName = nameComp.ToString();
                    Debug.Log("[GameOverUI] Button Clicked: " + btnName);

                    if (btnName == "RestartBtn" && gameOverT.Position.z > 0.0f)
                    {
                        Debug.Log("[GameOverUI] Restarting Game...");
                        SceneManager.LoadScene(currentSceneName);
                    }
                    else if (btnName == "QuitBtn" && gameOverT.Position.z > 0.0f)
                    {
                        Debug.Log("[GameOverUI] Quitting Game...");
                        SceneManager.QuitGame(); 
                    }
                }

                if (winEnt != null)
                {
                    var winT = winEnt.GetComponent<TransformComponent>();
                    string btnName = nameComp.ToString();
                    Debug.Log("[WinUI] Button Clicked: " + btnName);

                    if (btnName == "ReturnBtn" && winT.Position.z > 0.0f)
                    {
                        Debug.Log("[WinUI] Returning Menu...");
                        SceneManager.LoadScene("descentmenu");
                    }
                    else if (btnName == "QuitBtn" && winT.Position.z > 0.0f)
                    {
                        Debug.Log("[WinUI] Quitting Game...");
                        SceneManager.QuitGame();
                    }
                }
            }

            // Optional: Hover effects
            if (btn != null && btn.IsHovered())
            {
                // You could play a sound here if not already playing
                // if (!hoverSoundPlayed) { AudioManager.Play("ui_hover"); hoverSoundPlayed = true; }
            }
        }
    }
}