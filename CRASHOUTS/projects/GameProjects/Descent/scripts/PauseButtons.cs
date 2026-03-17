/*!
@file       PauseButtons.cs
@author     mohamadamirakmal.b
@author     Renfred Lee
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Pause menu UI behaviour for the Descent project.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;
using System.Diagnostics;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Handles pausing/unpausing, menu navigation, and related UI screens.
    */
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

        /*!
        @brief Cache entity references for pause UI and buttons.
        */
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

        /*!
        @brief Handle pause input and pause menu button interactions.
        */
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

        /*!
        @brief Handle the main pause menu button actions.
        */
        private void HandleMainPauseButtons()
        {
            if (CheckButtonClick(btnResume)) ResumeGame();
            else if (CheckButtonClick(btnRestart)) OpenRestartMenu();
            else if (CheckButtonClick(btnSettings)) OpenSettings();
            else if (CheckButtonClick(btnQuit)) SceneManager.LoadScene("descentmenu", 1.0f);
        }

        /*!
        @brief Handle the restart confirmation menu buttons.
        */
        private void HandleRestartConfirmButtons()
        {
            if (CheckButtonClick(btnYes))
            {
                if (string.IsNullOrEmpty(currentScene)) currentScene = "Level1";
                SceneManager.LoadScene(currentScene, 1.0f);
            }
            else if (CheckButtonClick(btnNo)) CloseRestartMenu();
        }

        /*!
        @brief Toggle the paused state and update UI and audio effects accordingly.
        */
        private void TogglePause()
        {
            if (isRestartMenuOpen) { CloseRestartMenu(); return; }
            if (isSettingsMenuOpen) { CloseSettings(); return; }

            isPaused = !isPaused;
            if (playerScript != null) playerScript.SetPauseState(isPaused);

            if (isPaused)
            {
                // Music Pause Effects
                AudioManager.AddGroupDSP(true, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                AudioManager.SetGroupDSPParameter(true, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, 22000.0f);
                AudioManager.FadeGroupDSPParameter(true, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, 800.0f, 0.2f);
                // Lower music volume slightly when paused
                AudioManager.FadeGroupVolume(true, AudioManager.playerMusVol * 0.4f, 0.2f);

                // SFF Pause Effect
                AudioManager.AddGroupDSP(false, AudioManager.FMOD_DSP_TYPE.LOWPASS);
                AudioManager.SetGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, 22000.0f);
                AudioManager.FadeGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, 600.0f, 0.2f); // SFX even more muffled
                // Lower SFX volume significantly when paused
                AudioManager.FadeGroupVolume(false, AudioManager.playerSFXVol * 0.3f, 0.2f);
            }
            else
            {
                // Fade music back to normal volume and frequency
                AudioManager.FadeGroupDSPParameter(true, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, 22000.0f, 0.4f);
                AudioManager.FadeGroupVolume(true, AudioManager.playerMusVol, 0.4f);

                // Fade SFX back to normal volume and frequency
                AudioManager.FadeGroupDSPParameter(false, AudioManager.FMOD_DSP_TYPE.LOWPASS, 0, 22000.0f, 0.4f);
                AudioManager.FadeGroupVolume(false, AudioManager.playerSFXVol, 0.4f);
            }

            SetEntityActive(pauseScreen, isPaused);
            SetEntityActive(closedBookScreen, isPaused);
            
            isRestartMenuOpen = false;
            isSettingsMenuOpen = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
        }

        /*!
        @brief Open the settings menu UI.
        */
        private void OpenSettings()
        {
            isSettingsMenuOpen = true;
            SetEntityActive(openedBookScreen, true);
            SetEntityActive(closedBookScreen, false);
            inputCooldown = 0.2f;
        }

        /*!
        @brief Close the settings menu UI.
        */
        private void CloseSettings()
        {
            isSettingsMenuOpen = false;
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(closedBookScreen, true);
            inputCooldown = 0.2f;
        }

        /*!
        @brief Resume the game (alias for toggling pause off).
        */
        private void ResumeGame() => TogglePause();

        /*!
        @brief Open the restart confirmation UI.
        */
        private void OpenRestartMenu() { isRestartMenuOpen = true; SetEntityActive(btnRestart, false); SetEntityActive(restartCheckScreen, true); }

        /*!
        @brief Close the restart confirmation UI.
        */
        private void CloseRestartMenu() { isRestartMenuOpen = false; SetEntityActive(restartCheckScreen, false); SetEntityActive(btnRestart, true); }

        /*!
        @brief Check whether a UI button entity was clicked, using a mouse release gate.
        @param btnEntity Button entity reference.
        @return True if clicked, otherwise false.
        */
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

        /*!
        @brief Set an entity active/inactive via internal calls.
        @param e Entity reference.
        @param active True to activate, false to deactivate.
        */
        private void SetEntityActive(Entity e, bool active)
        {
            if (e != null) InternalCalls.SetActive(e.ID, active);
        }
    }
}
