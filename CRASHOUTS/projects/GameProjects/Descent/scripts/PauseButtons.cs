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
        private const string PauseControllerName = "PauseScript";

        [SerializeField] private string currentScene;

        private Entity pauseScreen, restartCheckScreen, openedBookScreen, closedBookScreen;
        private Entity howTo1Screen, howTo2Screen;
        private Entity btnResume, btnRestart, btnSettings, btnQuit, btnYes, btnNo, btnCloseBook, btnNext, btnBack;

        private PlayerMovement playerScript;
        private bool isPaused = false;
        private bool isRestartMenuOpen = false;
        private bool isSettingsMenuOpen = false;
        private bool isHowTo1Open = false;
        private bool isHowTo2Open = false;
        private float inputCooldown = 0.0f;
        private float pauseToggleCooldown = 0.0f;
        private int selectedButtonIndex = 0;
        private float menuNavigationCooldown = 0.0f;

        // NEW: The Trigger Gate
        private bool isMouseReleased = true; 

        /*!
        @brief Cache entity references for pause UI and buttons.
        */
        public override void Start()
        {
            if (!IsPauseController()) return;

            playerScript = PlayerMovement.Instance;

            pauseScreen = Entity.Find("Pause");
            restartCheckScreen = Entity.Find("RestartCheck");
            openedBookScreen = Entity.Find("OpenedBook");
            closedBookScreen = Entity.Find("ClosedBook");
            howTo1Screen = Entity.Find("HowTo1");
            howTo2Screen = Entity.Find("HowTo2");

            btnResume = Entity.Find("ResumeButton");
            btnRestart = Entity.Find("RestartLevelButton");
            btnSettings = Entity.Find("SettingsButton");
            btnQuit = Entity.Find("QuitButton");
            btnYes = Entity.Find("Yes");
            btnNo = Entity.Find("No");
            btnCloseBook = Entity.Find("CloseBookBtn");
            btnNext = Entity.Find("NextPageBtn");
            btnBack = Entity.Find("BackBtn");

            SetEntityActive(pauseScreen, false);
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(closedBookScreen, false);
            SetEntityActive(howTo1Screen, false);
            SetEntityActive(howTo2Screen, false);
            InternalCalls.UI_ClearForcedHoveredButton();
        }

        /*!
        @brief Handle pause input and pause menu button interactions.
        */
        public override void Update()
        {
            if (!IsPauseController()) return;

            if (playerScript == null) playerScript = PlayerMovement.Instance;
            if (pauseToggleCooldown > 0.0f) pauseToggleCooldown -= (float)Time.DT;

            // 1. Pause Input
            if (pauseToggleCooldown <= 0.0f && Input.GetActionTriggered("Pause"))
            {
                if (playerScript != null && playerScript.isDialogueActive) return;
                TogglePause();
                pauseToggleCooldown = 0.2f;
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
                    if (isHowTo2Open)
                    {
                        Entity[] buttons = { btnBack };
                        UpdateControllerSelection(buttons);
                        if (CheckButtonClick(btnBack) || IsControllerSubmit(buttons, btnBack)) OpenHowTo1();
                    }
                    else if (isHowTo1Open)
                    {
                        Entity[] buttons = { btnNext, btnBack };
                        UpdateControllerSelection(buttons);
                        if (CheckButtonClick(btnNext) || IsControllerSubmit(buttons, btnNext)) OpenHowTo2();
                        else if (CheckButtonClick(btnBack) || IsControllerSubmit(buttons, btnBack)) OpenSettings();
                    }
                    else
                    {
                        Entity[] buttons = { btnCloseBook, btnNext };
                        UpdateControllerSelection(buttons);
                        if (CheckButtonClick(btnCloseBook) || IsControllerSubmit(buttons, btnCloseBook)) CloseSettings();
                        else if (CheckButtonClick(btnNext) || IsControllerSubmit(buttons, btnNext)) OpenHowTo1();
                    }
                }
                else HandleMainPauseButtons();
            }

            if (inputCooldown > 0.0f) inputCooldown -= (float)Time.DT;
            if (menuNavigationCooldown > 0.0f) menuNavigationCooldown -= (float)Time.DT;
        }

        /*!
        @brief Handle the main pause menu button actions.
        */
        private void HandleMainPauseButtons()
        {
            Entity[] buttons = { btnResume, btnRestart, btnSettings, btnQuit };
            UpdateControllerSelection(buttons);

            if (CheckButtonClick(btnResume) || IsControllerSubmit(buttons, btnResume)) ResumeGame();
            else if (CheckButtonClick(btnRestart) || IsControllerSubmit(buttons, btnRestart)) OpenRestartMenu();
            else if (CheckButtonClick(btnSettings) || IsControllerSubmit(buttons, btnSettings)) OpenSettings();
            else if (CheckButtonClick(btnQuit) || IsControllerSubmit(buttons, btnQuit)) SceneManager.LoadScene("descentmenu", 1.0f);
        }

        /*!
        @brief Handle the restart confirmation menu buttons.
        */
        private void HandleRestartConfirmButtons()
        {
            Entity[] buttons = { btnYes, btnNo };
            UpdateControllerSelection(buttons);

            if (CheckButtonClick(btnYes) || IsControllerSubmit(buttons, btnYes))
            {
                if (string.IsNullOrEmpty(currentScene)) currentScene = "Level1";
                SceneManager.LoadScene(currentScene, 1.0f);
            }
            else if (CheckButtonClick(btnNo) || IsControllerSubmit(buttons, btnNo)) CloseRestartMenu();
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
            Input.ClearState();

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
            isHowTo1Open = false;
            isHowTo2Open = false;
            SetEntityActive(restartCheckScreen, false);
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(howTo1Screen, false);
            SetEntityActive(howTo2Screen, false);

            if (isPaused) ResetControllerSelection();
            else InternalCalls.UI_ClearForcedHoveredButton();
        }

        /*!
        @brief Open the settings menu UI.
        */
        private void OpenSettings()
        {
            isSettingsMenuOpen = true;
            isHowTo1Open = false;
            isHowTo2Open = false;
            SetEntityActive(openedBookScreen, true);
            SetEntityActive(howTo1Screen, false);
            SetEntityActive(howTo2Screen, false);
            SetEntityActive(closedBookScreen, false);
            inputCooldown = 0.2f;
            selectedButtonIndex = 0;
        }

        /*!
        @brief Close the settings menu UI.
        */
        private void CloseSettings()
        {
            isSettingsMenuOpen = false;
            isHowTo1Open = false;
            isHowTo2Open = false;
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(howTo1Screen, false);
            SetEntityActive(howTo2Screen, false);
            SetEntityActive(closedBookScreen, true);
            inputCooldown = 0.2f;
            ResetControllerSelection();
        }

        private void OpenHowTo1()
        {
            isHowTo1Open = true;
            isHowTo2Open = false;
            SetEntityActive(openedBookScreen, false);
            SetEntityActive(howTo1Screen, true);
            SetEntityActive(howTo2Screen, false);
            inputCooldown = 0.2f;
            selectedButtonIndex = 0;
        }

        private void OpenHowTo2()
        {
            isHowTo1Open = false;
            isHowTo2Open = true;
            SetEntityActive(howTo1Screen, false);
            SetEntityActive(howTo2Screen, true);
            inputCooldown = 0.2f;
            selectedButtonIndex = 0;
        }

        /*!
        @brief Resume the game (alias for toggling pause off).
        */
        private void ResumeGame() => TogglePause();

        /*!
        @brief Open the restart confirmation UI.
        */
        private void OpenRestartMenu() { isRestartMenuOpen = true; SetEntityActive(btnRestart, false); SetEntityActive(restartCheckScreen, true); selectedButtonIndex = 0; }

        /*!
        @brief Close the restart confirmation UI.
        */
        private void CloseRestartMenu() { isRestartMenuOpen = false; SetEntityActive(restartCheckScreen, false); SetEntityActive(btnRestart, true); ResetControllerSelection(); }

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

        private void ResetControllerSelection()
        {
            selectedButtonIndex = 0;
            menuNavigationCooldown = 0.0f;
        }

        private void UpdateControllerSelection(Entity[] buttons)
        {
            if (buttons == null || buttons.Length == 0)
            {
                InternalCalls.UI_ClearForcedHoveredButton();
                return;
            }

            float menuY = Input.GetAxis("MenuY");
            if (menuNavigationCooldown <= 0.0f)
            {
                if (Input.GetActionTriggered("UIDown") || menuY <= -0.5f)
                {
                    selectedButtonIndex = (selectedButtonIndex + 1) % buttons.Length;
                    menuNavigationCooldown = 0.2f;
                }
                else if (Input.GetActionTriggered("UIUp") || menuY >= 0.5f)
                {
                    selectedButtonIndex = (selectedButtonIndex - 1 + buttons.Length) % buttons.Length;
                    menuNavigationCooldown = 0.2f;
                }
            }

            if (selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                selectedButtonIndex = 0;
            }

            Entity selected = buttons[selectedButtonIndex];
            if (selected != null && selected.IsActive())
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
            if (target == null || buttons == null || selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                return false;
            }

            return buttons[selectedButtonIndex] == target && Input.GetActionTriggered("UISubmit");
        }

        private bool IsPauseController()
        {
            return Entity != null &&
                   Entity.ID != 0 &&
                   InternalCalls.GetName(Entity.ID) == PauseControllerName;
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

        public override void OnDestroy()
        {
            if (IsPauseController()) InternalCalls.UI_ClearForcedHoveredButton();
        }
    }
}
