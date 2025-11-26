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
                    //Console.WriteLine("[C#] Loading Scene...");
                    Debug.Log("[C#] Loading Scene...");
                    SceneManager.LoadScene("descentmenu");
                }
                // FOR QuitBtn
                else if (name.ToString() == "QuitBtn")
                {
                    //Console.WriteLine("[C#] Quitting Game...");
                    Debug.Log("[C#] Quitting Game...");
                    SceneManager.QuitGame();
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