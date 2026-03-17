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

        /*!
        @brief Initialize internal click state.
        */
        public override void Start()
        {
            isClicking = false;
        }

        /*!
        @brief Poll for UI button clicks and perform the corresponding actions.
        */
        public override void Update()
        {
            if(!Input.IsMouseButtonDown(0)) isClicking = false;

            // Tick down the cooldown every frame
            if (_clickCooldown > 0)
            {
                _clickCooldown--;
            }

            // --- MAIN MENU ---
            if (CheckButtonClick(StartBtn))
            {
                Debug.Log("[C#] Transitioning to Cutscene...");
                SceneManager.LoadScene("Cutscene", 2f);
            }
            else if (CheckButtonClick(SettingsBtn))
            {
                Debug.Log("[C#] Loading Settings...");
                SceneManager.LoadScene("descentsettings", 0.5f);
            }
            else if (CheckButtonClick(CloseBtn))
            {
                Debug.Log("[C#] Loading Scene...");
                SceneManager.LoadScene("descentmenu", 0.5f);
            }
            
            // --- UNIFIED QUIT LOGIC ---
            else if (CheckButtonClick(QuitBtn))
            {
                if (QuitCheckPanel.ID != 0) 
                {
                    if (QuitBtn.ID != 0) InternalCalls.SetActive((uint)QuitBtn.ID, false);
                    InternalCalls.SetActive((uint)QuitCheckPanel.ID, true);
                }
                else
                {
                    SceneManager.LoadScene("descentmenu", 0.5f);
                }
            }

            // --- YES / NO CONFIRMATION ---
            else if (CheckButtonClick(YesBtn))
            {
                Debug.Log("YES");
                SceneManager.QuitGame();
            }
            else if (CheckButtonClick(NoBtn))
            {
                Debug.Log("NO");
                if (QuitCheckPanel.ID != 0) InternalCalls.SetActive((uint)QuitCheckPanel.ID, false);
                if (QuitBtn.ID != 0) InternalCalls.SetActive((uint)QuitBtn.ID, true);
            }
        }

        public override void FixedUpdate() { }
        public override void OnDestroy() { }

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
                
                AudioManager.GetInstance().PlayAttached("Click", buttonEnt.ID);
                Debug.Log($"[C#] Clicked Button: {InternalCalls.GetName((uint)buttonEnt.ID)}");
                return true;
            }

            return false;
        }
    }
}
