/*!
@file       PauseButtons.cs
@author     lee.r@digipen.edu
@date       28/11/2025
@brief      Handles the pause menu buttons and their interactions.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PauseButtons : ScriptBase
    {
        [SerializeField] private string currentScene;

        // --- CACHED ENTITIES ---
        private Entity pauseScreen;
        private Entity restartCheckScreen;
        private Entity openedBookScreen;
        private Entity closedBookScreen;

        private Entity btnResume;
        private Entity btnRestart;
        private Entity btnSettings;
        private Entity btnQuit;
        private Entity btnYes;
        private Entity btnNo;

        // NEW: Close Book Button
        private Entity btnCloseBook;

        // --- STATE ---
        private PlayerMovement playerScript;
        private bool isPaused = false;
        private bool isRestartMenuOpen = false;
        private bool isSettingsMenuOpen = false;

        // --- INPUT COOLDOWN ---
        private float inputCooldown = 0.0f;

        public override void Start()
        {
            if (PlayerMovement.Instance != null)
                playerScript = PlayerMovement.Instance;

            // Find UI Containers
            pauseScreen = Entity.Find("Pause");
            restartCheckScreen = Entity.Find("RestartCheck");
            openedBookScreen = Entity.Find("OpenedBook");
            closedBookScreen = Entity.Find("ClosedBook");

            // Find Main Buttons
            btnResume = Entity.Find("ResumeButton");
            btnRestart = Entity.Find("RestartLevelButton");
            btnSettings = Entity.Find("SettingsButton");
            btnQuit = Entity.Find("QuitButton");
            btnYes = Entity.Find("Yes");
            btnNo = Entity.Find("No");

            // NEW: Find Close Book Button
            btnCloseBook = Entity.Find("CloseBookBtn");

            // Init State: Hide everything except the game
            SetEntityActive(pauseScreen, false);
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
            // closedBookScreen is part of the pause menu, so hide it too
            SetEntityActive(closedBookScreen, false);
        }

        public override void Update()
        {
            // Ensure we have the player reference (handles scene transitions/respawns)
            if (playerScript == null && PlayerMovement.Instance != null)
                playerScript = PlayerMovement.Instance;

            // 1. INPUT HANDLING
            if (Input.IsKeyTriggered(Key.Escape) || Input.IsKeyTriggered(Key.P))
            {
                // Don't pause during dialogue
                if (playerScript != null && playerScript.isDialogueActive) return;

                TogglePause();
                return;
            }

            // 2. BUTTON LOGIC (Remove inputCooldown return to ensure clicks registered)
            if (isPaused)
            {
                if (isRestartMenuOpen) HandleRestartConfirmButtons();
                else if (isSettingsMenuOpen)
                {
                    if (CheckButtonClick(btnCloseBook)) CloseSettings();
                }
                else HandleMainPauseButtons();
            }

            if (inputCooldown > 0.0f) inputCooldown -= (float)Time.DT;
        }

        // --- MAIN MENU HANDLER ---
        private void HandleMainPauseButtons()
        {
            if (CheckButtonClick(btnResume)) ResumeGame();
            else if (CheckButtonClick(btnRestart)) OpenRestartMenu();
            else if (CheckButtonClick(btnSettings)) OpenSettings();
            else if (CheckButtonClick(btnQuit)) SceneManager.LoadScene("descentmenu", 1.0f);
        }

        // --- RESTART HANDLER ---
        private void HandleRestartConfirmButtons()
        {
            if (CheckButtonClick(btnYes))
            {
                if (string.IsNullOrEmpty(currentScene)) currentScene = "Level1";
                SceneManager.LoadScene(currentScene, 1.0f);
            }
            else if (CheckButtonClick(btnNo))
            {
                CloseRestartMenu();
            }
        }

        // --- STATE HELPERS ---

        private void TogglePause()
        {
            // Escape Key Back-Out Logic
            if (isRestartMenuOpen) { CloseRestartMenu(); return; }
            if (isSettingsMenuOpen) { CloseSettings(); return; }

            isPaused = !isPaused;

            // Sync with Player
            if (playerScript != null) playerScript.SetPauseState(isPaused);

            // Toggle Main Screens
            SetEntityActive(pauseScreen, isPaused);
            SetEntityActive(closedBookScreen, isPaused);

            // Ensure Sub-menus are closed
            isRestartMenuOpen = false;
            isSettingsMenuOpen = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);

            Debug.Log("unpause?");
        }

        private void ResumeGame()
        {
            TogglePause();
        }

        // --- SUB MENU TRANSITIONS ---

        private void OpenRestartMenu()
        {
            isRestartMenuOpen = true;
            SetEntityActive(btnRestart, false);
            SetEntityActive(restartCheckScreen, true);
            inputCooldown = 0.2f; // Prevent accidental double clicks
        }

        private void CloseRestartMenu()
        {
            isRestartMenuOpen = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(btnRestart, true);
            inputCooldown = 0.2f;
        }

        private void OpenSettings()
        {
            isSettingsMenuOpen = true;
            SetEntityActive(openedBookScreen, true);
            SetEntityActive(closedBookScreen, false); // Hide the main book background

            // IMPORTANT: Add delay here so the click that opened settings
            // doesn't immediately click "CloseBookBtn" if they overlap.
            inputCooldown = 0.2f;
        }

        private void CloseSettings()
        {
            isSettingsMenuOpen = false;
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(closedBookScreen, true); // Restore main book background

            inputCooldown = 0.2f;
        }

        // --- UTILS ---
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
