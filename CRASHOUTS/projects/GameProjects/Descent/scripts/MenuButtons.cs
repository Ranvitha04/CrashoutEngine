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
            if (btn.IsClicked() && !_hasClicked)
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
                else if (name.ToString() == "MenuBtn")
                {
                    //Scene.Manager.LoadScene("descentgame");
                    Debug.Log("[C#] Transitioning to Cutscene...");
                    SceneManager.LoadScene("descentmenu", 2f);
                }
                else if (name.ToString() == "RestartBtn")
                {
                    //Scene.Manager.LoadScene("descentgame");
                    Debug.Log("[C#] Transitioning to Cutscene...");
                    SceneManager.LoadScene("tutorial", 2f);
                }
                else if (name.ToString() == "RestartBtn2")
                {
                    //Scene.Manager.LoadScene("descentgame");
                    Debug.Log("[C#] Transitioning to Cutscene...");
                    SceneManager.LoadScene("tutorial", 2f);
                }
                else if (name.ToString() == "Level1Btn")
                {
                    //Scene.Manager.LoadScene("descentgame");
                    Debug.Log("[C#] Transitioning to Cutscene...");
                    SceneManager.LoadScene("tutorial2", 2f);
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
                    InternalCalls.SetActive(InternalCalls.FindEntityByName("QuitBtn"), false);
                    InternalCalls.SetActive(InternalCalls.FindEntityByName("QuitCheck"), true);
                    //SceneManager.LoadScene("descentconfirm", 0.5f);
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
