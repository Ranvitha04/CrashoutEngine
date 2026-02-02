using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class MenuButtons : ScriptBase
    {
        public static bool QuitFromPauseMenu = false;
        public override void Start()
        {
            //Debug.Log("Start");
        }

        public override void Update()
        {
            
        //Debug.Log("TEST");
        var btn = Entity.GetComponent<ButtonComponent>();
            var name = Entity.GetComponent<NameComponent>();
            if (btn.IsClicked())
            {
                //Console.WriteLine("[C#] Name is " + name);

                string msg = "[C#] Name is " + name;
                Debug.Log(msg);

               
                if (name.ToString() == "StartBtn")
                {
                    //Scene.Manager.LoadScene("descentgame");
                    Debug.Log("[C#] Transitioning to Tutorial...");
                    SceneManager.TransitionToScene("tutorial", 2f);
                }
             
                else if (name.ToString() == "SettingsBtn")
                {
                    //Console.WriteLine("[C#] Loading Scene...");
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentsettings");
                }
                
                else if (name.ToString() == "CloseBtn")
                {
                    
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentmenu");
                }
                
                else if (name.ToString() == "QuitBtn")
                {
                    QuitFromPauseMenu = false;
                    SceneManager.LoadScene("descentClosedBook");
                }
                else if (name.ToString() == "QuitYesBtn")
                {
                    if (QuitFromPauseMenu)
                    {
                       
                        SceneManager.LoadScene("descentmenu");
                    }
                    else
                    {
                      
                        SceneManager.QuitGame();
                    }
                }
                else if (name.ToString() == "QuitNoBtn")
                {

                if (QuitFromPauseMenu)
                    {
                       
                        
                        SceneManager.LoadScene("descentpause");
                        
                    }
                    else
                    {
                      
                        SceneManager.LoadScene("descentmenu");
                    }
                }
               
                else if (name.ToString() == "PauseResumeBtn")
                {
                    // Resume Game from pause menu
                    Debug.Log("[C#] Resume Game from pause menu.");
                    SceneManager.LoadScene("tutorial");
                }
                else if (name.ToString() == "PauseSettingsBtn")
                {
                    // Go to settings from pause menu
                    Debug.Log("[C#] Pause menu -> Settings.");
                    SceneManager.LoadScene("descentsettings");
                }
                else if (name.ToString() == "PauseQuitBtn")
                {
                    QuitFromPauseMenu = true;
                    SceneManager.LoadScene("descentmenu");
                }
                else if (name.ToString() == "ResumeBtn")
                {

                    SceneManager.LoadScene("descentgame");
                }


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