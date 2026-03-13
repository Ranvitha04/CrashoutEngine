/*!
@file       SimpleMainMenu.cs
@author     Renfred Lee
@date       13/03/2026
@brief      Basic start/quit menu logic for the Racer project.

All content © 2025 Digipen Institute of Technology Singapore.All rights reserved.
*/

using System;
using System.Runtime.CompilerServices;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    /*!
    @brief Handles click interactions for a minimal main menu.
    */
    public class SimpleMainMenu : ScriptBase
    {
        // Drag and drop these in your Inspector
        public Entity StartBtn;
        public Entity QuitBtn;

        // Type the name of the scene you want to load in the Inspector
        public string TargetSceneName = "GameScene";

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
        @brief Handle start/quit button clicks each frame.
        */
        public override void Update()
        {
            // Reset click state when mouse is released
            if (!Input.IsMouseButtonDown(0)) isClicking = false;

            // Tick down the UI cooldown
            if (_clickCooldown > 0)
            {
                _clickCooldown--;
            }

            // --- 1. START BUTTON ---
            if (CheckButtonClick(StartBtn))
            {
                Debug.Log($"[C#] Starting Game. Loading Scene: {TargetSceneName}");
                SceneManager.LoadScene(TargetSceneName, 0.5f);
            }
            // --- 2. QUIT BUTTON ---
            else if (CheckButtonClick(QuitBtn))
            {
                Debug.Log("[C#] Quitting Game...");
                SceneManager.QuitGame();
            }
        }

        public override void FixedUpdate() { }
        public override void OnDestroy() { }

        /*!
        @brief Check whether a button entity was clicked this frame.
        @param btnEnt Button entity reference.
        @return True if clicked, otherwise false.
        */
        private bool CheckButtonClick(Entity btnEnt)
        {
            if (isClicking) return false;
            if (Input.IsMouseButtonDown(0)) isClicking = true;
            if (_clickCooldown > 0) return false;

            if (btnEnt == null || btnEnt.ID == 0) return false;

            Entity buttonEnt = new Entity(btnEnt.ID);
            if (!buttonEnt.IsActive()) return false;

            var btn = buttonEnt.GetComponent<ButtonComponent>();
            var tf = buttonEnt.GetComponent<TransformComponent>();
            
            if (btn == null) return false;

            // 1. Check C++ Component State
            bool clicked = btn.IsClicked();

            // 2. Fallback Bounds Check
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

            // 3. Execution
            if (clicked)
            {
                _clickCooldown = 15; // Lock UI for 15 frames 
                
                // Uncomment this if you want click sounds!
                // AudioManager.GetInstance().PlayAttached("Click", buttonEnt.ID); 
                
                Debug.Log($"[C#] Clicked Button: {InternalCalls.GetName((uint)buttonEnt.ID)}");
                return true;
            }

            return false;
        }
    }
}
