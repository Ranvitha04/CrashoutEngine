/*!
@file       MenuButtons.cs
@author     lee.r@digipen.edu
@date       26/11/2025
@brief      Handles menu button interactions in the game.

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

                //Console.WriteLine("[C#] Name is " + name);
                string n = InternalCalls.GetName((uint)Entity.ID);
                AudioManager.GetInstance().PlayAttached("Click", Entity.ID);

                string msg = "[C#] Name is " + name;
                Debug.Log(msg);

                if (name.ToString() == "StartBtn")
                {
                    //Scene.Manager.LoadScene("descentgame");
                    Debug.Log("[C#] Transitioning to Cutscene...");
                    SceneManager.LoadScene("Cutscene", 2f);
                }
                else if (name.ToString() == "SettingsBtn")
                {
                    //Console.WriteLine("[C#] Loading Scene...");
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentsettings", 0.5f);
                }
                else if (name.ToString() == "CloseBtn")
                {
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentmenu", 0.5f);
                }
                else if (name.ToString() == "QuitBtn")
                {
                    uint quitCheck = InternalCalls.FindEntityByName("QuitCheck");
                    if (quitCheck != 0)
                    {
                        InternalCalls.SetActive(InternalCalls.FindEntityByName("QuitBtn"), false);
                        InternalCalls.SetActive(quitCheck, true);
                        //SceneManager.LoadScene("descentconfirm", 0.5f);
                    }
                    else
                    {
                        // No confirmation UI in this scene -> return to main menu
                        SceneManager.LoadScene("descentmenu", 0.5f);
                    }
                }
                else if (name.ToString() == "Yes")
                {
                    Debug.Log("YES");
                    SceneManager.QuitGame();
                }
                else if (name.ToString() == "No")
                {
                    Debug.Log("NO");
                    InternalCalls.SetActive(InternalCalls.FindEntityByName("QuitBtn"), true);
                    InternalCalls.SetActive(InternalCalls.FindEntityByName("QuitCheck"), false);
                    //SceneManager.LoadScene("descentmenu", 0.5f); 
                }
               //ClosedBook
                else if (name.ToString() == "PauseResumeBtn")
                {
                    // Resume Game from pause menu
                    Debug.Log("[C#] Resume Game from pause menu.");
                    SceneManager.LoadScene("descentgame");
                }
                else if (name.ToString() == "PauseSettingsBtn")
                {
                    // Go to settings from pause menu
                    Debug.Log("[C#] Pause menu -> Settings.");
                    SceneManager.LoadScene("descentsettings");
                }
                else if (name.ToString() == "PauseQuitBtn")
                {
                    
                    SceneManager.LoadScene("descentconfirm");
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
                else if (n == "ExitBtnGame")
                {
                    // whatever your exit flow is
                    SceneManager.LoadScene("descentconfirm");
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
