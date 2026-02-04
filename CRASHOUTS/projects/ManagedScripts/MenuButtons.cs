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
                   
                    SceneManager.LoadScene("descentconfirm", 0.5f);
                }
                else if (name.ToString() == "QuitYesBtn")
                {
                    SceneManager.QuitGame();
                }
                else if (name.ToString() == "QuitNoBtn")
                {
               
                    SceneManager.LoadScene("descentmenu", 0.5f); 
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