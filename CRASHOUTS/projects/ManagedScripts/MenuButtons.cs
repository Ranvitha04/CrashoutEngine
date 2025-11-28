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

                // FOR StartBtn
                if (name.ToString() == "StartBtn")
                {
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentgame");
                }
                // FOR SettingsBtn
                else if (name.ToString() == "SettingsBtn")
                {
                    //Console.WriteLine("[C#] Loading Scene...");
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentsettings");
                }
                // FOR CloseBtn
                else if (name.ToString() == "CloseBtn")
                {
                    
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentmenu");
                }
                // FOR QuitBtn
                else if (name.ToString() == "QuitBtn")
                {
                    // MAIN MENU "Quit Game" button
                    Debug.Log("[C#] Open quit confirmation scene.");
                    SceneManager.LoadScene("descentconfirm");
                }
                else if (name.ToString() == "QuitYesBtn")
                {
                    // YES on confirmation
                    Debug.Log("[C#] Quitting Game from confirmation.");
                    SceneManager.QuitGame();
                }
                else if (name.ToString() == "QuitNoBtn")
                {
                    // NO on confirmation -> go back to main menu
                    Debug.Log("[C#] Cancel quit, back to menu.");
                    SceneManager.LoadScene("descentmenu"); 
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