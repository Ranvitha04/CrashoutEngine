using System;

namespace CrashoutsEngine
{
    [ScriptBehaviour]
    public class MenuButtons : ScriptBase
    {
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
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentgame");
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
                   
                    SceneManager.LoadScene("descentconfirm");
                }
                else if (name.ToString() == "QuitYesBtn")
                {
                    SceneManager.QuitGame();
                }
                else if (name.ToString() == "QuitNoBtn")
                {
               
                    SceneManager.LoadScene("descentmenu"); 
                }
               
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