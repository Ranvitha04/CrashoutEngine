/*!
@file       SubmitScoreButton.cs
@author     Renfred Lee
@author     Renfred (RTIS)
@date       13/03/2026
@brief      Submits the player's race time to the leaderboard via a UI button.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Reads the player's name input and submits the final time to the leaderboard.
    */
    public class SubmitScoreButton : ScriptBase
    {
        public Entity submitBtn;       // <-- Drag the Submit Button entity here
        public Entity textInputBox;    
        public Entity leaderboardUI;   
        public Entity nameInputPanel;  

        private bool isClicking = false;
        private int _clickCooldown = 0;

        /*!
        @brief Initialize click state.
        */
        public override void Start()
        {
            isClicking = false;
            _clickCooldown = 0;
        }

        /*!
        @brief Poll for submit button clicks and trigger score submission.
        */
        public override void Update() 
        {
            if(!Input.IsMouseButtonDown(0)) isClicking = false;

            // Tick down the cooldown every frame
            if (_clickCooldown > 0)
            {
                _clickCooldown--;
            }

            // Check if the submit button was clicked this frame
            if (CheckButtonClick(submitBtn))
            {
                ExecuteSubmit();
            }
        }

        /*!
        @brief Collect player name and time, submit them, and refresh the leaderboard UI.
        */
        private void ExecuteSubmit() 
        {
            string typedName = "Anonymous";

            // 1. Get the name using our new InternalCall
            if (textInputBox != null && textInputBox.ID != 0)
            {
                string uiText = InternalCalls.GetText(textInputBox.ID);
                // Make sure they actually typed something and it's not a placeholder
                if (!string.IsNullOrEmpty(uiText) && uiText != "Enter Name:") 
                {
                    typedName = uiText;
                }
            }

            // 2. Get the saved time from LapTracker
            float finalTime = LapTracker.Instance.FinalTime;

            // 3. Submit the score
            Debug.Log($"Submitting {typedName} with time {finalTime}");
            Leaderboard.SubmitTime(typedName, finalTime);

            // 4. Force the Leaderboard to refresh immediately
            if (leaderboardUI != null && leaderboardUI.ID != 0)
            {
                Leaderboard.ForceRefresh(leaderboardUI);
            }

            // 5. Hide the popup panel 
            if (nameInputPanel != null && nameInputPanel.ID != 0)
            {
                // Note: casting to uint to match your MenuButtons.cs architecture
                InternalCalls.SetActive((uint)nameInputPanel.ID, false); 
            }
        }

        /*!
        @brief Check whether a button entity was clicked this frame.
        @param btnEnt Button entity reference.
        @return True if clicked, otherwise false.
        */
        private bool CheckButtonClick(Entity btnEnt)
        {
            if(isClicking) return false;
            if(Input.IsMouseButtonDown(0)) isClicking = true;
            if (_clickCooldown > 0) return false;

            if (btnEnt == null || btnEnt.ID == 0) return false;

            Entity buttonEnt = new Entity(btnEnt.ID);
            if (!buttonEnt.IsActive()) return false;

            var btn = buttonEnt.GetComponent<ButtonComponent>();
            var tf = buttonEnt.GetComponent<TransformComponent>();
            
            if (btn == null) return false;

            bool clicked = btn.IsClicked();

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

            if (clicked)
            {
                _clickCooldown = 15; // Lock UI for 15 frames to prevent double-submits
                AudioManager.GetInstance().PlayAttached("Click", buttonEnt.ID);
                Debug.Log($"[C#] Clicked Button: {InternalCalls.GetName((uint)buttonEnt.ID)}");
                return true;
            }

            return false;
        }
    }
}
