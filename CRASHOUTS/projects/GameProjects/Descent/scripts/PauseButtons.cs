using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class PauseButtons : ScriptBase
    {
        [SerializeField] private string currentScene;

        private Entity pauseScreen, restartCheckScreen, openedBookScreen, closedBookScreen;
        private Entity btnResume, btnRestart, btnSettings, btnQuit, btnYes, btnNo, btnCloseBook;

        private PlayerMovement playerScript;
        private bool isPaused = false;
        private bool isRestartMenuOpen = false;
        private bool isSettingsMenuOpen = false;
        private float inputCooldown = 0.0f;

        // NEW: The Trigger Gate
        private bool isMouseReleased = true; 

        public override void Start()
        {
            playerScript = PlayerMovement.Instance;

            pauseScreen = Entity.Find("Pause");
            restartCheckScreen = Entity.Find("RestartCheck");
            openedBookScreen = Entity.Find("OpenedBook");
            closedBookScreen = Entity.Find("ClosedBook");

            btnResume = Entity.Find("ResumeButton");
            btnRestart = Entity.Find("RestartLevelButton");
            btnSettings = Entity.Find("SettingsButton");
            btnQuit = Entity.Find("QuitButton");
            btnYes = Entity.Find("Yes");
            btnNo = Entity.Find("No");
            btnCloseBook = Entity.Find("CloseBookBtn");

            SetEntityActive(pauseScreen, false);
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(closedBookScreen, false);
        }

        public override void Update()
        {
            if (playerScript == null) playerScript = PlayerMovement.Instance;

            // 1. Pause Input
            if (Input.IsKeyTriggered(Key.Escape) || Input.IsKeyTriggered(Key.P))
            {
                if (playerScript != null && playerScript.isDialogueActive) return;
                TogglePause();
                return;
            }

            // 2. Reset Mouse Trigger Gate (MouseButton 0 = Left Click)
            if (!Input.IsMouseButtonDown(0)) 
            {
                isMouseReleased = true;
            }

            // 3. Logic: Only process if mouse was released first
            if (isPaused && isMouseReleased)
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
            else if (CheckButtonClick(btnNo)) CloseRestartMenu();
        }

        private void TogglePause()
        {
            if (isRestartMenuOpen) { CloseRestartMenu(); return; }
            if (isSettingsMenuOpen) { CloseSettings(); return; }

            isPaused = !isPaused;
            if (playerScript != null) playerScript.SetPauseState(isPaused);

            SetEntityActive(pauseScreen, isPaused);
            SetEntityActive(closedBookScreen, isPaused);
            
            isRestartMenuOpen = false;
            isSettingsMenuOpen = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
        }

        private void OpenSettings()
        {
            isSettingsMenuOpen = true;
            SetEntityActive(openedBookScreen, true);
            SetEntityActive(closedBookScreen, false);
            inputCooldown = 0.2f;
        }

        private void CloseSettings()
        {
            isSettingsMenuOpen = false;
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(closedBookScreen, true);
            inputCooldown = 0.2f;
        }

        private void ResumeGame() => TogglePause();
        private void OpenRestartMenu() { isRestartMenuOpen = true; SetEntityActive(btnRestart, false); SetEntityActive(restartCheckScreen, true); }
        private void CloseRestartMenu() { isRestartMenuOpen = false; SetEntityActive(restartCheckScreen, false); SetEntityActive(btnRestart, true); }

        private bool CheckButtonClick(Entity btnEntity)
        {
            if (btnEntity == null) return false;
            var btn = btnEntity.GetComponent<ButtonComponent>();
            
            // The Trigger Logic
            if (btn != null && btn.IsClicked() && isMouseReleased)
            {
                isMouseReleased = false; // Lock the gate until mouse is lifted
                return true;
            }
            return false;
        }

        private void SetEntityActive(Entity e, bool active)
        {
            if (e != null) InternalCalls.SetActive(e.ID, active);
        }
    }
}