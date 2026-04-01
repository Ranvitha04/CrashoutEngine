/*!
@file       GameOverUI.cs
@author     mohamadamirakmal.b
@author     Renfred Lee
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
        private int selectedButtonIndex = 0;
        private float menuNavigationCooldown = 0.0f;
        private bool controllerNavigationActive = false;
        private CEVec2 lastMousePos;

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
            SetEntityActive(winrestartCheckScreen, false);
            lastMousePos = Input.GetMousePosition();
            ResetControllerSelection();
            InternalCalls.UI_ClearForcedHoveredButton();
        }

        public override void Update()
        {
            // Input Cooldown to prevent double clicks
            if (inputCooldown > 0.0f)
            {
                inputCooldown -= (float)Time.DT;
                return;
            }

            if (menuNavigationCooldown > 0.0f)
            {
                menuNavigationCooldown -= (float)Time.DT;
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
            Entity[] buttons = { btnGORestart, btnGOQuit };
            UpdateControllerSelection(buttons);

            if (CheckButtonClick(btnGORestart) || IsControllerSubmit(buttons, btnGORestart))
            {
                Debug.Log("press");
                OpenRestartCheck();
            }
            else if (CheckButtonClick(btnGOQuit) || IsControllerSubmit(buttons, btnGOQuit))
            {
                // Quit to Menu (Matches PauseButtons logic)
                SceneManager.LoadScene(mainMenuScene, 1.0f);
            }
        }

        private void HandleRestartConfirmation()
        {
            Entity yesButton = btnYes2;
            Entity noButton = btnNo2;

            if (winrestartCheckScreen != null && winrestartCheckScreen.IsActive())
            {
                yesButton = winbtnYes;
                noButton = winbtnNo;
            }

            Entity[] buttons = { yesButton, noButton };
            UpdateControllerSelection(buttons);

            if (CheckButtonClick(btnYes2) || IsControllerSubmit(buttons, btnYes2))
            {
                // RELOAD LEVEL
                Debug.Log("[GameOverUI] Restarting...");
                if (string.IsNullOrEmpty(currentSceneName)) currentSceneName = "tutorial";
                SceneManager.LoadScene(currentSceneName, 1.0f);
            }
            else if (CheckButtonClick(btnNo2) || IsControllerSubmit(buttons, btnNo2))
            {
                CloseRestartCheck();
            }
            else if (CheckButtonClick(winbtnNo) || IsControllerSubmit(buttons, winbtnNo))
            {
                WinCloseRestartCheck();
            }
            else if (CheckButtonClick(winbtnYes) || IsControllerSubmit(buttons, winbtnYes))
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
            selectedButtonIndex = 0;
            controllerNavigationActive = false;
        }

        private void CloseRestartCheck()
        {
            isRestartCheckOpen = false;
            SetEntityActive(restartCheckScreen, false);
            inputCooldown = 0.2f;
            ResetControllerSelection();
        }

        private void WinOpenRestartCheck()
        {
            isRestartCheckOpen = true;
            SetEntityActive(winrestartCheckScreen, true);
            inputCooldown = 0.2f; // Safety delay
            selectedButtonIndex = 0;
            controllerNavigationActive = false;
        }

        private void WinCloseRestartCheck()
        {
            isRestartCheckOpen = false;
            SetEntityActive(winrestartCheckScreen, false);
            inputCooldown = 0.2f;
            ResetControllerSelection();
        }

        // =================================================================================
        //                                  WIN LOGIC
        // =================================================================================

        private void HandleWinMain()
        {
            Entity[] buttons = { winbtnGORestart, winbtnGOQuit };
            UpdateControllerSelection(buttons);

            if (CheckButtonClick(winbtnGORestart) || IsControllerSubmit(buttons, winbtnGORestart))
            {
                Debug.Log("press");
                WinOpenRestartCheck();
            }
            else if (CheckButtonClick(winbtnGOQuit) || IsControllerSubmit(buttons, winbtnGOQuit))
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
            if (btn != null && btn.IsClicked())
            {
                AudioManager.GetInstance().PlayAttached("Click", btnEntity.ID);
                controllerNavigationActive = false;
                InternalCalls.UI_ClearForcedHoveredButton();
                return true;
            }

            return false;
        }

        private void ResetControllerSelection()
        {
            selectedButtonIndex = 0;
            menuNavigationCooldown = 0.0f;
            controllerNavigationActive = false;
            InternalCalls.UI_ClearForcedHoveredButton();
        }

        private void UpdateControllerSelection(Entity[] buttons)
        {
            if (buttons == null || buttons.Length == 0)
            {
                controllerNavigationActive = false;
                InternalCalls.UI_ClearForcedHoveredButton();
                return;
            }

            CEVec2 currentMousePos = Input.GetMousePosition();
            if (Math.Abs(currentMousePos.x - lastMousePos.x) > 0.1f || Math.Abs(currentMousePos.y - lastMousePos.y) > 0.1f)
            {
                controllerNavigationActive = false;
            }
            lastMousePos = currentMousePos;

            bool movedDown = false;
            bool movedUp = false;

            if (menuNavigationCooldown <= 0.0f)
            {
                float menuY = Input.GetAxis("MenuY");
                if (Input.GetActionTriggered("UIDown") || menuY <= -0.5f)
                {
                    selectedButtonIndex = (selectedButtonIndex + 1) % buttons.Length;
                    menuNavigationCooldown = 0.2f;
                    movedDown = true;
                }
                else if (Input.GetActionTriggered("UIUp") || menuY >= 0.5f)
                {
                    selectedButtonIndex = (selectedButtonIndex - 1 + buttons.Length) % buttons.Length;
                    menuNavigationCooldown = 0.2f;
                    movedUp = true;
                }
            }

            if (selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                selectedButtonIndex = 0;
            }

            if (movedDown || movedUp || Input.GetActionTriggered("UISubmit"))
            {
                controllerNavigationActive = true;
            }

            if (!controllerNavigationActive)
            {
                InternalCalls.UI_ClearForcedHoveredButton();
                return;
            }

            Entity selected = buttons[selectedButtonIndex];
            if (selected != null && selected.ID != 0 && selected.IsActive())
            {
                InternalCalls.UI_SetForcedHoveredButton(selected.ID);
            }
            else
            {
                InternalCalls.UI_ClearForcedHoveredButton();
            }
        }

        private bool IsControllerSubmit(Entity[] buttons, Entity target)
        {
            if (target == null || target.ID == 0 || buttons == null)
            {
                return false;
            }

            if (selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                return false;
            }

            if (buttons[selectedButtonIndex] == target && Input.GetActionTriggered("UISubmit"))
            {
                AudioManager.GetInstance().PlayAttached("Click", target.ID);
                return true;
            }
            return false;
        }

        private void SetEntityActive(Entity e, bool active)
        {
            if (e != null) InternalCalls.SetActive(e.ID, active);
        }

        public override void OnDestroy()
        {
            InternalCalls.UI_ClearForcedHoveredButton();
        }
    }
}
