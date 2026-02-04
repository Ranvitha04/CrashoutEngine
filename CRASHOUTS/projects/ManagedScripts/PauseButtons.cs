using System;

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

        // --- STATE ---
        private PlayerMovement playerScript;
        private bool isPaused = false;
        private bool isRestartMenuOpen = false;
        private bool isSettingsMenuOpen = false;

        // --- FIX: INPUT COOLDOWN ---
        private float inputCooldown = 0.0f;

        public override void Start()
        {
            if (PlayerMovement.Instance != null)
                playerScript = PlayerMovement.Instance;

            // Find UI
            pauseScreen = Entity.Find("Pause");
            restartCheckScreen = Entity.Find("RestartCheck");
            openedBookScreen = Entity.Find("OpenedBook");
            closedBookScreen = Entity.Find("ClosedBook");

            // Find Buttons
            btnResume = Entity.Find("ResumeButton");
            btnRestart = Entity.Find("RestartLevelButton");
            btnSettings = Entity.Find("SettingsButton");
            btnQuit = Entity.Find("QuitButton");
            btnYes = Entity.Find("Yes");
            btnNo = Entity.Find("No");

            // Init State
            SetEntityActive(pauseScreen, false);
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
        }

        public override void Update()
        {
            // 1. INPUT HANDLING
            if (Input.IsKeyTriggered(Key.Escape) || Input.IsKeyTriggered(Key.P))
            {
                if (playerScript != null && playerScript.isDialogueActive) return;

                TogglePause();
                inputCooldown = 0.2f;
                return;
            }

            // 2. COOLDOWN
            if (inputCooldown > 0.0f)
            {
                inputCooldown -= (float)Time.DT;
                return;
            }

            // 3. UI STATE MACHINE
            if (isPaused)
            {
                if (isRestartMenuOpen)
                {
                    HandleRestartConfirmButtons();
                }
                else if (isSettingsMenuOpen)
                {
                    // FIX: Do NOT check btnSettings here. It is hidden and stale.
                    // Only check for a specific "Back" or "Close" button inside the book.
                    // For now, pressing ESC (handled above) is the correct way to close it.

                    // If you have a specific close button in the book, check it here:
                    // if (CheckButtonClick(btnCloseBook)) CloseSettings();
                }
                else
                {
                    HandleMainPauseButtons();
                }
            }
        }

        private void TogglePause()
        {
            // Back out of menus first
            if (isRestartMenuOpen) { CloseRestartMenu(); return; }
            if (isSettingsMenuOpen) { CloseSettings(); return; }

            isPaused = !isPaused;

            // Sync with Player
            if (playerScript != null) playerScript.SetPauseState(isPaused);

            // Show/Hide Main Screen AND the Closed Book background
            SetEntityActive(pauseScreen, isPaused);
            SetEntityActive(closedBookScreen, isPaused); // <--- ADD THIS

            // Force close sub-menus
            isRestartMenuOpen = false;
            isSettingsMenuOpen = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
        }

        private void ResumeGame()
        {
            // We just trigger TogglePause logic, which handles unpausing perfectly
            if (isPaused) TogglePause();
        }

        private void HandleMainPauseButtons()
        {
            if (CheckButtonClick(btnResume)) ResumeGame();
            else if (CheckButtonClick(btnRestart)) OpenRestartMenu();
            else if (CheckButtonClick(btnSettings)) OpenSettings();
            else if (CheckButtonClick(btnQuit)) SceneManager.LoadScene("descentmenu", 1.0f);
        }

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

        // --- SUB MENUS ---
        private void OpenRestartMenu()
        {
            isRestartMenuOpen = true;
            SetEntityActive(restartCheckScreen, true);
            SetEntityActive(btnRestart, false);
        }
        private void CloseRestartMenu()
        {
            isRestartMenuOpen = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(btnRestart, true);
        }
        private void OpenSettings()
        {
            isSettingsMenuOpen = true;
            SetEntityActive(openedBookScreen, true);
            SetEntityActive(closedBookScreen, false);
        }
        private void CloseSettings()
        {
            isSettingsMenuOpen = false;
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(closedBookScreen, true);
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