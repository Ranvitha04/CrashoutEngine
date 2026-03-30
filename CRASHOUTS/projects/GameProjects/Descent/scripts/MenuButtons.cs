/*!
@file       MenuButtons.cs
@author     Renfred (RTIS)
@author     m.biniswandi
@author     Renfred Lee
@date       13/03/2026
@brief      UI button logic for the Descent main menu and quit confirmation.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Handles menu button clicks and scene transitions.
    */
    public class MenuButtons : ScriptBase
    {
        private const string MenuControllerName = "MenuScript";
        private const string SettingsControllerName = "SettingsScript";

        public Entity StartBtn;
        public Entity SettingsBtn;
        public Entity CloseBtn;
        public Entity QuitBtn;

        public Entity QuitCheckPanel;
        public Entity YesBtn;
        public Entity NoBtn;

        private bool isClicking = false;

        // NEW: A simple frame cooldown to prevent overlapping buttons from swapping instantly
        private int _clickCooldown = 0;
        private bool isMouseReleased = true;
        private int selectedButtonIndex = 0;
        private float menuNavigationCooldown = 0.0f;
        private bool controllerNavigationActive = false;
        [SerializeField] private float navigationRumbleLow = 0.15f;
        [SerializeField] private float navigationRumbleHigh = 0.25f;
        [SerializeField] private float navigationRumbleDuration = 0.05f;

        /*!
        @brief Initialize internal click state.
        */
        public override void Start()
        {
            isClicking = false;
            isMouseReleased = true;
            ResetControllerSelection();
            if (IsMenuController()) InternalCalls.UI_ClearForcedHoveredButton();
        }

        /*!
        @brief Poll for UI button clicks and perform the corresponding actions.
        */
        public override void Update()
        {
            if (!IsMenuController()) return;

            if(!Input.IsMouseButtonDown(0)) isClicking = false;
            if (!Input.IsMouseButtonDown(0)) isMouseReleased = true;

            // Tick down the cooldown every frame
            if (_clickCooldown > 0)
            {
                _clickCooldown--;
            }
            if (menuNavigationCooldown > 0.0f)
            {
                menuNavigationCooldown -= (float)Time.DT;
            }

            Entity[] activeButtons = GetActiveButtons();
            UpdateControllerSelection(activeButtons);

            // --- MAIN MENU ---
            if (CheckButtonClick(StartBtn) || IsControllerSubmit(activeButtons, StartBtn))
            {
                HandleButtonActivated(StartBtn);
            }
            else if (CheckButtonClick(SettingsBtn) || IsControllerSubmit(activeButtons, SettingsBtn))
            {
                HandleButtonActivated(SettingsBtn);
            }
            else if (CheckButtonClick(CloseBtn) || IsControllerSubmit(activeButtons, CloseBtn))
            {
                HandleButtonActivated(CloseBtn);
            }
            
            // --- UNIFIED QUIT LOGIC ---
            else if (CheckButtonClick(QuitBtn) || IsControllerSubmit(activeButtons, QuitBtn))
            {
                HandleButtonActivated(QuitBtn);
            }

            // --- YES / NO CONFIRMATION ---
            else if (CheckButtonClick(YesBtn) || IsControllerSubmit(activeButtons, YesBtn))
            {
                HandleButtonActivated(YesBtn);
            }
            else if (CheckButtonClick(NoBtn) || IsControllerSubmit(activeButtons, NoBtn))
            {
                HandleButtonActivated(NoBtn);
            }
        }

        public override void FixedUpdate() { }

        /*!
        @brief Check whether a button entity was clicked this frame.
        @param btnEnt Button entity reference.
        @return True if the button was clicked, otherwise false.
        */
        private bool CheckButtonClick(Entity btnEnt)
        {
            if(isClicking) return false;
            if(Input.IsMouseButtonDown(0)) isClicking = true;
            // 1. If we are in the cooldown period, ignore all clicks!
            if (_clickCooldown > 0) return false;
            if (!isMouseReleased) return false;

            if (btnEnt == null || btnEnt.ID == 0) return false;

            Entity buttonEnt = new Entity(btnEnt.ID);
            if (!buttonEnt.IsActive()) return false;

            var btn = buttonEnt.GetComponent<ButtonComponent>();
            var tf = buttonEnt.GetComponent<TransformComponent>();
            
            if (btn == null) return false;

            // 2. Let C++ evaluate normally! This fixes the CloseBtn bug.
            bool clicked = btn.IsClicked();

            // 3. Fallback bounds check if C++ misses the click
            if (!clicked && tf != null)
            {
                if (Input.IsMouseButtonTriggered(0))
                {
                    CEVec2 mouse = Input.GetMouseWorldPosition();
                    float halfW = tf.Size.x * 0.5f;
                    float halfH = tf.Size.y * 0.5f;
                    
                    if (mouse.x >= tf.Position.x - halfW && mouse.x <= tf.Position.x + halfW &&
                        mouse.y >= tf.Position.y - halfH && mouse.y <= tf.Position.y + halfH)
                    {
                        clicked = true;
                    }
                }
            }

            // 4. Action execution and Cooldown Trigger
            if (clicked)
            {
                _clickCooldown = 15; // Lock UI for 15 frames to stop rapid Quit/No swapping
                isMouseReleased = false;
                controllerNavigationActive = false;
                InternalCalls.UI_ClearForcedHoveredButton();
                
                return true;
            }

            return false;
        }

        private void HandleButtonActivated(Entity button)
        {
            if (button == null || button.ID == 0) return;

            AudioManager.GetInstance().PlayAttached("Click", button.ID);
            Debug.Log($"[C#] Clicked Button: {InternalCalls.GetName((uint)button.ID)}");

            if (button == StartBtn)
            {
                Debug.Log("[C#] Transitioning to Cutscene...");
                SceneManager.LoadScene("Cutscene", 2f);
            }
            else if (button == SettingsBtn)
            {
                Debug.Log("[C#] Loading Settings...");
                SceneManager.LoadScene("descentsettings", 0.5f);
            }
            else if (button == CloseBtn)
            {
                Debug.Log("[C#] Loading Scene...");
                SceneManager.LoadScene("descentmenu", 0.5f);
            }
            else if (button == QuitBtn)
            {
                if (QuitCheckPanel != null && QuitCheckPanel.ID != 0)
                {
                    if (QuitBtn != null && QuitBtn.ID != 0) InternalCalls.SetActive(QuitBtn.ID, false);
                    InternalCalls.SetActive(QuitCheckPanel.ID, true);
                    ResetControllerSelection();
                }
                else
                {
                    SceneManager.LoadScene("descentmenu", 0.5f);
                }
            }
            else if (button == YesBtn)
            {
                Debug.Log("YES");
                SceneManager.QuitGame();
            }
            else if (button == NoBtn)
            {
                Debug.Log("NO");
                if (QuitCheckPanel != null && QuitCheckPanel.ID != 0) InternalCalls.SetActive(QuitCheckPanel.ID, false);
                if (QuitBtn != null && QuitBtn.ID != 0) InternalCalls.SetActive(QuitBtn.ID, true);
                ResetControllerSelection();
            }
        }

        private Entity[] GetActiveButtons()
        {
            Entity validatedQuitCheckPanel = GetValidatedButton(QuitCheckPanel, "QuitCheck");
            if (validatedQuitCheckPanel != null && validatedQuitCheckPanel.IsActive())
            {
                System.Collections.Generic.List<Entity> confirmButtons = new System.Collections.Generic.List<Entity>();
                TryAddActiveButton(confirmButtons, YesBtn, "Yes");
                TryAddActiveButton(confirmButtons, NoBtn, "No");
                return confirmButtons.ToArray();
            }

            System.Collections.Generic.List<Entity> buttons = new System.Collections.Generic.List<Entity>();
            TryAddActiveButton(buttons, StartBtn, "StartBtn");
            TryAddActiveButton(buttons, SettingsBtn, "SettingsBtn");
            TryAddActiveButton(buttons, CloseBtn, "CloseBtn");
            TryAddActiveButton(buttons, QuitBtn, "QuitBtn");
            return buttons.ToArray();
        }

        private bool IsMenuController()
        {
            string controllerName = InternalCalls.GetName(Entity.ID);
            return Entity != null &&
                   Entity.ID != 0 &&
                   (controllerName == MenuControllerName || controllerName == SettingsControllerName) &&
                   HasConfiguredButtons();
        }

        private bool HasConfiguredButtons()
        {
            return GetValidatedButton(StartBtn, "StartBtn") != null ||
                   GetValidatedButton(SettingsBtn, "SettingsBtn") != null ||
                   GetValidatedButton(CloseBtn, "CloseBtn") != null ||
                   GetValidatedButton(QuitBtn, "QuitBtn") != null ||
                   GetValidatedButton(YesBtn, "Yes") != null ||
                   GetValidatedButton(NoBtn, "No") != null;
        }

        private void TryAddActiveButton(System.Collections.Generic.List<Entity> buttons, Entity button, string expectedName)
        {
            Entity validatedButton = GetValidatedButton(button, expectedName);
            if (validatedButton != null && validatedButton.IsActive())
            {
                buttons.Add(validatedButton);
            }
        }

        private Entity GetValidatedButton(Entity button, string expectedName)
        {
            if (button == null || button.ID == 0) return null;
            if (InternalCalls.GetName(button.ID) != expectedName) return null;
            return button;
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

            if (movedDown || movedUp)
            {
                Input.SetControllerVibration(navigationRumbleLow, navigationRumbleHigh, navigationRumbleDuration);
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
            if (_clickCooldown > 0 || target == null || target.ID == 0 || buttons == null)
            {
                return false;
            }

            if (selectedButtonIndex < 0 || selectedButtonIndex >= buttons.Length)
            {
                return false;
            }

            return buttons[selectedButtonIndex] == target && Input.GetActionTriggered("UISubmit");
        }

        public override void OnDestroy()
        {
            if (IsMenuController()) InternalCalls.UI_ClearForcedHoveredButton();
        }
    }
}
