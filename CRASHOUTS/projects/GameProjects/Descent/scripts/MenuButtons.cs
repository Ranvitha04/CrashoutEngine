/*!
@file      MenuButtons.cs
@author    lee.r@digipen.edu
@date      26/11/2025
@brief     Handles menu button interactions in the game.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class MenuButtons : ScriptBase
    {
        private bool _hasClicked = false;

        public override void Start()
        {
            //Debug.Log("Start");
        }

        public override void Update()
        {
            //Debug.Log("TEST");
            var btn = Entity.GetComponent<ButtonComponent>();
            var name = Entity.GetComponent<NameComponent>();
            var tf = Entity.GetComponent<TransformComponent>();
            bool clicked = btn.IsClicked();
            
            if (!clicked && tf != null)
            {
                // Fallback for scenes where button state isn't updated
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

            if (clicked && !_hasClicked)
            {
                _hasClicked = true;

                string n = InternalCalls.GetName((uint)Entity.ID);
                string nameStr = name.ToString(); // Cached for easier reading
                
                AudioManager.GetInstance().PlayAttached("Click", Entity.ID);

                string msg = "[C#] Name is " + nameStr;
                Debug.Log(msg);

                if (nameStr == "StartBtn")
                {
                    Debug.Log("[C#] Transitioning to Cutscene...");
                    SceneManager.LoadScene("Cutscene", 2f);
                }
                else if (nameStr == "SettingsBtn" || nameStr == "PauseSettingsBtn")
                {
                    Debug.Log("[C#] Loading Settings...");
                    SceneManager.LoadScene("descentsettings", 0.5f);
                }
                else if (nameStr == "CloseBtn")
                {
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentmenu", 0.5f);
                }
                else if (nameStr == "PauseResumeBtn")
                {
                    // Resume Game from pause menu
                    Debug.Log("[C#] Resume Game from pause menu.");
                    SceneManager.LoadScene("descentgame");
                }
                // --- UNIFIED QUIT BUTTON LOGIC ---
                else if (nameStr == "QuitBtn" || nameStr == "PauseQuitBtn" || n == "ExitBtnGame")
                {
                    uint quitCheck = InternalCalls.FindEntityByName("QuitCheck");
                    if (quitCheck != 0)
                    {
                        // Hide whichever quit button was pressed so it can't be spammed
                        InternalCalls.SetActive(InternalCalls.FindEntityByName(n), false);
                        // Show the Yes/No confirmation overlay
                        InternalCalls.SetActive(quitCheck, true);
                    }
                    else
                    {
                        // Fallback if no QuitCheck exists in the scene
                        SceneManager.LoadScene("descentmenu", 0.5f);
                    }
                }
                // --- YES / NO CONFIRMATION LOGIC ---
                else if (nameStr == "Yes")
                {
                    Debug.Log("YES");
                    SceneManager.QuitGame();
                }
                else if (nameStr == "No")
                {
                    Debug.Log("NO");
                    // Hide the confirmation UI
                    InternalCalls.SetActive(InternalCalls.FindEntityByName("QuitCheck"), false);
                    
                    // Safely attempt to reactivate any quit buttons that might have been disabled
                    uint quitBtn = InternalCalls.FindEntityByName("QuitBtn");
                    if (quitBtn != 0) InternalCalls.SetActive(quitBtn, true);

                    uint pauseQuitBtn = InternalCalls.FindEntityByName("PauseQuitBtn");
                    if (pauseQuitBtn != 0) InternalCalls.SetActive(pauseQuitBtn, true);

                    uint exitBtnGame = InternalCalls.FindEntityByName("ExitBtnGame");
                    if (exitBtnGame != 0) InternalCalls.SetActive(exitBtnGame, true);
                }

                // --- CLOSED BOOK BUTTONS ---
                if (n == "ResumeBtnGame")
                {
                    // close all book UI (resume game)
                    if (BookUIController.Instance != null)
                        BookUIController.Instance.HideAll();
                }
                else if (n == "SettingBtnGame")
                {
                    // go to open book UI
                    if (BookUIController.Instance != null)
                        BookUIController.Instance.ShowOpenBook();
                }
                // --- OPEN BOOK BUTTONS ---
                else if (n == "ClosedOpenBookBtn")
                {
                    if (BookUIController.Instance != null)
                        BookUIController.Instance.ShowClosedBook();
                }
            }
            else if (!btn.IsClicked())
            {
                _hasClicked = false;
            }

            if (btn.IsHovered())
            {
                //Console.WriteLine("Hovered");
            }
        }

        public override void FixedUpdate()
        {
        }

        public override void OnDestroy()
        {
        }
    }
}