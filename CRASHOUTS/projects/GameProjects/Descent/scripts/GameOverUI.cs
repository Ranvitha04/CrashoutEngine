/*!
@file       GameOverUI.cs
@author     lee.r@digipen.edu
@date       28/11/2025
@brief      Handles the Game Over and Win UI screens, including restart and quit functionality.

All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using CrashoutsEngine;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class GameOverUI : ScriptBase
    {
        // Name of the scene to reload
        [SerializeField] private string currentSceneName = "descentgame";
        [SerializeField] private string mainMenuScene = "descentmenu";

        // --- ENTITIES & TRANSFORMS ---
        private Entity gameOverScreen;
        private TransformComponent gameOverTf; // Needed to check Z position

        private Entity winScreen;
        private TransformComponent winTf;      // Needed to check Z position

        private Entity restartCheckScreen;     // "RestartCheck2"
        private Entity winrestartCheckScreen;     // "RestartCheck2"

        // --- BUTTONS (Game Over) ---
        private Entity btnGORestart; // "RestartLevelButton"
        private Entity btnGOQuit;    // "QuitButton"
        private Entity btnYes2;      // "Yes2"
        private Entity btnNo2;       // "No2"

        // --- BUTTONS (Win Screen) ---
        //private Entity btnWinReturn; // "ReturnBtn"
        //private Entity btnWinQuit;   // "QuitBtn" (Assuming different name/entity than GO Quit)
        private Entity winbtnGORestart; // "RestartLevelButton"
        private Entity winbtnGOQuit;    // "QuitButton"
        private Entity winbtnYes;      // "Yes2"
        private Entity winbtnNo;       // "No2"

        // --- STATE ---
        private bool isRestartCheckOpen = false;
        private float inputCooldown = 0.0f;

        public override void Start()
        {
            Debug.Log("[GameOverUI] Initializing...");

            // 1. FIND SCREENS
            gameOverScreen = Entity.Find("GameOverScreen");
            if (gameOverScreen != null) gameOverTf = gameOverScreen.GetComponent<TransformComponent>();

            winScreen = Entity.Find("WinScreen");
            if (winScreen != null) winTf = winScreen.GetComponent<TransformComponent>();

            restartCheckScreen = Entity.Find("RestartCheck2");
            winrestartCheckScreen = Entity.Find("RestartCheck3");

            // 2. FIND GAME OVER BUTTONS
            btnGORestart = Entity.Find("RestartLevelButton2");
            btnGOQuit = Entity.Find("QuitButton2");
            btnYes2 = Entity.Find("Yes2");
            btnNo2 = Entity.Find("No2");

            // 3. FIND WIN BUTTONS
            winbtnGORestart = Entity.Find("RestartLevelButton3");
            winbtnGOQuit = Entity.Find("QuitButton3");
            winbtnYes = Entity.Find("Yes3");
            winbtnNo = Entity.Find("No3");

            // 4. INIT SUB-MENUS
            SetEntityActive(restartCheckScreen, false);
        }

        public override void Update()
        {
            // Input Cooldown to prevent double clicks
            if (inputCooldown > 0.0f)
            {
                inputCooldown -= (float)Time.DT;
                return;
            }

            // --- 1. HANDLE GAME OVER SCREEN ---
            // We check Z > 0 because PlayerMovement shows it by moving Z, not SetActive
            if (gameOverTf != null)
            {
                if (isRestartCheckOpen)
                {
                    HandleRestartConfirmation();
                }
                else
                {
                    HandleGameOverMain();
                }
            }

            // --- 2. HANDLE WIN SCREEN ---
            if (winTf != null)
            {
                if (isRestartCheckOpen)
                {
                    HandleRestartConfirmation();
                }
                else
                {
                    HandleWinMain();
                }
            }
        }

        // =================================================================================
        //                                GAME OVER LOGIC
        // =================================================================================

        private void HandleGameOverMain()
        {
            if (CheckButtonClick(btnGORestart))
            {
                Debug.Log("press");
                OpenRestartCheck();
            }
            else if (CheckButtonClick(btnGOQuit))
            {
                // Quit to Menu (Matches PauseButtons logic)
                SceneManager.LoadScene(mainMenuScene, 1.0f);
            }
        }

        private void HandleRestartConfirmation()
        {
            if (CheckButtonClick(btnYes2))
            {
                // RELOAD LEVEL
                Debug.Log("[GameOverUI] Restarting...");
                if (string.IsNullOrEmpty(currentSceneName)) currentSceneName = "tutorial";
                SceneManager.LoadScene(currentSceneName, 1.0f);
            }
            else if (CheckButtonClick(btnNo2))
            {
                CloseRestartCheck();
            }
            else if (CheckButtonClick(winbtnNo))
            {
                WinCloseRestartCheck();
            }
            else if (CheckButtonClick(winbtnYes))
            {
                // RELOAD LEVEL
                Debug.Log("[GameOverUI] Restarting...");
                if (string.IsNullOrEmpty(currentSceneName)) currentSceneName = "tutorial";
                SceneManager.LoadScene(currentSceneName, 1.0f);
            }
        }

        private void OpenRestartCheck()
        {
            isRestartCheckOpen = true;
            SetEntityActive(restartCheckScreen, true);
            inputCooldown = 0.2f; // Safety delay
        }

        private void CloseRestartCheck()
        {
            isRestartCheckOpen = false;
            SetEntityActive(restartCheckScreen, false);
            inputCooldown = 0.2f;
        }

        private void WinOpenRestartCheck()
        {
            isRestartCheckOpen = true;
            SetEntityActive(winrestartCheckScreen, true);
            inputCooldown = 0.2f; // Safety delay
        }

        private void WinCloseRestartCheck()
        {
            isRestartCheckOpen = false;
            SetEntityActive(winrestartCheckScreen, false);
            inputCooldown = 0.2f;
        }

        // =================================================================================
        //                                  WIN LOGIC
        // =================================================================================

        private void HandleWinMain()
        {
            if (CheckButtonClick(winbtnGORestart))
            {
                Debug.Log("press");
                WinOpenRestartCheck();
            }
            else if (CheckButtonClick(winbtnGOQuit))
            {
                // Quit to Menu (Matches PauseButtons logic)
                SceneManager.LoadScene(mainMenuScene, 1.0f);
            }
        }

        //private void HandleWinMain()
        //{
        //    if (CheckButtonClick(btnWinReturn))
        //    {
        //        Debug.Log("[WinUI] Returning to Menu...");
        //        SceneManager.LoadScene(mainMenuScene);
        //    }
        //    else if (CheckButtonClick(btnWinQuit))
        //    {
        //        Debug.Log("[WinUI] Quitting App...");
        //        SceneManager.QuitGame();
        //    }
        //}

        // =================================================================================
        //                                   HELPERS
        // =================================================================================

        private bool CheckButtonClick(Entity btnEntity)
        {
            if (btnEntity == null) return false;
            var btn = btnEntity.GetComponent<ButtonComponent>();
            return (btn != null && btn.IsClicked());
        }

        private void SetEntityActive(Entity e, bool active)
        {
            if (e != null) InternalCalls.SetActive(e.ID, active);
        }
    }
}
